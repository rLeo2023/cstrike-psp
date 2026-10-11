#include "Framework.h"
#include "Slider.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"

#define ART_BANNER			"gfx/shell/head_audio"

class CMenuAudio : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;

	CMenuAudio() : CMenuFramework("CMenuAudio") { }

private:
	void _Init() override;
	void _VidInit() override;
	void GetConfig();
	void SaveAndPopMenu() override;

	CMenuSlider	soundVolume;
	CMenuSlider	musicVolume;
	CMenuSlider	suitVolume;
	CMenuCheckBox noDSP;
};

void CMenuAudio::GetConfig( void )
{
	soundVolume.LinkCvar( "volume" );
	musicVolume.LinkCvar( "MP3Volume" );
	suitVolume.LinkCvar( "suitvolume" );
	noDSP.LinkCvar( "dsp_off" );
}

void CMenuAudio::SaveAndPopMenu()
{
	soundVolume.WriteCvar();
	musicVolume.WriteCvar();
	suitVolume.WriteCvar();
	noDSP.WriteCvar();

	CMenuFramework::SaveAndPopMenu();
}

void CMenuAudio::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	// Coordenada X = 280 para que no se encimen con el botón Done de la izquierda
	soundVolume.SetNameAndStatus( L( "GameUI_SoundEffectVolume" ), L( "" ) );
	soundVolume.Setup( 0.0, 1.0, 0.05f );
	soundVolume.onChanged = CMenuEditable::WriteCvarCb;
	soundVolume.SetCoord( 280, 260 );

	musicVolume.SetNameAndStatus( L( "GameUI_MP3Volume" ), L( "" ) );
	musicVolume.Setup( 0.0, 1.0, 0.05f );
	musicVolume.onChanged = CMenuEditable::WriteCvarCb;
	musicVolume.SetCoord( 280, 320 );

	suitVolume.SetNameAndStatus( L( "GameUI_HEVSuitVolume" ), L( "" ) );
	suitVolume.Setup( 0.0, 1.0, 0.05f );
	suitVolume.onChanged = CMenuEditable::WriteCvarCb;
	suitVolume.SetCoord( 280, 380 );

	noDSP.SetNameAndStatus( L( "Disable DSP effects" ), L( "" ) );
	noDSP.onChanged = CMenuEditable::WriteCvarCb;
	noDSP.SetCoord( 280, 440 );

	AddItem( background );
	AddItem( banner );
	AddButton( L( "Done" ), L( "" ), PC_DONE,
		VoidCb( &CMenuAudio::SaveAndPopMenu ) );
	AddItem( soundVolume );
	AddItem( musicVolume );
	AddItem( suitVolume );
	AddItem( noDSP );
}

void CMenuAudio::_VidInit( )
{
	GetConfig();
}

ADD_MENU( menu_audio, CMenuAudio, UI_Audio_Menu );