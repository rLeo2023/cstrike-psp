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
#include "Slider.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "StringArrayModel.h"
#include "Action.h"

#define ART_BANNER			"gfx/shell/head_gamepad"

enum engineAxis_t
{
	JOY_AXIS_SIDE = 0,
	JOY_AXIS_FWD,
	JOY_AXIS_PITCH,
	JOY_AXIS_YAW,
	JOY_AXIS_RT,
	JOY_AXIS_LT,
	JOY_AXIS_NULL
};

static const char *axisNames[7] =
{
	"Side",
	"Forward",
	"Yaw",
	"Pitch",
	"Right Trigger",
	"Left Trigger",
	"NOT BOUND"
};

class CMenuGamePad : public CMenuFramework
{
public:
	CMenuGamePad() : CMenuFramework("CMenuGamePad") { }

private:
	void _Init() override;
	void _VidInit() override;
	void GetConfig();
	void SaveAndPopMenu() override;

#ifdef MAINUI_PSP
	CMenuSlider joyDzMin, joyDzMax, joyCvPower, joyCvExpo;
#endif
	CMenuSlider side, forward, pitch, yaw;
	CMenuCheckBox invSide, invFwd, invPitch, invYaw;

	CMenuSpinControl axisBind[6];

	CMenuAction axisBind_label;
};

/*
=================
CMenuGamePad::GetConfig
=================
*/
void CMenuGamePad::GetConfig( void )
{
	float _side, _forward, _pitch, _yaw;
	char binding[7] = { 0 };

#ifdef MAINUI_PSP
	joyDzMin.SetCurrentValue( EngFuncs::GetCvarFloat( "psp_joy_dz_min" ));
	joyDzMax.SetCurrentValue( EngFuncs::GetCvarFloat( "psp_joy_dz_max" ));
	joyCvPower.SetCurrentValue( EngFuncs::GetCvarFloat( "psp_joy_cv_power" ));
	joyCvExpo.SetCurrentValue( EngFuncs::GetCvarFloat( "psp_joy_cv_expo" ));
#endif

	Q_strncpy( binding, EngFuncs::GetCvarString( "joy_axis_binding"), sizeof( binding ));

	_side = EngFuncs::GetCvarFloat( "joy_side" );
	_forward = EngFuncs::GetCvarFloat( "joy_forward" );
	_pitch = EngFuncs::GetCvarFloat( "joy_pitch" );
	_yaw = EngFuncs::GetCvarFloat( "joy_yaw" );

	side.SetCurrentValue( fabs( _side ) );
	forward.SetCurrentValue( fabs( _forward ));
	pitch.SetCurrentValue( fabs( _pitch ));
	yaw.SetCurrentValue( fabs( _yaw ));

	invSide.bChecked = _side < 0.0f ? true: false;
	invFwd.bChecked = _forward < 0.0f ? true: false;
	invPitch.bChecked = _pitch < 0.0f ? true: false;
	invYaw.bChecked = _yaw < 0.0f ? true: false;

	// I made a monster...
	for( unsigned int i = 0; i < sizeof( binding ) - 1; i++ )
	{
		switch( binding[i] )
		{
		case 's':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_SIDE] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_SIDE );
			break;
		case 'f':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_FWD] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_FWD );
			break;
		case 'p':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_PITCH] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_PITCH );
			break;
		case 'y':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_YAW] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_YAW );
			break;
		case 'r':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_RT] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_RT );
			break;
		case 'l':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_LT] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_LT );
			break;
		default:
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_NULL] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_NULL );
		}
	}
}

/*
=================
CMenuGamePad::SetConfig
=================
*/
void CMenuGamePad::SaveAndPopMenu()
{
	float _side, _forward, _pitch, _yaw;
	char binding[7] = { 0 };

#ifdef MAINUI_PSP
	EngFuncs::CvarSetValue( "psp_joy_dz_min", joyDzMin.GetCurrentValue());
	EngFuncs::CvarSetValue( "psp_joy_dz_max", joyDzMax.GetCurrentValue());
	EngFuncs::CvarSetValue( "psp_joy_cv_power", joyCvPower.GetCurrentValue());
	EngFuncs::CvarSetValue( "psp_joy_cv_expo", joyCvExpo.GetCurrentValue());
#endif

	_side = side.GetCurrentValue();
	if( invSide.bChecked )
		_side *= -1;

	_forward = forward.GetCurrentValue();
	if( invFwd.bChecked )
		_forward *= -1;

	_pitch = pitch.GetCurrentValue();
	if( invPitch.bChecked )
		_pitch *= -1;

	_yaw = yaw.GetCurrentValue();
	if( invYaw.bChecked )
		_yaw *= -1;

	for( int i = 0; i < 6; i++ )
	{
		switch( (int)axisBind[i].GetCurrentValue() )
		{
		case JOY_AXIS_SIDE: binding[i]  = 's'; break;
		case JOY_AXIS_FWD: binding[i]   = 'f'; break;
		case JOY_AXIS_PITCH: binding[i] = 'p'; break;
		case JOY_AXIS_YAW: binding[i]   = 'y'; break;
		case JOY_AXIS_RT: binding[i]    = 'r'; break;
		case JOY_AXIS_LT: binding[i]    = 'l'; break;
		default: binding[i] = '0'; break;
		}
	}

	EngFuncs::CvarSetValue( "joy_side", _side );
	EngFuncs::CvarSetValue( "joy_forward", _forward );
	EngFuncs::CvarSetValue( "joy_pitch", _pitch );
	EngFuncs::CvarSetValue( "joy_yaw", _yaw );
	EngFuncs::CvarSetString( "joy_axis_binding", binding );

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuGamePad::Init
=================
*/
void CMenuGamePad::_Init( void )
{
	int i, y;

	static CStringArrayModel model( axisNames, V_ARRAYSIZE( axisNames ) );

	banner.SetPicture( ART_BANNER );

	axisBind_label.eTextAlignment = QM_CENTER;
	axisBind_label.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	axisBind_label.colorBase = uiColorHelp;
	axisBind_label.szName = L( "Axis binding map" );

#ifdef MAINUI_PSP
	axisBind[2].szStatusText = L( "Set axis binding X" );
	axisBind[2].Setup( &model );

	axisBind[3].szStatusText = L( "Set axis binding Y" );
	axisBind[3].Setup( &model );
#else
	for( i = 0, y = 230; i < 6; i++, y += 50 )
	{
		axisBind[i].szStatusText = L( "Set axis binding" );
		axisBind[i].Setup( &model );
	}
#endif

	side.Setup( 0.0f, 1.0f, 0.1f );
	side.SetNameAndStatus( L( "Side" ), L( "Side movement sensitity" ) );
	invSide.SetNameAndStatus( L( "Invert" ), L( "Invert side movement axis" ) );

	forward.Setup( 0.0f, 1.0f, 0.1f );
	forward.SetNameAndStatus( L( "Forward" ), L( "Forward movement sensitivity" ) );
	invFwd.SetNameAndStatus( L( "Invert" ), L( "Invert forward movement axis" ) );

#ifdef MAINUI_PSP
	pitch.Setup( 0.0f, 200.0f, 2.0f );
	pitch.SetNameAndStatus( L( "Look X" ), L( "Horizontal look sensitivity" ) );
	invPitch.SetNameAndStatus( L( "Invert" ), L( "Invert pitch axis" ) );

	yaw.Setup( 0.0f, 200.0f, 2.0f );
	yaw.SetNameAndStatus( L( "Look Y" ), L( "Yaw rotating sensitivity" ) );
	invYaw.SetNameAndStatus( L( "Invert" ), L( "Invert yaw axis" ) );

	joyDzMin.Setup( 0.0f, 127.0f, 1.0f );
	joyDzMin.SetNameAndStatus( L( "Joy DZ min" ), L( "Joystick deadzone min" ) );

	joyDzMax.Setup( 0.0f, 127.0f, 1.0f );
	joyDzMax.SetNameAndStatus( L( "Joy DZ max" ), L( "Joystick deadzone max" ) );

	joyCvPower.Setup( 0.0f, 10.0f, 1.0f );
	joyCvPower.SetNameAndStatus( L( "Joy CV power" ), L( "Joystick curve power" ) );

	joyCvExpo.Setup( 0.0f, 1.0f, 0.1f );
	joyCvExpo.SetNameAndStatus( L( "Joy CV expo" ), L( "Joystick curve expo" ) );
#else
	pitch.Setup( 0.0f, 200.0f, 0.1f );
	pitch.SetNameAndStatus( L( "Look X" ), L( "Horizontal look sensitivity" ) );
	invPitch.SetNameAndStatus( L( "Invert" ), L( "Invert pitch axis" ) );

	yaw.Setup( 0.0f, 200.0f, 0.1f );
	yaw.SetNameAndStatus( L( "Look Y" ), L( "Yaw rotating sensitivity" ) );
	invYaw.SetNameAndStatus( L( "Invert" ), L( "Invert yaw axis" ) );
#endif

	AddItem( background );
	AddItem( banner );
	AddButton( L( "Controls" ), L( "Change keyboard and mouse settings" ), PC_CONTROLS, UI_Controls_Menu );
	AddButton( L( "Done" ), L( "Go back to the Configuration Menu" ), PC_DONE, VoidCb( &CMenuGamePad::SaveAndPopMenu ) );	// Обе строки уже встречались ранее !!

#ifdef MAINUI_PSP
	AddItem( axisBind[2] );
	AddItem( axisBind[3] );
#else
	for( i = 0; i < 6; i++ )
	{
		AddItem( axisBind[i] );
	}
#endif
	AddItem( side );
	AddItem( invSide );
	AddItem( forward );
	AddItem( invFwd );
	AddItem( pitch );
	AddItem( invPitch );
	AddItem( yaw );
	AddItem( invYaw );
#ifdef MAINUI_PSP
	AddItem( joyDzMin );
	AddItem( joyDzMax );
	AddItem( joyCvPower );
	AddItem( joyCvExpo );
#endif
	AddItem( axisBind_label );
}

void CMenuGamePad::_VidInit()
{
#ifdef MAINUI_PSP
	axisBind_label.SetCoord( 80, 250 );
	axisBind_label.SetCharSize( QM_SMALLFONT );

	axisBind[2].SetRect( 80, 400, 256, invSide.size.h );
	axisBind[2].SetCharSize( QM_SMALLFONT );
	axisBind[3].SetRect( 80, 450, 256, invSide.size.h );
	axisBind[3].SetCharSize( QM_SMALLFONT );

	int sliderAlign = invSide.size.h - side.size.h;

	side.SetCoord( 388, 280 + sliderAlign );
	side.SetCharSize( QM_SMALLFONT );
	invSide.SetCoord( 600, 280 );

	forward.SetCoord( 388, 360 + sliderAlign );
	forward.SetCharSize( QM_SMALLFONT );
	invFwd.SetCoord( 600, 360 );

	pitch.SetCoord( 388, 440 + sliderAlign );
	pitch.SetCharSize( QM_SMALLFONT );
	invPitch.SetCoord( 600, 440 );

	yaw.SetCoord( 388, 520 + sliderAlign );
	yaw.SetCharSize( QM_SMALLFONT );
	invYaw.SetCoord( 600, 520 );

	joyDzMin.SetCoord( 800, 280 + sliderAlign );
	joyDzMin.SetCharSize( QM_SMALLFONT );

	joyDzMax.SetCoord( 800, 360 + sliderAlign );
	joyDzMax.SetCharSize( QM_SMALLFONT );

	joyCvPower.SetCoord( 800, 440 + sliderAlign );
	joyCvPower.SetCharSize( QM_SMALLFONT );

	joyCvExpo.SetCoord( 800, 520 + sliderAlign );
	joyCvExpo.SetCharSize( QM_SMALLFONT );
#else
	axisBind_label.SetCoord( 360, 230 );
	axisBind_label.SetCharSize( QM_SMALLFONT );

	for( int i = 0, y = 280; i < 6; i++, y += 50 )
	{
		axisBind[i].SetRect( 360, y, 256, invSide.size.h );
		axisBind[i].SetCharSize( QM_SMALLFONT );
	}

	int sliderAlign = invSide.size.h - side.size.h;

	side.SetCoord( 630, 280 + sliderAlign );
	side.SetCharSize( QM_SMALLFONT );
	invSide.SetCoord( 850, 280 );

	forward.SetCoord( 630, 330 + sliderAlign );
	forward.SetCharSize( QM_SMALLFONT );
	invFwd.SetCoord( 850, 330 );

	pitch.SetCoord( 630, 380 + sliderAlign );
	pitch.SetCharSize( QM_SMALLFONT );
	invPitch.SetCoord( 850, 380 );

	yaw.SetCoord( 630, 430 + sliderAlign );
	yaw.SetCharSize( QM_SMALLFONT );
	invYaw.SetCoord( 850, 430 );
#endif
	GetConfig();
}

ADD_MENU( menu_gamepad, CMenuGamePad, UI_GamePad_Menu );
