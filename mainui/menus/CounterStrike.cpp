/*
Counter-Strike menu.

Reproduces the stock CS 1.6 "new game" flow, in the part the engine owns:
pick a map, decide whether to play with bots and how many, start a listen
server.

Team selection is deliberately NOT here. In the original game the map loads
first and the server then shows the Terrorist / Counter-Terrorist / Spectator
screen (titles.txt, "Team_Select"). That screen is driven by the game library,
so putting team buttons in this menu would both duplicate it and fight with it.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/
#include "Framework.h"
#include "keydefs.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Table.h"
#include "Slider.h"
#include "CheckBox.h"
#include "YesNoMessageBox.h"
#include "MenuStrings.h"

#define ART_BANNER		"gfx/shell/head_creategame"

// upper bound for the bot count slider
#define MAX_BOTS		15
// with "Ad-hoc" checked
#define MAX_BOTS_ADHOC		6

class CMenuCounterStrike;

class CMenuCSMapListModel : public CMenuBaseModel
{
public:
	CMenuCSMapListModel( CMenuCounterStrike *parent ) : parent( parent ) { }

	void Update() override;
	// map name only: most cstrike maps have no title anyway ("No Title"),
	// and a second column halves the width on a 480 pixel screen
	int GetColumns() const override { return 1; }
	int GetRows() const override { return m_iNumItems; }
	const char *GetCellText( int line, int column ) override
	{
		return column == 0 ? mapName[line] : NULL;
	}

	char		mapName[UI_MAXGAMES][64];
	char		mapsDescription[UI_MAXGAMES][64];
	int	m_iNumItems;
	CMenuCounterStrike *parent;
};

class CMenuCounterStrike : public CMenuFramework
{
public:
	CMenuCounterStrike() : CMenuFramework( "CMenuCounterStrike" ), mapsListModel( this ) { }
	static void StartGameCb( CMenuBaseItem *pSelf, void *pExtra );
	static void ToggleBotsCb( CMenuBaseItem *pSelf, void *pExtra );

	CMenuTable	mapsList;
	CMenuCSMapListModel mapsListModel;

	// bot count, zero means "no bots" which is how the original menu
	// expresses it too: the checkbox off is just a zero count
	CMenuSlider botCount;

	// bot difficulty, a pair of radio buttons: the server copies
	// cs_menu_botdiff to bot_difficulty when the map starts (0 easy, 1 normal)
	CMenuCheckBox diffEasy, diffMedium;
	void SetDifficulty( int diff );

	// host over ad-hoc: one more player slot, another PSP joins with the
	// main menu's "Join Ad-hoc"
	CMenuCheckBox adhoc;

	// the hosting PSP runs the server, the bots and its own game: with
	// another player connected 6 bots (4 vs 4) is about what it carries
	int MaxBots() const { return adhoc.bChecked ? MAX_BOTS_ADHOC : MAX_BOTS; }
	void ApplyBotLimit()
	{
		botCount.Setup( 0, MaxBots(), 1.0f );
		if( botCount.GetCurrentValue() > MaxBots())
		{
			botCount.SetCurrentValue( MaxBots());
			botCount.WriteCvar(); // the server reads the count from cs_menu_botcount
		}
	}

	CMenuPicButton *done;
	CMenuPicButton *botsToggle;

	CMenuYesNoMessageBox msgBox;

private:
	void _Init() override;
	void _VidInit() override;
};

/*
=================
CMenuCounterStrike::ToggleBotsCb
=================
Left arrow removes a bot, right arrow adds one, like the +/- control the
original menu uses. Zero bots means a solo game.
*/
void CMenuCounterStrike::ToggleBotsCb( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuCounterStrike *menu = (CMenuCounterStrike *)pSelf->Parent();
	float value = menu->botCount.GetCurrentValue() + (pExtra ? 1.0f : -1.0f);

	if( value < 0 ) value = 0;
	if( value > menu->MaxBots()) value = menu->MaxBots();

	menu->botCount.SetCurrentValue( value );
	menu->botCount.WriteCvar();
}

void CMenuCounterStrike::SetDifficulty( int diff )
{
	diffEasy.bChecked = ( diff == 0 );
	diffMedium.bChecked = ( diff != 0 );
	EngFuncs::CvarSetValue( "cs_menu_botdiff", diff ? 1.0f : 0.0f );
}

/*
=================
CMenuCSMapListModel::Update
=================
Reads the maps.lst the engine generated, which already excludes maps with
no spawn points, so anything listed here is playable.
*/
void CMenuCSMapListModel::Update( void )
{
	char *afile;

	if( !uiStatic.needMapListUpdate )
		return;

	// Do NOT ask for a refresh here. That rescans every .bsp, opening each
	// one and parsing its entity lump, which is fine on a desktop and
	// freezes the handheld. A cstrike install already ships maps.lst, so
	// just read it; only generate it when missing.
	afile = (char *)EngFuncs::COM_LoadFile( "maps.lst", NULL );

	if( !afile )
	{
		Con_Printf( "S_PSP: maps.lst missing, generating\n" );
		if( !EngFuncs::CreateMapsList( FALSE ) )
			afile = (char *)EngFuncs::COM_LoadFile( "maps.lst", NULL );
	}

	if( !afile )
	{
		parent->done->SetGrayed( true );
		m_iNumItems = 0;
		Con_Printf( "S_PSP: can't open maps.lst\n" );
		return;
	}

	char *pfile = afile;
	char token[1024];
	int numMaps = 0;

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token )) != NULL )
	{
		if( numMaps >= UI_MAXGAMES ) break;

		Q_strncpy( mapName[numMaps], token, 64 );
		if(( pfile = EngFuncs::COM_ParseFile( pfile, token )) == NULL )
		{
			Q_strncpy( mapsDescription[numMaps], mapName[numMaps], 64 );
			numMaps++;
			break; // unexpected end of file
		}
		Q_strncpy( mapsDescription[numMaps], token, 64 );
		numMaps++;
	}

	if( !numMaps )
	{
		parent->done->SetGrayed( true );
		Con_Printf( "S_PSP: no maps found, is this a cstrike install?\n" );
	}
	else
	{
		// we have maps, so the Start button must become clickable again
		parent->done->SetGrayed( false );
	}

	m_iNumItems = numMaps;
	EngFuncs::COM_FreeFile( afile );
	uiStatic.needMapListUpdate = false;
}

/*
=================
CMenuCounterStrike::StartGameCb
=================
Starts a listen server on the selected map with bot_quota bots. Team choice
is left to the server, which shows its select screen once we are connected.
*/
void CMenuCounterStrike::StartGameCb( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuCounterStrike *menu = (CMenuCounterStrike*)pSelf->Parent();
	int item = menu->mapsList.GetCurrentIndex();

	if( item < 0 || item >= menu->mapsListModel.GetRows() )
		return;

	const char *mapName = menu->mapsListModel.mapName[item];

	if( !EngFuncs::IsMapValid( mapName ))
		return;	// bad map

	if( EngFuncs::GetCvarFloat( "host_serverstate" ) )
	{
		if(	EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
			EngFuncs::HostEndGame( "end of the game" );
		else
			EngFuncs::HostEndGame( "starting new server" );
	}

	menu->ApplyBotLimit();
	int bots = (int)menu->botCount.GetCurrentValue();
	// leave room for the human player. Never 1: with maxplayers 1 the engine
	// forces deathmatch 0 and the game library installs the training rules
	// (tr_ maps: no teams, looks for info_player_start and finds none).
	int players = bots + 1;
	bool adhoc = menu->adhoc.bChecked;
	if( adhoc )
		players++;	// the PSP that joins
	if( players < 2 )
		players = 2;

	EngFuncs::CvarSetValue( "deathmatch", 1.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", players );

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	char cmd2[256];
	Com_EscapeCommand( cmd2, mapName, 256 );

	// same "wait a few frames" dance as the deathmatch menu, the old server
	// has to finish shutting down before maxplayers sticks.
	// bot_quota/bot_difficulty are registered by the game library, which is
	// only loaded by "map", so they have to come after it in the buffer.
	char cmd[1024];
	// adhoc_host joins the ad-hoc group first (blocks a few seconds), the
	// listen server then opens its port over ad-hoc
	sprintf( cmd, "%sendgame;menu_connectionprogress localserver;wait;wait;wait;maxplayers %i;latch;map %s;bot_difficulty %i;bot_quota %i\n",
		adhoc ? "adhoc_host;" : "", players, cmd2, (int)EngFuncs::GetCvarFloat( "cs_menu_botdiff" ), bots );
	EngFuncs::ClientCmd( FALSE, cmd );
}

/*
=================
CMenuCounterStrike::Init
=================
*/
void CMenuCounterStrike::_Init( void )
{
	uiStatic.needMapListUpdate = true;


	// register before linking, otherwise WriteCvar() finds nothing
	EngFuncs::CvarRegister( "cs_menu_botcount", "0", FCVAR_ARCHIVE );

	AddItem( background );
	AddItem( banner );
	banner.SetPicture( ART_BANNER );

	done = AddButton( "Start Game", "Start the game on the selected map", PC_DONE, StartGameCb );
	done->onReleasedClActive = msgBox.MakeOpenEvent();
	done->SetGrayed( true );	// cleared by the map list model once maps load

	mapsList.SetCharSize( QM_SMALLFONT );
	mapsList.SetupColumn( 0, "Map", 1.0f );
	mapsList.SetModel( &mapsListModel );

	// items that are not registered here are never drawn or focused
	AddItem( mapsList );

	// no status text: the slider draws it to its right, across the map list
	botCount.SetNameAndStatus( "Bots", NULL );
	botCount.Setup( 0, MAX_BOTS, 1.0f );
	botCount.SetCurrentValue( 0 );
	botCount.LinkCvar( "cs_menu_botcount" );
	AddItem( botCount );

	EngFuncs::CvarRegister( "cs_menu_botdiff", "1", FCVAR_ARCHIVE );

	diffEasy.SetNameAndStatus( "Easy", "Easy bots: slower reactions, worse aim" );
	SET_EVENT( diffEasy.onChanged, ((CMenuCounterStrike *)pSelf->Parent())->SetDifficulty( 0 ));
	AddItem( diffEasy );

	diffMedium.SetNameAndStatus( "Medium", "Normal bots" );
	SET_EVENT( diffMedium.onChanged, ((CMenuCounterStrike *)pSelf->Parent())->SetDifficulty( 1 ));
	AddItem( diffMedium );

	adhoc.SetNameAndStatus( "Ad-hoc", "Let another PSP join this game (Join Ad-hoc in its main menu). Up to 6 bots" );
	SET_EVENT( adhoc.onChanged, ((CMenuCounterStrike *)pSelf->Parent())->ApplyBotLimit() );
	AddItem( adhoc );

	// Both controls need an explicit rect, the framework only places the
	// AddButton ones. Units are MainUI's virtual 1024x768 space; on PSP the
	// scale is 272/768 (~0.35), so the visible width is ~1355 units.
	// The list takes the right side, under the banner (which ends ~190).
	// The slider goes under "Start Game" (72,230) instead of below the list:
	// its label is drawn 1.5 font heights above it and ran into the list.
	mapsList.SetRect( 340, 210, 880, 440 );
	botCount.SetRect( 72, 380, 230, 30 );
	diffEasy.SetCoord( 72, 450 );
	diffMedium.SetCoord( 72, 510 );
	adhoc.SetCoord( 72, 590 );

	msgBox.SetMessage( "Start the selected map?" );
	msgBox.HighlightChoice( CMenuYesNoMessageBox::HIGHLIGHT_YES );
	msgBox.Link( this );
}

void CMenuCounterStrike::_VidInit( void )
{
	uiStatic.needMapListUpdate = true;
	SetDifficulty( (int)EngFuncs::GetCvarFloat( "cs_menu_botdiff" ));
}

ADD_MENU( menu_counterstrike, CMenuCounterStrike, UI_CounterStrike_Menu );