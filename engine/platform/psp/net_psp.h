/*
net_psp.h - BSD socket subset over PSP ad-hoc (PDP)
Copyright (C) 2026 rLeo2023

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

Takes the place of platform/stub/net_stub.h in net_ws.c. The PSP has no
IP stack in ad-hoc mode, only datagrams between MAC addresses (PDP), so
every PSP gets a made-up address 10.x.y.z from the last three bytes of its
MAC and these calls translate. Sockets only work after "adhoc_host" or
"adhoc_join" started ad-hoc; until then socket() fails like the stub did,
so offline games behave exactly as before.
*/
#ifndef NET_PSP_H
#define NET_PSP_H

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define SOCKET int
typedef int WSAsize_t;
struct in_addr { unsigned long s_addr; };
struct sockaddr_in { short sin_family; unsigned short sin_port; struct in_addr sin_addr; };
struct sockaddr { short sa_family; int stub[32]; };
struct hostent { int h_addr_list[1]; };

#define AF_INET          2
#define PF_INET          2
#define SOCK_STREAM      1
#define SOCK_DGRAM       2
#define IPPROTO_TCP      6
#define IPPROTO_UDP      17
#define INADDR_ANY       0
#define INADDR_BROADCAST 0xffffffffUL

#define ntohs(n) ( (((n) & 0xFF00) >> 8) | (((n) & 0x00FF) << 8) )
#define htons(n) ( (((n) & 0xFF00) >> 8) | (((n) & 0x00FF) << 8) )
#define ntohl(n) ( (((n) & 0xFF000000) >> 24) | (((n) & 0x00FF0000) >> 8) \
	 | (((n) & 0x0000FF00) << 8) | (((n) & 0x000000FF) << 24) )
#define htonl(n) ( (((n) & 0xFF000000) >> 24) | (((n) & 0x00FF0000) >> 8) \
	 | (((n) & 0x0000FF00) << 8) | (((n) & 0x000000FF) << 24) )

extern int psp_net_errno;

int PSP_NetSocket( int type );
int PSP_NetBind( int s, const struct sockaddr *addr );
int PSP_NetSendTo( int s, const void *buf, int len, const struct sockaddr *to );
int PSP_NetRecvFrom( int s, void *buf, int len, struct sockaddr *from );
int PSP_NetClose( int s );
int PSP_NetGetSockName( int s, struct sockaddr *addr );
int PSP_NetGetHostName( char *buf, int len );
unsigned long PSP_InetAddr( const char *s );

#define socket( d, t, p )                 PSP_NetSocket( t )
#define bind( s, a, l )                   PSP_NetBind( s, (const struct sockaddr *)(a) )
#define sendto( s, b, l, f, t, tl )       PSP_NetSendTo( s, b, l, (const struct sockaddr *)(t) )
#define recvfrom( s, b, l, f, fr, fl )    PSP_NetRecvFrom( s, b, l, (struct sockaddr *)(fr) )
#define closesocket( s )                  PSP_NetClose( s )
#define getsockname( s, a, l )            PSP_NetGetSockName( s, (struct sockaddr *)(a) )
#define gethostname( b, l )               PSP_NetGetHostName( b, l )
#define inet_addr( s )                    PSP_InetAddr( s )

// options are irrelevant: PDP is always non blocking here and broadcast capable
#define ioctlsocket(...)    (0)
#define setsockopt(...)     (0)
#define gethostbyname(...)  NULL
#define connect(...)        (-1)
#define send(...)           (-1)
#define recv(...)           (-1)
#define select(...)         (-1)

#define WSAGetLastError()  (psp_net_errno)
#define WSAEINTR           1
#define WSAEBADF           2
#define WSAEACCES          3
#define WSAEFAULT          4
#define WSAEINVAL          5
#define WSAEMFILE          6
#define WSAEWOULDBLOCK     7
#define WSAEINPROGRESS     8
#define WSAEALREADY        9
#define WSAENOTSOCK        10
#define WSAEDESTADDRREQ    11
#define WSAEMSGSIZE        12
#define WSAEPROTOTYPE      13
#define WSAENOPROTOOPT     14
#define WSAEPROTONOSUPPORT 15
#define WSAESOCKTNOSUPPORT 16
#define WSAEOPNOTSUPP      17
#define WSAEPFNOSUPPORT    18
#define WSAEAFNOSUPPORT    19
#define WSAEADDRINUSE      20
#define WSAEADDRNOTAVAIL   21
#define WSAENETDOWN        22
#define WSAENETUNREACH     23
#define WSAENETRESET       24
#define WSAECONNABORTED    25
#define WSAECONNRESET      26
#define WSAENOBUFS         27
#define WSAEISCONN         28
#define WSAENOTCONN        29
#define WSAESHUTDOWN       30
#define WSAETOOMANYREFS    31
#define WSAETIMEDOUT       32
#define WSAECONNREFUSED    33
#define WSAELOOP           34
#define WSAENAMETOOLONG    35
#define WSAEHOSTDOWN       36

#endif // NET_PSP_H
