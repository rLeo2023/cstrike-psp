/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "Field.h"
#include "keydefs.h"
#include "TabView.h"

#define ART_BANNER	     	"gfx/shell/head_config"

#ifdef MAINUI_PSP
static void UI_SaveSettings( void )
{
	EngFuncs::ClientCmd( FALSE, "trysaveconfig\n" );
	EngFuncs::PlayLocalSound( uiSoundLaunch );
}

/*
=================
CMenuPlayerName

the firmware keyboard needs the volatile memory the engine keeps locked, so
the name is typed with the engine's own on-screen keyboard: it shows up while
the field has focus (d-pad + cross, "en" or up past the top row to leave it)
=================
*/
class CMenuPlayerName : public CMenuFramework
{
public:
	CMenuPlayerName() : CMenuFramework( "CMenuPlayerName" ) { }

	void SaveAndPopMenu() override
	{
		if( UI::Names::CheckIsNameValid( name.GetBuffer() ))
		{
			name.WriteCvar();
			EngFuncs::ClientCmd( FALSE, "trysaveconfig\n" );
		}
		CMenuFramework::SaveAndPopMenu();
	}

	void Hide() override
	{
		// the field never gets QM_LOSTFOCUS when the whole menu closes,
		// so the keyboard would stay on screen over the next menu
		UI_EnableTextInput( false );
		CMenuFramework::Hide();
	}

private:
	void _Init( void ) override
	{
		banner.SetPicture( ART_BANNER );

		name.szName = L( "Name" );
		name.szStatusText = L( "Enter your multiplayer display name" );
		name.iMaxLength = 32;
		name.LinkCvar( "name" );
		// under the banner and Done, above the on-screen keyboard
		name.SetRect( 320, 310, 256, 36 );

		AddItem( background );
		AddItem( banner );
		AddButton( L( "Done" ), L( "Save the name and go back" ),
			PC_DONE, VoidCb( &CMenuPlayerName::SaveAndPopMenu ));
		AddItem( name );
	}

	CMenuField name;
};

ADD_MENU( menu_playername, CMenuPlayerName, UI_PlayerName_Menu );
#endif

class CMenuOptions: public CMenuFramework
{
private:
	void _Init( void ) override;

public:
	typedef CMenuFramework BaseClass;
	CMenuOptions() : CMenuFramework("CMenuOptions") { }

	// update dialog
	CMenuYesNoMessageBox msgBox;
};

/*
=================
CMenuOptions::Init
=================
*/
void CMenuOptions::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	msgBox.SetMessage( L( "Check the Internet for updates?" ) );
	SET_EVENT( msgBox.onPositive, UI_OpenUpdatePage( false, true ) );

	msgBox.Link( this );

	AddItem( background );
	AddItem( banner );
#ifdef MAINUI_PSP
	AddButton( L( "Player name" ), L( "Change your multiplayer display name" ),
		PC_CUSTOMIZE, UI_PlayerName_Menu, QMF_NOTIFY );
	AddButton( L( "Controls" ), L( "Look speed, analog stick, auto-aim and crosshair" ),
		PC_CONTROLS, UI_AdvControls_Menu, QMF_NOTIFY );
#else
	AddButton( L( "Controls" ), L( "Change keyboard and mouse settings" ),
		PC_CONTROLS, UI_Controls_Menu, QMF_NOTIFY );
#endif
	AddButton( L( "GameUI_Audio" ), L( "Change sound volume and quality" ),
		PC_AUDIO, UI_Audio_Menu, QMF_NOTIFY );
#ifdef MAINUI_PSP
	// the Video page only held a "Video options" button (no video modes
	// on PSP): open the options directly
	AddButton( L( "GameUI_Video" ), L( "Change screen size, gamma and image quality" ),
		PC_VIDEO, UI_VidOptions_Menu, QMF_NOTIFY );
#else
	AddButton( L( "GameUI_Video" ), L( "Change screen size, video mode and gamma" ),
		PC_VIDEO, UI_Video_Menu, QMF_NOTIFY );
#endif
	if( UI_IsXashFWGS() )
	{
#ifndef MAINUI_PSP
		AddButton( L( "Touch" ), L( "Change touch settings and buttons" ),
			PC_TOUCH, UI_Touch_Menu, QMF_NOTIFY );
		// on PSP the stick and look settings live in Controls -> Adv. Controls
		AddButton( L( "GameUI_Joystick" ), L( "Change gamepad axis and button settings" ),
			PC_GAMEPAD, UI_GamePad_Menu, QMF_NOTIFY );
#endif
	}
#ifndef MAINUI_PSP
	AddButton( L( "Update" ), L( "Check for updates" ),
		PC_UPDATE, msgBox.MakeOpenEvent(), QMF_NOTIFY );
#else
	// every settings page already saves on Done, this is the explicit way
	AddButton( L( "Save settings" ), L( "Save all settings now (controls, video, audio, auto-aim)" ),
		PC_SAVE_GAME, UI_SaveSettings, QMF_NOTIFY );
#endif
	AddButton( L( "Done" ), L( "Go back to the Main menu" ),
		PC_DONE, VoidCb( &CMenuOptions::Hide ), QMF_NOTIFY );
}

ADD_MENU( menu_options, CMenuOptions, UI_Options_Menu );
