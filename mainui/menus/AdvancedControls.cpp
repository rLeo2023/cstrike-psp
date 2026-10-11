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
#include "kbutton.h"
#include "MenuStrings.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "Slider.h"

#define ART_BANNER			"gfx/shell/head_advanced"

#ifndef MAINUI_PSP

class CAdvancedControls : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;
	CAdvancedControls() : CMenuFramework("CAdvancedControls") { }

	void ToggleLookCheckboxes( bool write );

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void SaveAndPopMenu() override;

	void GetConfig( void );
	void PitchInvert( void );

	CMenuPicButton done, inputDev;

	CMenuCheckBox	crosshair;
	CMenuCheckBox	invertMouse;
	CMenuCheckBox	mouseLook;
	CMenuCheckBox	lookSpring;
	CMenuCheckBox	lookStrafe;
	CMenuCheckBox	lookFilter;
	CMenuCheckBox	autoaim;
	CMenuSlider	sensitivity;
};

/*
=================
UI_AdvControls_GetConfig
=================
*/
void CAdvancedControls::GetConfig( )
{
	kbutton_t	*mlook;

	if( EngFuncs::GetCvarFloat( "m_pitch" ) < 0 )
		invertMouse.bChecked = true;

	mlook = (kbutton_s *)EngFuncs::KEY_GetState( "in_mlook" );
	if( mlook )
	{
		if( mlook && mlook->state & 1 )
			mouseLook.bChecked = true;
		else
			mouseLook.bChecked = false;
	}
	else
	{
		mouseLook.SetGrayed( true );
		mouseLook.bChecked = true;
	}

	crosshair.LinkCvar( "crosshair" );
	lookSpring.LinkCvar( "lookspring" );
	lookStrafe.LinkCvar( "lookstrafe" );
	lookFilter.LinkCvar( "look_filter" );

	autoaim.LinkCvar( "sv_aim" );
	sensitivity.LinkCvar( "sensitivity" );

	ToggleLookCheckboxes( false );
}

void CAdvancedControls::PitchInvert()
{
	bool invert = invertMouse.bChecked;
	float m_pitch = EngFuncs::GetCvarFloat( "m_pitch" );
	if( ( invert && (m_pitch > 0) ) ||
		( !invert && (m_pitch < 0) ) )
	{
		EngFuncs::CvarSetValue( "m_pitch", -m_pitch );
	}
}

void CAdvancedControls::ToggleLookCheckboxes( bool write )
{
	lookSpring.SetGrayed( mouseLook.bChecked );
	lookStrafe.SetGrayed( mouseLook.bChecked );

	if( write )
	{
		if( mouseLook.bChecked )
			EngFuncs::ClientCmd( FALSE, "+mlook\nbind _force_write\n" );
		else
			EngFuncs::ClientCmd( FALSE, "-mlook\nbind _force_write\n" );
	}
}

void CAdvancedControls::SaveAndPopMenu()
{
	crosshair.WriteCvar();
	lookSpring.WriteCvar();
	lookStrafe.WriteCvar();
	lookFilter.WriteCvar();
	if( EngFuncs::GetCvarString("m_filter")[0] )
		EngFuncs::CvarSetValue( "m_filter", lookFilter.bChecked );
	autoaim.WriteCvar();
	sensitivity.WriteCvar();

	ToggleLookCheckboxes( true );

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
UI_AdvControls_Init
=================
*/
void CAdvancedControls::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	done.SetNameAndStatus( L( "Done" ), L( "save changed and go back to the Customize Menu" ) );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CAdvancedControls::SaveAndPopMenu );
	done.SetCoord( 72, 680 );

	crosshair.SetNameAndStatus( L( "Crosshair" ), L( "Enable the weapon aiming crosshair" ) );
	crosshair.iFlags |= QMF_NOTIFY;
	crosshair.SetCoord( 72, 280 );

	invertMouse.SetNameAndStatus( L( "GameUI_ReverseMouse" ), L( "GameUI_ReverseMouseLabel" ) );
	invertMouse.iFlags |= QMF_NOTIFY;
	invertMouse.onChanged = VoidCb( &CAdvancedControls::PitchInvert );
	invertMouse.SetCoord( 72, 330 );

	mouseLook.SetNameAndStatus( L( "GameUI_MouseLook" ), L( "GameUI_MouseLookLabel" ) );
	mouseLook.iFlags |= QMF_NOTIFY;
	SET_EVENT( mouseLook.onChanged,
		((CAdvancedControls*)pSelf->Parent())->ToggleLookCheckboxes( true ) );
	mouseLook.SetCoord( 72, 380 );

	lookSpring.SetNameAndStatus( L( "Look spring" ), L( "Causes the screen to 'spring' back to looking straight ahead when you move forward" ) );
	lookSpring.iFlags |= QMF_NOTIFY;
	lookSpring.SetCoord( 72, 430 );

	lookStrafe.SetNameAndStatus( L( "Look strafe" ), L( "In combination with your mouse look modifier, causes left-right movements to strafe instead of turn" ) );
	lookStrafe.iFlags |= QMF_NOTIFY;
	lookStrafe.SetCoord( 72, 480 );

	lookFilter.SetNameAndStatus( L( "GameUI_MouseFilter" ), L( "GameUI_MouseFilterLabel" ) );
	lookFilter.iFlags |= QMF_NOTIFY;
	lookFilter.SetCoord( 72, 530 );

	autoaim.SetNameAndStatus( L( "GameUI_AutoAim" ), L( "GameUI_AutoaimLabel" ) );
	autoaim.iFlags |= QMF_NOTIFY;
	autoaim.SetCoord( 72, 580 );

	sensitivity.SetNameAndStatus( L( "GameUI_MouseSensitivity" ), L( "Set in-game mouse sensitivity" ) );
	sensitivity.Setup( 0.0, 20.0f, 0.1 );
	sensitivity.SetCoord( 72, 660 );

	inputDev.SetNameAndStatus( L( "Input devices" ), L( "Toggle mouse, touch controls" ) );
	inputDev.onReleased = UI_InputDevices_Menu;
	inputDev.iFlags |= QMF_NOTIFY;
	if( CL_IsActive() && !EngFuncs::GetCvarFloat( "host_serverstate" ))
		inputDev.SetGrayed( true );
	//inputDev.SetRect( 72, 230, UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );
	inputDev.SetCoord( 72, 230 );

	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( inputDev );
	AddItem( crosshair );
	AddItem( invertMouse );
	AddItem( mouseLook );
	AddItem( lookSpring );
	AddItem( lookStrafe );
	AddItem( lookFilter );
	AddItem( autoaim );
	AddItem( sensitivity );
}


void CAdvancedControls::_VidInit()
{
	GetConfig();
}

#else // MAINUI_PSP

// PSP: no mouse. The analog stick moves (joy_forward/joy_side, negative
// because the PSP reports the stick negated), the face buttons turn
// (cl_yawspeed/cl_pitchspeed, a negative pitch speed inverts), plus the
// client side aim assist. Replaces the separate Gamepad menu.
class CAdvancedControls : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;
	CAdvancedControls() : CMenuFramework("CAdvancedControls") { }

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void SaveAndPopMenu() override;

	void GetConfig( void );

	CMenuPicButton done;

	CMenuCheckBox	crosshair;
	CMenuCheckBox	autoaim;
	CMenuCheckBox	invertLook;
	CMenuSlider	autoaimSpeed;
	CMenuSlider	lookYaw;
	CMenuSlider	lookPitch;
	CMenuSlider	moveSens;
	CMenuSlider	deadzone;
};

void CAdvancedControls::GetConfig( void )
{
	float pitchSpeed = EngFuncs::GetCvarFloat( "cl_pitchspeed" );

	crosshair.LinkCvar( "crosshair" );
	autoaim.LinkCvar( "cl_autoaim" );
	autoaimSpeed.LinkCvar( "cl_autoaim_speed" );
	lookYaw.LinkCvar( "cl_yawspeed" );
	deadzone.LinkCvar( "psp_joy_dz_min" );

	invertLook.bChecked = pitchSpeed < 0.0f;
	lookPitch.SetCurrentValue( fabs( pitchSpeed ));
	moveSens.SetCurrentValue( fabs( EngFuncs::GetCvarFloat( "joy_forward" )));
}

void CAdvancedControls::SaveAndPopMenu()
{
	crosshair.WriteCvar();
	autoaim.WriteCvar();
	autoaimSpeed.WriteCvar();
	lookYaw.WriteCvar();
	deadzone.WriteCvar();

	float pitchSpeed = lookPitch.GetCurrentValue();
	EngFuncs::CvarSetValue( "cl_pitchspeed", invertLook.bChecked ? -pitchSpeed : pitchSpeed );

	float move = moveSens.GetCurrentValue();
	EngFuncs::CvarSetValue( "joy_forward", -move );
	EngFuncs::CvarSetValue( "joy_side", -move );

	CMenuFramework::SaveAndPopMenu();
}

void CAdvancedControls::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	done.SetNameAndStatus( L( "Done" ), L( "save changed and go back to the Customize Menu" ) );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CAdvancedControls::SaveAndPopMenu );
	done.SetCoord( 72, 680 );

	// left column: checkboxes
	crosshair.SetNameAndStatus( L( "Crosshair" ), L( "Enable the weapon aiming crosshair" ) );
	crosshair.SetCoord( 72, 240 );

	autoaim.SetNameAndStatus( L( "GameUI_AutoAim" ), L( "Pull the crosshair towards visible enemies close to it" ) );
	autoaim.SetCoord( 72, 300 );

	invertLook.SetNameAndStatus( L( "Invert look" ), L( "Triangle looks down, cross looks up" ) );
	invertLook.SetCoord( 72, 360 );

	autoaimSpeed.SetNameAndStatus( L( "Aim power" ), L( "How fast the crosshair is pulled towards the enemy" ) );
	autoaimSpeed.Setup( 20.0f, 150.0f, 10.0f );
	autoaimSpeed.SetCoord( 72, 470 );

	// right column: sliders
	lookYaw.SetNameAndStatus( L( "Turn speed" ), L( "Square / circle turn speed, degrees per second" ) );
	lookYaw.Setup( 60.0f, 360.0f, 10.0f );
	lookYaw.SetCoord( 560, 270 );

	lookPitch.SetNameAndStatus( L( "Look speed" ), L( "Triangle / cross look speed, degrees per second" ) );
	lookPitch.Setup( 60.0f, 300.0f, 10.0f );
	lookPitch.SetCoord( 560, 370 );

	moveSens.SetNameAndStatus( L( "Move speed" ), L( "Movement speed with the stick fully pushed" ) );
	moveSens.Setup( 0.2f, 1.0f, 0.1f );
	moveSens.SetCoord( 560, 470 );

	deadzone.SetNameAndStatus( L( "Deadzone" ), L( "Stick movement ignored around the center (0 - 127)" ) );
	deadzone.Setup( 0.0f, 60.0f, 1.0f );
	deadzone.SetCoord( 560, 570 );

	AddItem( background );
	AddItem( banner );
	AddItem( done );
	AddItem( crosshair );
	AddItem( autoaim );
	AddItem( invertLook );
	AddItem( autoaimSpeed );
	AddItem( lookYaw );
	AddItem( lookPitch );
	AddItem( moveSens );
	AddItem( deadzone );
}

void CAdvancedControls::_VidInit()
{
	GetConfig();
}

#endif // MAINUI_PSP

ADD_MENU( menu_advcontrols, CAdvancedControls, UI_AdvControls_Menu );
