/*
net_psp.c - BSD socket subset over PSP ad-hoc (PDP), ad-hoc commands
Copyright (C) 2026 rLeo2023

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

See net_psp.h. Two commands drive it:
  adhoc_host  joins the ad-hoc group; the menu then starts a listen server
              as usual and it is reachable at port 27015
  adhoc_join  joins the group, broadcasts a "ping" to port 27015 and
              connects to the first PSP that answers
*/
#include "common.h"
#include "netchan.h"
#include "platform/psp/net_psp.h"

#include <pspkernel.h>
#include <pspnet.h>
#include <pspnet_adhoc.h>
#include <pspnet_adhocctl.h>
#include <psputility.h>
#include <psputility_netmodules.h>

#define ADHOC_GROUP        "CSTRIKE"
#define ADHOC_PRODUCT      "ULUS99999"
#define ADHOC_MAX_SOCKETS  4
#define ADHOC_MAX_PEERS    16
#define ADHOC_PDP_BUFSIZE  0x4000
#define ADHOC_SERVER_PORT  27015
#define ADHOC_SEARCH_PORT  27006
#define ADHOC_JOIN_PORT    27007	// client port of a PSP that joins
#define ADHOC_MAX_DATAGRAM 2048	// game packets stay under ~1400 bytes
#define ADHOC_ERROR_WOULD_BLOCK ((int)0x80410709)
#define ADHOC_STATE_CONNECTED 1

typedef struct
{
	qboolean	used;
	int		pdp;	// PDP id once bound, -1 before
	unsigned short	port;
} adhoc_socket_t;

typedef struct
{
	byte	ip[4];
	byte	mac[6];
} adhoc_peer_t;

int psp_net_errno;

static qboolean		adhoc_active;
static qboolean		adhoc_libsready;	// modules loaded, net/adhoc/adhocctl initialized
static qboolean		adhoc_joiner;	// joined someone else's game: no server port
static byte		adhoc_mac[6];
static adhoc_socket_t	adhoc_sockets[ADHOC_MAX_SOCKETS];
static adhoc_peer_t	adhoc_peers[ADHOC_MAX_PEERS];
static int		adhoc_numpeers;
static int		adhoc_senderrors;

static const byte adhoc_broadcast[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

/*
==============
address mapping: 10.mac[3].mac[4].mac[5]
==============
*/
static void Adhoc_MacToIp( const byte *mac, byte *ip )
{
	ip[0] = 10;
	ip[1] = mac[3];
	ip[2] = mac[4];
	ip[3] = mac[5];
}

static void Adhoc_AddPeer( const byte *mac )
{
	byte	ip[4];
	int	i;

	Adhoc_MacToIp( mac, ip );

	for( i = 0; i < adhoc_numpeers; i++ )
	{
		if( !memcmp( adhoc_peers[i].mac, mac, 6 ))
			return;
	}

	if( adhoc_numpeers >= ADHOC_MAX_PEERS )
		return;

	memcpy( adhoc_peers[adhoc_numpeers].mac, mac, 6 );
	memcpy( adhoc_peers[adhoc_numpeers].ip, ip, 4 );
	adhoc_numpeers++;

	Con_Printf( "S_PSP: adhoc peer %02x:%02x:%02x:%02x:%02x:%02x is %i.%i.%i.%i\n",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ip[0], ip[1], ip[2], ip[3] );
}

// peers the ad-hoc control library knows about, for addresses never heard from
static void Adhoc_RefreshPeers( void )
{
	static struct SceNetAdhocctlPeerInfo	list[ADHOC_MAX_PEERS];
	struct SceNetAdhocctlPeerInfo	*peer;
	int	length = sizeof( list );

	memset( list, 0, sizeof( list ));

	if( sceNetAdhocctlGetPeerList( &length, list ) < 0 || length <= 0 )
		return;

	for( peer = list; peer; peer = peer->next )
	{
		Adhoc_AddPeer( peer->mac );
		if( (byte *)( peer + 1 ) > (byte *)list + length )
			break;
	}
}

static qboolean Adhoc_IpToMac( const byte *ip, byte *mac )
{
	int	pass, i;

	if( ip[0] == 255 && ip[1] == 255 && ip[2] == 255 && ip[3] == 255 )
	{
		memcpy( mac, adhoc_broadcast, 6 );
		return true;
	}

	for( pass = 0; pass < 2; pass++ )
	{
		for( i = 0; i < adhoc_numpeers; i++ )
		{
			if( !memcmp( adhoc_peers[i].ip, ip, 4 ))
			{
				memcpy( mac, adhoc_peers[i].mac, 6 );
				return true;
			}
		}

		if( !pass )
			Adhoc_RefreshPeers();
	}

	return false;
}

/*
==============
sockets
==============
*/
static adhoc_socket_t *Adhoc_GetSocket( int s )
{
	if( s < 1 || s > ADHOC_MAX_SOCKETS || !adhoc_sockets[s - 1].used )
		return NULL;
	return &adhoc_sockets[s - 1];
}

int PSP_NetSocket( int type )
{
	int	i;

	if( !adhoc_active || type != SOCK_DGRAM )
	{
		psp_net_errno = WSAENETDOWN;
		return -1;
	}

	for( i = 0; i < ADHOC_MAX_SOCKETS; i++ )
	{
		if( !adhoc_sockets[i].used )
		{
			adhoc_sockets[i].used = true;
			adhoc_sockets[i].pdp = -1;
			adhoc_sockets[i].port = 0;
			return i + 1;	// 0 is a valid descriptor for the engine, keep it unused
		}
	}

	psp_net_errno = WSAEMFILE;
	return -1;
}

int PSP_NetBind( int s, const struct sockaddr *addr )
{
	adhoc_socket_t	*sock = Adhoc_GetSocket( s );
	unsigned short	port;
	int		pdp;

	if( !sock )
	{
		psp_net_errno = WSAENOTSOCK;
		return -1;
	}

	port = ntohs( ((const struct sockaddr_in *)addr)->sin_port );

	// a PSP that joins has no use for the server port, and its client
	// port differs from the host's: two PPSSPP instances on one PC would
	// otherwise bind the same ports and catch each other's packets
	if( adhoc_joiner )
	{
		if( port == ADHOC_SERVER_PORT )
		{
			psp_net_errno = WSAEADDRINUSE;
			return -1;
		}
		port = ADHOC_JOIN_PORT;
	}
	else if( !port ) port = PORT_CLIENT;

	pdp = sceNetAdhocPdpCreate( adhoc_mac, port, ADHOC_PDP_BUFSIZE, 0 );
	if( pdp < 0 )
	{
		Con_Printf( "S_PSP: adhoc PdpCreate port %i failed: 0x%08x\n", port, pdp );
		psp_net_errno = WSAEADDRINUSE;
		return -1;
	}

	sock->pdp = pdp;
	sock->port = port;
	return 0;
}

int PSP_NetSendTo( int s, const void *buf, int len, const struct sockaddr *to )
{
	adhoc_socket_t	*sock = Adhoc_GetSocket( s );
	const struct sockaddr_in	*sin = (const struct sockaddr_in *)to;
	byte		mac[6];
	int		ret;

	if( !sock || sock->pdp < 0 )
	{
		psp_net_errno = WSAENOTSOCK;
		return -1;
	}

	if( !Adhoc_IpToMac( (const byte *)&sin->sin_addr.s_addr, mac ))
	{
		psp_net_errno = WSAEHOSTDOWN;
		return -1;
	}

	// like a too big UDP datagram: the server's connection speed test
	// sends 64000 bytes, which jammed the other side's receive queue. It
	// fails and the client falls back to the normal connection
	if( len > ADHOC_MAX_DATAGRAM )
	{
		psp_net_errno = WSAEMSGSIZE;
		return -1;
	}

	ret = sceNetAdhocPdpSend( sock->pdp, mac, ntohs( sin->sin_port ), (void *)buf, len, 0, 1 );
	if( ret < 0 )
	{
		// a few are enough to see what goes wrong, the game sends every frame
		if( adhoc_senderrors++ < 8 )
			Con_Printf( "S_PSP: adhoc send of %i bytes failed: 0x%08x\n", len, ret );
		psp_net_errno = WSAEWOULDBLOCK;
		return -1;
	}


	return len;
}

static void Adhoc_DropDatagram( int pdp, int error )
{
	static byte	junk[0x10000];
	static int	logged;
	unsigned short	port;
	byte		mac[6];
	int		size = sizeof( junk );
	int		ret = sceNetAdhocPdpRecv( pdp, mac, &port, junk, &size, 0, 1 );

	if( logged++ < 8 )
		Con_Printf( "S_PSP: adhoc receive error 0x%08x, dropped %i bytes (0x%08x)\n", error, ret >= 0 ? size : 0, ret );
}

int PSP_NetRecvFrom( int s, void *buf, int len, struct sockaddr *from )
{
	adhoc_socket_t	*sock = Adhoc_GetSocket( s );
	struct sockaddr_in	*sin = (struct sockaddr_in *)from;
	unsigned short	port = 0;
	byte		mac[6];
	int		size = len;
	int		ret;

	if( !sock || sock->pdp < 0 )
	{
		psp_net_errno = WSAENOTSOCK;
		return -1;
	}

	ret = sceNetAdhocPdpRecv( sock->pdp, mac, &port, buf, &size, 0, 1 );
	if( ret < 0 || size <= 0 )
	{
		// anything but "nothing there" may be a datagram bigger than buf,
		// which stays first in the queue and blocks all the others
		if( ret < 0 && ret != ADHOC_ERROR_WOULD_BLOCK )
			Adhoc_DropDatagram( sock->pdp, ret );

		psp_net_errno = WSAEWOULDBLOCK;
		return -1;
	}

	Adhoc_AddPeer( mac );

	memset( from, 0, sizeof( *from ));
	sin->sin_family = AF_INET;
	sin->sin_port = htons( port );
	Adhoc_MacToIp( mac, (byte *)&sin->sin_addr.s_addr );


	return size;
}

int PSP_NetClose( int s )
{
	adhoc_socket_t	*sock = Adhoc_GetSocket( s );

	if( !sock )
		return -1;

	if( sock->pdp >= 0 )
		sceNetAdhocPdpDelete( sock->pdp, 0 );

	sock->used = false;
	sock->pdp = -1;
	return 0;
}

int PSP_NetGetSockName( int s, struct sockaddr *addr )
{
	adhoc_socket_t	*sock = Adhoc_GetSocket( s );
	struct sockaddr_in	*sin = (struct sockaddr_in *)addr;

	if( !sock )
	{
		psp_net_errno = WSAENOTSOCK;
		return -1;
	}

	memset( addr, 0, sizeof( *addr ));
	sin->sin_family = AF_INET;
	sin->sin_port = htons( sock->port );
	Adhoc_MacToIp( adhoc_mac, (byte *)&sin->sin_addr.s_addr );
	return 0;
}

int PSP_NetGetHostName( char *buf, int len )
{
	byte	ip[4];

	if( !adhoc_active )
	{
		Q_strncpy( buf, "localhost", len );
		return -1;
	}

	Adhoc_MacToIp( adhoc_mac, ip );
	Q_snprintf( buf, len, "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3] );
	return 0;
}

unsigned long PSP_InetAddr( const char *s )
{
	byte	ip[4];
	int	i;

	for( i = 0; i < 4; i++ )
	{
		int	value = 0, digits = 0;

		while( *s >= '0' && *s <= '9' )
		{
			value = value * 10 + ( *s++ - '0' );
			digits++;
		}

		if( !digits || value > 255 )
			return 0xffffffffUL;

		ip[i] = value;

		if( i < 3 && *s++ != '.' )
			return 0xffffffffUL;
	}

	{
		// ip[] has no alignment guarantee, the MIPS CPU faults on an unaligned word read
		unsigned long	addr;
		memcpy( &addr, ip, sizeof( ip ));
		return addr;
	}
}

/*
==============
Adhoc_InitLibs

Network modules and libraries, once per run: they stay up after a failed
attempt and a retry only joins the group again.
==============
*/
static qboolean Adhoc_InitLibs( void )
{
	struct productStruct	product;
	int	ret;

	if( adhoc_libsready )
		return true;

	if(( ret = sceUtilityLoadNetModule( PSP_NET_MODULE_COMMON )) < 0 )
		Con_Printf( "S_PSP: adhoc load net common: 0x%08x\n", ret );
	if(( ret = sceUtilityLoadNetModule( PSP_NET_MODULE_ADHOC )) < 0 )
		Con_Printf( "S_PSP: adhoc load net adhoc: 0x%08x\n", ret );

	if(( ret = sceNetInit( 128 * 1024, 42, 4 * 1024, 42, 4 * 1024 )) < 0 )
	{
		Con_Printf( "S_PSP: adhoc sceNetInit failed: 0x%08x\n", ret );
		return false;
	}

	if(( ret = sceNetAdhocInit()) < 0 )
	{
		Con_Printf( "S_PSP: adhoc sceNetAdhocInit failed: 0x%08x\n", ret );
		return false;
	}

	// exactly nine characters, no terminator inside: Q_strncpy would keep
	// only eight and the ad-hoc server drops a login with an invalid code
	memset( &product, 0, sizeof( product ));
	memcpy( product.product, ADHOC_PRODUCT, sizeof( product.product ));

	if(( ret = sceNetAdhocctlInit( 0x2000, 0x30, &product )) < 0 )
	{
		Con_Printf( "S_PSP: adhoc sceNetAdhocctlInit failed: 0x%08x\n", ret );
		return false;
	}

	adhoc_libsready = true;
	return true;
}

// starts joining the group; the first PSP creates it, the others find it by name
static qboolean Adhoc_ConnectGroup( void )
{
	char	group[8];
	int	ret;

	Con_Printf( "S_PSP: adhoc starting\n" );

	if( !Adhoc_InitLibs( ))
		return false;

	memset( group, 0, sizeof( group ));
	memcpy( group, ADHOC_GROUP, Q_strlen( ADHOC_GROUP ));

	if(( ret = sceNetAdhocctlConnect( group )) < 0 )
	{
		Con_Printf( "S_PSP: adhoc connect failed: 0x%08x\n", ret );
		return false;
	}

	return true;
}

static qboolean Adhoc_GroupJoined( void )
{
	int	state = 0;

	return sceNetAdhocctlGetState( &state ) >= 0 && state == ADHOC_STATE_CONNECTED;
}

// in the group: addresses are known, sockets can open
static void Adhoc_Activate( int waited )
{
	byte	ip[4];

	sceNetGetLocalEtherAddr( adhoc_mac );
	adhoc_active = true;

	Adhoc_MacToIp( adhoc_mac, ip );
	Con_Printf( "S_PSP: adhoc connected after %i ms, this PSP is %i.%i.%i.%i\n", waited, ip[0], ip[1], ip[2], ip[3] );

	// sockets of an earlier game failed while ad-hoc was off, the next
	// NET_Config( true ) has to open them again
	NET_Config( false );
}

/*
==============
adhoc_host

Blocks until the group is joined (about a second), the map load follows
in the same command buffer.
==============
*/
static void Adhoc_Host_f( void )
{
	int	waited;

	adhoc_joiner = false;

	if( adhoc_active )
		return;

	if( !Adhoc_ConnectGroup( ))
	{
		Con_Printf( "S_PSP: adhoc_host failed, the game starts offline\n" );
		return;
	}

	for( waited = 0; waited < 15000; waited += 50 )
	{
		if( Adhoc_GroupJoined( ))
		{
			Adhoc_Activate( waited );
			return;
		}
		sceKernelDelayThread( 50 * 1000 );
	}

	Con_Printf( "S_PSP: adhoc group not joined after 15 s, the game starts offline\n" );
	sceNetAdhocctlDisconnect();	// so a retry can connect again
}

/*
==============
adhoc_join

Runs a frame at a time (Adhoc_Frame) so the connection screen can show
what is going on: join the group, then broadcast "ping" to the server port
from a spare port; the engine's server answers "ack" to whoever pings, so
the first answer is a game to connect to.
==============
*/
typedef enum
{
	JOIN_IDLE = 0,
	JOIN_GROUP,	// waiting to be in the ad-hoc group
	JOIN_SEARCH,	// pinging for a game
	JOIN_FAILED	// message on screen, then back to the menu
} adhoc_joinstate_t;

#define JOIN_GROUP_TIMEOUT	15.0
#define JOIN_SEARCH_TIMEOUT	15.0
#define JOIN_FAILED_SHOW	4.0

static adhoc_joinstate_t	join_state;
static double		join_start, join_lastping;
static int		join_pdp = -1;

static void Adhoc_JoinStatus( float progress, const char *text )
{
	Con_Printf( "S_PSP: adhoc: %s\n", text );
	Cbuf_AddText( va( "menu_connectionprogress stufftext %.2f \"%s\"\n", progress, text ));
}

static void Adhoc_JoinFail( const char *text )
{
	if( join_pdp >= 0 )
	{
		sceNetAdhocPdpDelete( join_pdp, 0 );
		join_pdp = -1;
	}

	Adhoc_JoinStatus( 0.0f, text );
	join_state = JOIN_FAILED;
	join_start = Sys_DoubleTime();
}

static void Adhoc_Join_f( void )
{
	if( join_state != JOIN_IDLE && join_state != JOIN_FAILED )
		return; // already searching

	// before the sockets open: a joiner binds no server port
	adhoc_joiner = true;

	// the connection screen, kept up while searching
	Cbuf_AddText( "menu_connectionprogress menu \"Ad-hoc\"\n" );

	join_start = Sys_DoubleTime();

	if( adhoc_active )
	{
		join_state = JOIN_SEARCH;
		join_lastping = 0.0;
		return;
	}

	if( !Adhoc_ConnectGroup( ))
	{
		Adhoc_JoinFail( "Ad-hoc could not start (see xash.log)" );
		return;
	}

	join_state = JOIN_GROUP;
	Adhoc_JoinStatus( 0.1f, "Joining the ad-hoc group..." );
}

static void Adhoc_JoinFrame( void )
{
	static const char	ping[] = "\xff\xff\xff\xffping";
	double		now = Sys_DoubleTime();
	double		elapsed = now - join_start;

	switch( join_state )
	{
	case JOIN_GROUP:
		if( Adhoc_GroupJoined( ))
		{
			Adhoc_Activate( (int)( elapsed * 1000.0 ));
			join_state = JOIN_SEARCH;
			join_start = now;
			join_lastping = 0.0;
		}
		else if( elapsed > JOIN_GROUP_TIMEOUT )
		{
			sceNetAdhocctlDisconnect();
			Adhoc_JoinFail( "No ad-hoc connection. In PPSSPP: networking on, ad-hoc server 127.0.0.1" );
		}
		break;

	case JOIN_SEARCH:
	{
		byte		buf[256], mac[6];
		unsigned short	port = 0;
		int		size = sizeof( buf );

		if( join_pdp < 0 )
		{
			join_pdp = sceNetAdhocPdpCreate( adhoc_mac, ADHOC_SEARCH_PORT, 0x1000, 0 );
			if( join_pdp < 0 )
			{
				Con_Printf( "S_PSP: adhoc search socket failed: 0x%08x\n", join_pdp );
				Adhoc_JoinFail( "Ad-hoc search failed (see xash.log)" );
				break;
			}
		}

		if( now - join_lastping >= 0.5 )
		{
			join_lastping = now;
			sceNetAdhocPdpSend( join_pdp, (unsigned char *)adhoc_broadcast, ADHOC_SERVER_PORT, (void *)ping, sizeof( ping ) - 1, 0, 1 );
			Adhoc_JoinStatus( 0.3f + 0.4f * (float)( elapsed / JOIN_SEARCH_TIMEOUT ),
				va( "Searching for a game... %i s", (int)elapsed ));
		}

		if( sceNetAdhocPdpRecv( join_pdp, mac, &port, buf, &size, 0, 1 ) >= 0 && size > 0 && port == ADHOC_SERVER_PORT )
		{
			byte	ip[4];

			sceNetAdhocPdpDelete( join_pdp, 0 );
			join_pdp = -1;
			join_state = JOIN_IDLE;

			Adhoc_AddPeer( mac );
			Adhoc_MacToIp( mac, ip );

			Adhoc_JoinStatus( 0.8f, va( "Game found at %i.%i.%i.%i, connecting...", ip[0], ip[1], ip[2], ip[3] ));
			Cbuf_AddText( va( "connect %i.%i.%i.%i:%i\n", ip[0], ip[1], ip[2], ip[3], ADHOC_SERVER_PORT ));
		}
		else if( elapsed > JOIN_SEARCH_TIMEOUT )
		{
			Adhoc_JoinFail( "No game found. Host one first: Counter-Strike, Ad-hoc, Start Game" );
		}
		break;
	}

	case JOIN_FAILED:
		// leave the message up for a moment, then back to the main menu
		if( elapsed > JOIN_FAILED_SHOW )
		{
			join_state = JOIN_IDLE;
			Cbuf_AddText( "menu_connectionprogress disconnect\n" );
		}
		break;

	default:
		break;
	}
}

// once per host frame
void PSP_NetFrame( void )
{
	if( join_state != JOIN_IDLE )
		Adhoc_JoinFrame();
}

void PSP_NetInit( void )
{
	Cmd_AddCommand( "adhoc_host", Adhoc_Host_f, "join the PSP ad-hoc group to host a game" );
	Cmd_AddCommand( "adhoc_join", Adhoc_Join_f, "join the PSP ad-hoc group and connect to the game found there" );
}
