/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// menu.cpp
//
// generic menu handler
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "com_weapons.h"

#include <string.h>
#include <stdio.h>
#include "draw_util.h"
#include "strl.h"

//#include "vgui_TeamFortressViewport.h"

#define MAX_MENU_STRING	512

char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

int KB_ConvertString( char *in, char **ppout );

void Touch_CloseMenu()
{
	gMobileAPI.pfnTouchRemoveButton( "_menu_*" );
	gMobileAPI.pfnTouchSetClientOnly( 0 );
}

#ifdef __PSP__
#include "keydefs.h"

// slot number a menu line selects ("1. Terrorist Forces" -> 1, "0. Exit"
// -> 10), 0 for titles and blank lines; color escapes like \w are skipped
static int MenuLineSlot( const char *line )
{
	while( *line == ' ' || *line == '\t' || ( line[0] == '\\' && line[1] && line[1] != '\n' ))
		line += ( *line == '\\' ) ? 2 : 1;

	if( line[0] >= '0' && line[0] <= '9' && line[1] == '.' )
		return line[0] == '0' ? 10 : line[0] - '0';

	return 0;
}

// The main buy menu ("#Buy") uses slots 1-8 and 0, the client adds
// "9. Autobuy" to it and runs the autobuy command (autobuy.txt) itself
#define AUTOBUY_SLOT	9
static bool s_bMenuHasAutobuy;

static void AddAutobuyLine( char *menu, size_t size )
{
	const char line[] = "9. Autobuy\n";
	char *insert = NULL;
	char *p = menu;

	// after the last regular item, otherwise in front of "0. Exit"
	while( *p )
	{
		int slot = MenuLineSlot( p );
		char *next = strchr( p, '\n' );
		next = next ? next + 1 : p + strlen( p );

		if( slot >= 1 && slot <= 8 )
			insert = next;
		else if( slot == 10 && !insert )
			insert = p;

		p = next;
	}

	if( !insert || strlen( menu ) + sizeof( line ) > size )
		return;

	memmove( insert + sizeof( line ) - 1, insert, strlen( insert ) + 1 );
	memcpy( insert, line, sizeof( line ) - 1 );
}

void CHudMenu::ResetCursor( void )
{
	m_iCursorSlot = 0;
	MoveCursor( 1 );
}

// next valid slot in menu order (1..9, then 0 which is slot 10)
void CHudMenu::MoveCursor( int dir )
{
	int slot = m_iCursorSlot;

	for( int tries = 0; tries < 10; tries++ )
	{
		slot += dir;
		if( slot > 10 ) slot = 1;
		if( slot < 1 ) slot = 10;

		if( m_bitsValidSlots & ( 1 << ( slot - 1 )))
		{
			m_iCursorSlot = slot;
			return;
		}
	}
}

// returns true when the key was used by the menu
bool CHudMenu::HandleMenuKey( int keynum )
{
	if( !m_fMenuDisplayed || !m_bitsValidSlots )
		return false;

	switch( keynum )
	{
	case K_UPARROW:
		MoveCursor( -1 );
		return true;
	case K_DOWNARROW:
		MoveCursor( 1 );
		return true;
	case K_A_BUTTON: // cross
	case K_ENTER:
		if( m_iCursorSlot )
			SelectMenuItem( m_iCursorSlot );
		return true;
	case K_B_BUTTON: // circle
	case K_SPACE:
		// "0. Exit/Cancel" when the menu has one. Menus without it (team
		// and class selection) are a choice the server waits for: hiding
		// them left the player stuck with no menu, so they stay open
		if( m_bitsValidSlots & ( 1 << 9 ))
			SelectMenuItem( 10 );
		return true;
	}

	return false;
}
#endif

int CHudMenu :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( gHUD.m_Menu, ShowMenu );
	HOOK_MESSAGE( gHUD.m_Menu, VGUIMenu );
	HOOK_MESSAGE( gHUD.m_Menu, BuyClose );
	HOOK_MESSAGE( gHUD.m_Menu, AllowSpec );
	HOOK_COMMAND( gHUD.m_Menu, "client_buy_open", OldStyleMenuOpen );
	HOOK_COMMAND( gHUD.m_Menu, "client_buy_close", OldStyleMenuClose );
	HOOK_COMMAND( gHUD.m_Menu, "showvguimenu", ShowVGUIMenu );

	_extended_menus = CVAR_CREATE("_extended_menus", "1", FCVAR_ARCHIVE);

	InitHUDData();

	m_bAllowSpec = true; // by default, spectating is allowed

	return 1;
}

void CHudMenu :: InitHUDData( void )
{
	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
#ifdef __PSP__
	m_iCursorSlot = 0;
#endif
	Reset();
}

void CHudMenu :: Reset( void )
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = FALSE;
}

int CHudMenu :: VidInit( void )
{
	return 1;
}

int CHudMenu :: Draw( float flTime )
{
	// check for if menu is set to disappear
	if ( m_flShutoffTime > 0 )
	{
		if ( m_flShutoffTime <= gHUD.m_flTime )
		{  // times up, shutoff
			UserCmd_OldStyleMenuClose();
			return 1;
		}
	}

	// don't draw the menu if the scoreboard is being shown
	//if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		//return 1;

	// draw the menu, along the left-hand side of the screen

	// count the number of newlines
	int nlc = 0;
	int i;
	for ( i = 0; i < MAX_MENU_STRING && g_szMenuString[i] != '\0'; i++ )
	{
		if ( g_szMenuString[i] == '\n' )
			nlc++;
	}

#ifdef __PSP__
	// The stock centering assumes 12 pixel lines but draws 24 apart, so on
	// the short PSP screen long menus ran off the bottom ("0. Exit" of the
	// buy menu was out of sight): tighter lines, centered on their real
	// height, never above the top
	const int lineHeight = 18;
	int y = ( ScreenHeight - ( nlc + 1 ) * lineHeight ) / 2;
	if( y < 4 ) y = 4;
#else
	// center it
	int y = (ScreenHeight/2) - ((nlc/2)*12) - 40; // make sure it is above the say text
	const int lineHeight = 24;
#endif
	int x = 20;

	i = 0;
	while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' )
	{
#ifdef __PSP__
		if( m_iCursorSlot && MenuLineSlot( g_szMenuString + i ) == m_iCursorSlot )
			DrawUtils::DrawHudString( x - 14, y, 320, ">", 255, 210, 64 );
#endif
		DrawUtils::DrawHudString( x, y, 320, g_szMenuString + i, 255, 255, 255 );
		y += lineHeight;

		while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' && g_szMenuString[i] != '\n' )
			i++;
		if ( g_szMenuString[i] == '\n' )
			i++;
	}

	return 1;
}

// selects an item from the menu
void CHudMenu :: SelectMenuItem( int menu_item )
{
	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ( (menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item-1))) )
	{
		char szbuf[32];
#ifdef __PSP__
		if( s_bMenuHasAutobuy && menu_item == AUTOBUY_SLOT )
		{
			// close the server side menu, then buy from autobuy.txt
			ClientCmd( "menuselect 10\n" );
			ClientCmd( "autobuy\n" );
			UserCmd_OldStyleMenuClose();
			return;
		}
#endif
		sprintf( szbuf, "menuselect %d\n", menu_item );
		ClientCmd( szbuf );

		UserCmd_OldStyleMenuClose();
	}
}


// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, FALSE if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudMenu :: MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf )
{
	char *temp = NULL, *menustring;

	BufferReader reader( pszName, pbuf, iSize );

	m_bitsValidSlots = reader.ReadShort();
	int DisplayTime = reader.ReadChar();
	int NeedMore = reader.ReadByte();

	if ( DisplayTime > 0 )
		m_flShutoffTime = DisplayTime + gHUD.m_flTime;
	else
		m_flShutoffTime = -1;

	if ( !m_bitsValidSlots )
	{
		UserCmd_OldStyleMenuClose(); // no valid slots means that the menu should be turned off
		return 1;
	}

	menustring = reader.ReadString();

	// menu will be replaced by scripted touch config
	// so execute it and exit
	if( _extended_menus->value != 0.0f )
	{
		if( !strncmp(menustring, "#Radio", 6 ) )
		{
			if( menustring[6] == 'A' )
			{
				ShowVGUIMenu(MENU_RADIOA); return 1;
			}
			else if( menustring[6] == 'B' )
			{
				ShowVGUIMenu(MENU_RADIOB); return 1;
			}
			else if( menustring[6] == 'C' )
			{
				ShowVGUIMenu(MENU_RADIOC); return 1;
			}
			else ShowVGUIMenu( MENU_NUMERICAL_MENU ); // we just show touch screen numbers
		}
		else ShowVGUIMenu(MENU_NUMERICAL_MENU);
	}
	else ShowVGUIMenu(MENU_NUMERICAL_MENU);

	if ( !m_fWaitingForMore ) // this is the start of a new menu
	{
		strlcpy( g_szPrelocalisedMenuString, menustring, sizeof( g_szPrelocalisedMenuString ) );
	}
	else
	{  // append to the current menu string
		strlcat( g_szPrelocalisedMenuString, menustring, sizeof( g_szPrelocalisedMenuString ) );
	}

	if ( !NeedMore )
	{  // we have the whole string, so we can localise it now
		strlcpy( g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString( g_szPrelocalisedMenuString ), sizeof( g_szMenuString ) );
		// Swap in characters
		if ( KB_ConvertString( g_szMenuString, &temp ) )
		{
			strlcpy( g_szMenuString, temp, sizeof( g_szMenuString ) );
			free( temp );
		}
#ifdef __PSP__
		s_bMenuHasAutobuy = !strcmp( g_szPrelocalisedMenuString, "#Buy" ) && !( m_bitsValidSlots & ( 1 << ( AUTOBUY_SLOT - 1 )));
		if( s_bMenuHasAutobuy )
		{
			AddAutobuyLine( g_szMenuString, sizeof( g_szMenuString ));
			m_bitsValidSlots |= 1 << ( AUTOBUY_SLOT - 1 );
		}
		ResetCursor();
#endif
	}

	m_fMenuDisplayed = 1;
	m_iFlags |= HUD_DRAW;

	m_fWaitingForMore = NeedMore;

	return 1;
}

int CHudMenu::MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int menuType = reader.ReadByte();
	m_bitsValidSlots = reader.ReadShort(); // is ignored

	ShowVGUIMenu(menuType);
	return 1;
}

int CHudMenu::MsgFunc_BuyClose(const char *pszName, int iSize, void *pbuf)
{
	Touch_CloseMenu();

	if (g_pMenu)
		g_pMenu->HideVGUIMenu();

	return 1;
}

int CHudMenu::MsgFunc_AllowSpec(const char *pszName, int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	m_bAllowSpec = (bool)reader.ReadByte();

	return 1;
}

void CHudMenu::UserCmd_OldStyleMenuOpen()
{
	m_flShutoffTime = -1; // stay open until user will not close it
	strlcpy( g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString("Buy"), sizeof( g_szMenuString ) );
}

void CHudMenu::UserCmd_OldStyleMenuClose()
{
	m_fMenuDisplayed = 0; // no valid slots means that the menu should be turned off
	m_iFlags &= ~HUD_DRAW;

	Touch_CloseMenu();
}

// lol, no real VGUI here
// it's really good only for touchscreen

void CHudMenu::ShowVGUIMenu( int menuType )
{
	int team = g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber;
	int haveCancel = team != TEAM_UNASSIGNED ? 1 : 0;

	if( g_pMenu && !UseOldTouchMenusEnabled() )
	{
		switch( menuType )
		{
		case MENU_TEAM:
		{
			int param = 0;
			if( m_bAllowSpec )
			{
				if( g_iTeamNumber == TEAM_UNASSIGNED ||
					g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].dead ||
					!g_iFreezeTimeOver )
				{
					param |= 1 << 0;
				}
			}

			if( IsASMapType() && g_iTeamNumber == TEAM_CT )
				param |= 1 << 1;

			if( haveCancel )
				param |= 1 << 2;

			g_pMenu->ShowVGUIMenu( menuType, param, 0 );
			return;
		}
		case MENU_CLASS_T:
		case MENU_CLASS_CT:
			g_pMenu->ShowVGUIMenu( menuType, gHUD.GetGameType() == GAME_CZERO, haveCancel );
			return;
		case MENU_BUY:
		case MENU_BUY_PISTOL:
		case MENU_BUY_SHOTGUN:
		case MENU_BUY_RIFLE:
		case MENU_BUY_SUBMACHINEGUN:
		case MENU_BUY_MACHINEGUN:
		case MENU_BUY_ITEM:
			g_pMenu->ShowVGUIMenu( menuType, gHUD.GetGameType() == GAME_CZERO, team );
			return;
		}
	}

	const char *szCmd;

	switch(menuType)
	{
	case MENU_TEAM:
		szCmd = "exec touch/chooseteam.cfg";
		break;
	case MENU_CLASS_T:
		szCmd = "exec touch/chooseteam_tr.cfg";
		break;
	case MENU_CLASS_CT:
		szCmd = "exec touch/chooseteam_ct.cfg";
		break;
	case MENU_BUY:
		szCmd = "exec touch/buy.cfg";
		break;
	case MENU_BUY_PISTOL:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
			szCmd = "exec touch/buy_pistol_t.cfg";
		else szCmd = "exec touch/buy_pistol_ct.cfg";
		break;
	case MENU_BUY_SHOTGUN:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
			szCmd = "exec touch/buy_shotgun_t.cfg";
		else szCmd = "exec touch/buy_shotgun_ct.cfg";
		break;
	case MENU_BUY_RIFLE:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
			szCmd = "exec touch/buy_rifle_t.cfg";
		else szCmd ="exec touch/buy_rifle_ct.cfg";
		break;
	case MENU_BUY_SUBMACHINEGUN:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
			szCmd = "exec touch/buy_submachinegun_t.cfg";
		else szCmd = "exec touch/buy_submachinegun_ct.cfg";
		break;
	case MENU_BUY_MACHINEGUN:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
			szCmd = "exec touch/buy_machinegun_t.cfg";
		else szCmd = "exec touch/buy_machinegun_ct.cfg";
		break;
	case MENU_BUY_ITEM:
		if( g_PlayerExtraInfo[gHUD.m_Scoreboard.m_iPlayerNum].teamnumber == TEAM_TERRORIST )
			szCmd = "exec touch/buy_item_t.cfg";
		else szCmd = "exec touch/buy_item_ct.cfg";
		break;
	case MENU_RADIOA:
		szCmd = "exec touch/radioa.cfg";
		break;
	case MENU_RADIOB:
		szCmd = "exec touch/radiob.cfg";
		break;
	case MENU_RADIOC:
		szCmd = "exec touch/radioc.cfg";
		break;
	case MENU_RADIOSELECTOR:
		szCmd = "exec touch/radioselector.cfg";
		break;
	case MENU_BUY_CSDM:
		szCmd = "exec touch/custom/dm_menu.cfg";
		break;
	case MENU_NUMERICAL_MENU:
#ifdef __ANDROID__
		szCmd = "exec touch/numerical_menu.cfg";
		break;
#else
		return;
#endif
	default:
		UserCmd_OldStyleMenuClose();
		return;
	}

	m_fMenuDisplayed = 1;
	ClientCmd(szCmd);
}

void CHudMenu::UserCmd_ShowVGUIMenu()
{
	if( gEngfuncs.Cmd_Argc() < 2 )
	{
		ConsolePrint("usage: showvguimenu <menuType>\n");
		return;
	}

	int menuType = atoi(gEngfuncs.Cmd_Argv(1));
	ShowVGUIMenu(menuType);
}
