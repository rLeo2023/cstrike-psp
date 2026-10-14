/*
sys_psp.c - PSP System utils
Copyright (C) 2021 Sergey Galushko

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "platform/platform.h"
#include "platform/psp/ka/ka.h"
#include "platform/psp/scemp3/pspmp3.h"
#include <pspsdk.h>
#include <pspkernel.h>
#include <psppower.h>
#include <psprtc.h>
#include <pspprof.h>
#include <pspctrl.h>
#include <psputility.h>
#include <pspge.h>
#include <pspdisplay.h>

#include <ctype.h>
#include <stddef.h> // ptrdiff_t
#include <malloc.h> // mallinfo


PSP_MODULE_INFO( "Counter-Strike", PSP_MODULE_USER, 1, 0 );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU );
PSP_MAIN_THREAD_STACK_SIZE_KB( 512 );
/* The most the heap takes, on 64 MB models with extra memory. A negative
   size ("all free memory minus a threshold") makes the heap allocation fail
   on startup, every malloc then fails with "Mem_AllocPool: out of memory"
   at host.c, both with "-3 * 1024" and with -1 plus
   PSP_HEAP_THRESHOLD_SIZE_KB( 3 * 1024 ).
   PSP_LogMemory() reports what is left outside the heap, size it from that:
   with a 30 MB heap (MEMSIZE=1, PPSSPP) it showed 14235 KB still free, and
   30 MB ran out during CS precache. 40 MB keeps ~4 MB for modules/stacks. */
#define PSP_HEAP_MAX_KB	( 40 * 1024 )
PSP_HEAP_SIZE_KB( PSP_HEAP_MAX_KB );

/* With less memory (PSP-1000: 24 MB for games) the heap is the largest free
   block minus this, left for what is loaded after the heap is taken.
   Measured: audio, renderer and threads take ~32 KB, the ad-hoc network
   modules 176 KB more */
#define PSP_HEAP_RESERVE_KB	512

extern SceLwMutexWorkarea	__sbrk_mutex;	// libcglue
extern SceUID		__psp_heap_blockid;	// libcglue, __psp_free_heap frees it on exit
static char		*heap_bottom, *heap_top, *heap_end;
static int		heap_maxfree_kb;	// largest free block when the heap was taken

/*
================
_sbrk

replaces libcglue's: same single block taken on the first call (libc init
and C++ constructors already allocate before main), but sized to fit the
memory there is. sce_newlib_heap_kb_size ends up with the real size, so the
logs and the low memory checks keep using it
================
*/
void *_sbrk( ptrdiff_t incr )
{
	void	*result = (void *)-1;

	sceKernelLockLwMutex( &__sbrk_mutex, 1, NULL );

	if( !heap_bottom )
	{
		int	maxfree = (int)sceKernelMaxFreeMemSize();
		int	size = sce_newlib_heap_kb_size * 1024;

		heap_maxfree_kb = maxfree / 1024;

		if( size > maxfree - PSP_HEAP_RESERVE_KB * 1024 )
			size = ( maxfree - PSP_HEAP_RESERVE_KB * 1024 ) & ~0xFFFF; // 64 KB multiple

		if( size > 0 )
		{
			__psp_heap_blockid = sceKernelAllocPartitionMemory( 2, "heap_block", PSP_SMEM_Low, size, NULL );
			if( __psp_heap_blockid > 0 )
			{
				heap_bottom = heap_top = (char *)sceKernelGetBlockHeadAddr( __psp_heap_blockid );
				heap_end = heap_bottom + size;
				sce_newlib_heap_kb_size = size / 1024;
			}
		}
	}

	if( heap_bottom && heap_top + incr >= heap_bottom && heap_top + incr <= heap_end )
	{
		result = heap_top;
		heap_top += incr;
	}

	sceKernelUnlockLwMutex( &__sbrk_mutex, 1 );

	return result;
}

static qboolean	psp_audiolib_init = false;

static int Platform_ExitCallback( int count, int arg, void *common )
{
	host.crashed = true;
	return 0;
}

static int Platform_PowerCallback( int count, int arg, void *common )
{
	P5Ram_PowerCallback( count, arg, common );
	return 0;
}

static int Platform_CallbackThread( SceSize args, void *argp )
{
	int cbid;

	//cbid = sceKernelCreateCallback( "Exit Callback", Platform_ExitCallback, NULL );
	//sceKernelRegisterExitCallback( cbid );

	cbid = sceKernelCreateCallback( "Power Callback", Platform_PowerCallback, NULL );
	scePowerRegisterCallback( 0, cbid );

	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
static int Platform_SetupCallbacks( void )
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", Platform_CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

#if XASH_TIMER == TIMER_PSP
double Platform_DoubleTime( void )
{
	return ( double )sceKernelGetSystemTimeWide() * 0.000001; // microseconds to seconds
}

void Platform_Sleep( int msec )
{
	sceKernelDelayThread( msec * 1000 );
}
#endif

void *Platform_GetNativeObject( const char *name )
{
	return NULL;
}

void Platform_Vibrate( float life, char flags )
{

}

void Platform_GetClipboardText( char *buffer, size_t size )
{

}

void Platform_SetClipboardText( const char *buffer, size_t size )
{

}

void Platform_ShellExecute( const char *path, const char *parms )
{

}

#if XASH_MESSAGEBOX == MSGBOX_PSP
// VRAM offset of the error screen (512x272 8888, 544 KB): past the renderer's
// draw, display and depth buffers (0 - 0x154000), inside the 2 MB of a
// PSP-1000. pspDebugScreenInit() wrote into the frame the GE draws into:
// a list still running painted over the text, and PPSSPP kept showing its
// own copy of that frame, so errors looked like a black screen
#define PSP_MSGBOX_VRAM_OFFSET	0x170000

void Platform_MessageBox( const char *title, const char *message, qboolean parentMainWindow )
{
	qboolean	nomemory = Q_stristr( message, "out of memory" ) != NULL;

	// keep a copy in xash.log too
	Sys_PrintLog( va( "MessageBox [%s]: %s\n", title, message ));

	// renderer uploads fail with their own message, not through Mem_Alloc
	if( nomemory )
	{
		void Mem_PrintReport( void );
		Mem_PrintReport();
	}

	// Clear the sound buffer.
	S_StopAllSounds( true );

	// drop whatever the GE has queued (waiting with sceGeDrawSync hung: an
	// error while loading leaves an open list that never finishes), then
	// show the message from VRAM it never drew into
	{
		PspGeBreakParam	brk;
		sceGeBreak( 1, &brk );
	}
	pspDebugScreenInitEx( (void *)( 0x40000000 | ( (u32)sceGeEdramGetAddr() + PSP_MSGBOX_VRAM_OFFSET )), PSP_DISPLAY_PIXEL_FORMAT_8888, 1 );
	pspDebugScreenClear();

	pspDebugScreenSetTextColor( 0x0080ff );
	pspDebugScreenPrintf( "\n\n\n\n%s:\n", title );
	pspDebugScreenSetTextColor( 0x0000ff );
	pspDebugScreenPrintData( message, Q_strlen( message ) );

	// the heap got less than it wanted: no extra memory (PSP-1000, or the
	// CFW option is off)
	if( nomemory && sce_newlib_heap_kb_size < PSP_HEAP_MAX_KB )
	{
		pspDebugScreenSetTextColor( 0x00ffff );
		pspDebugScreenPrintf( "\n\nOnly %d MB of memory available (%d MB needed).\n"
			"PSP-2000/3000/Go: enable \"Force Extra Memory\" in your CFW settings.\n"
			"PSP-1000 is not supported yet.\n\n"
			"Solo hay %d MB de memoria (se necesitan %d MB).\n"
			"PSP-2000/3000/Go: activa \"Force Extra Memory\" en el CFW.\n"
			"La PSP-1000 todavia no es compatible.\n",
			sce_newlib_heap_kb_size / 1024, PSP_HEAP_MAX_KB / 1024,
			sce_newlib_heap_kb_size / 1024, PSP_HEAP_MAX_KB / 1024 );
	}

	pspDebugScreenSetTextColor( 0xffffff );
	pspDebugScreenPrintf( "\n\nPress X to continue.\n" );
	Sys_PrintLog( "MessageBox: shown, waiting for X\n" );

	// Wait for a X button press (released first: it may be held from the menu).
	SceCtrlData pad;
	do
	{
		sceCtrlReadBufferPositive( &pad, 1 );
	}
	while( pad.Buttons & PSP_CTRL_CROSS );
	do
	{
		sceCtrlReadBufferPositive( &pad, 1 );
	}
	while( !( pad.Buttons & PSP_CTRL_CROSS ) );
	Sys_PrintLog( "MessageBox: X pressed\n" );
	pspDebugScreenClear();
}
#endif // XASH_MESSAGEBOX == MSGBOX_PSP

void Platform_ReadCmd( const char *fname, int *argc, char **argv )
{
	int 		cmd_fd;
	size_t		cmd_fsize;
	byte		*cmd_buff;
	int			i, j = 0;

	cmd_fd = sceIoOpen( fname, PSP_O_RDONLY, 0777 );
	if( cmd_fd > 0 )
	{
		cmd_fsize = sceIoLseek( cmd_fd, 0, PSP_SEEK_END );
		sceIoLseek( cmd_fd, 0, PSP_SEEK_SET );

		printf( "CMD FILE(%s) Size: %i\n", fname, cmd_fsize );

		if( cmd_fsize == 0 ) return;

		cmd_buff = malloc( cmd_fsize );
		if( !cmd_buff )
		{
			printf( "CMD FILE(%s) Memory allocation error!\n", fname );
			sceIoClose(cmd_fd);
			return;
		}

		if( sceIoRead(cmd_fd, cmd_buff, cmd_fsize) >= 0 )
		{
			for( i = 0; i < cmd_fsize; i++ )
			{
				if( isspace( cmd_buff[i] ) != 0 )
				{
					if( j == 0 ) continue;

					argv[*argc] = malloc( j + 1 );
					if( !argv[*argc] )
					{
						printf( "CMD FILE(%s) Memory allocation error!\n", fname );
						free( cmd_buff );
						sceIoClose( cmd_fd );
						return;
					}
					memcpy( argv[*argc], &cmd_buff[i - j], j );
					argv[*argc][j] = 0x00;
					( int )( *argc )++;
					j = 0;
				}
				else j++;

			}
			if( j != 0 )
			{
				argv[*argc] = malloc( j + 1 );
				if( !argv[*argc] )
				{
					printf( "CMD FILE(%s) Memory allocation error!\n", fname );
					free( cmd_buff );
					sceIoClose( cmd_fd );
					return;
				}
				memcpy( argv[*argc], &cmd_buff[i - j], j );
				argv[*argc][j] = 0x00;
				( int )( *argc )++;
				j = 0;
			}
		}
		else printf( "CMD FILE(%s) Read error!\n", fname );

		free( cmd_buff );
		sceIoClose( cmd_fd );
	}
}

SceUID Platform_LoadModule( const char *filename, int mpid, SceSize argsize, void *argp )
{
	SceKernelLMOption option;
	SceUID modid = 0;
	int retVal = 0, mresult;

	memset( &option, 0, sizeof( option ) );
	option.size = sizeof( option );
	option.mpidtext = mpid;
	option.mpiddata = mpid;
	option.position = 0;
	option.access = 1;

	retVal = sceKernelLoadModule( filename, 0, &option );
	if(retVal < 0)
		return retVal;

	modid = retVal;

	retVal = sceKernelStartModule( modid, argsize, argp, &mresult, NULL );
	if( retVal < 0 )
		return retVal;

	return modid;
}

int Platform_UnloadModule( SceUID modid, int *sce_code )
{
	int status;
	*sce_code = sceKernelStopModule( modid, 0, NULL, &status, NULL);

	if( ( *sce_code ) < 0 )
		return -2;
	else if( status == SCE_KERNEL_ERROR_NOT_STOPPED )
		return -1;

	*sce_code = sceKernelUnloadModule( modid );
	return ( ( ( *sce_code ) < 0 ) ? -2 : 0 );
}

int Platform_InitAudioLibs( void )
{
	int	status;

	if( psp_audiolib_init )
		return -1;

	status = sceUtilityLoadModule( PSP_MODULE_AV_AVCODEC );
	if ( status < 0 )
	{
		Con_DPrintf( S_ERROR "sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC) returned 0x%08X\n", status );
		return status;
	}

	status = sceUtilityLoadModule( PSP_MODULE_AV_MP3 );
	if ( status < 0 )
	{
		Con_DPrintf( S_ERROR "sceUtilityLoadModule(PSP_MODULE_AV_MP3) returned 0x%08X\n", status );
		return status;
	}

	// init mp3 resources
	status = sceMp3InitResource();
	if ( status < 0 )
	{
		Con_DPrintf( S_ERROR "sceMp3InitResource returned 0x%08X\n", status );
		return status;
	}

	psp_audiolib_init = true;

	return 0;
}

int Platform_ShutdownAudioLibs( void )
{
	int	status;

	if( !psp_audiolib_init )
		return -1;

	status = sceMp3TermResource();
	if ( status < 0 )
		Con_DPrintf( S_ERROR "sceMp3TermResource returned 0x%08X\n", status );

	status = sceUtilityUnloadModule( PSP_MODULE_AV_MP3 );
	if ( status < 0 )
		Con_DPrintf( S_ERROR "sceUtilityUnloadModule(PSP_MODULE_AV_MP3) returned 0x%08X\n", status );

	status = sceUtilityUnloadModule( PSP_MODULE_AV_AVCODEC );
	if ( status < 0 )
		Con_DPrintf( S_ERROR "sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC) returned 0x%08X\n", status );

	psp_audiolib_init = false;

	return 0;
}

/*
================
PSP_LogMemory

memory left outside the newlib heap, called once the log file is open
================
*/
void PSP_LogMemory( void )
{
	Sys_PrintLog( va( "S_PSP: heap %d KB (of %d KB max, largest free block was %d KB), outside heap: max block %d KB, total free %d KB\n",
		sce_newlib_heap_kb_size, PSP_HEAP_MAX_KB, heap_maxfree_kb, (int)( sceKernelMaxFreeMemSize() / 1024 ),
		(int)( sceKernelTotalFreeMemSize() / 1024 )));
}

/*
================
PSP_LogHeapUsage

how much of the newlib heap is left, used once a map is fully loaded to
tell which maps fit and how much room bots would have
================
*/
int PSP_HeapFreeKB( void )
{
	struct mallinfo mi = mallinfo();
	int heap = sce_newlib_heap_kb_size * 1024;

	return (( heap - (int)mi.arena ) + (int)mi.fordblks ) / 1024;
}

void PSP_LogHeapUsage( const char *when )
{
	static int outside_lowest_kb = 0x7FFFFFFF;
	struct mallinfo mi = mallinfo();
	int heap = sce_newlib_heap_kb_size * 1024;
	// never touched part of the heap plus free blocks inside the used part
	int left = ( heap - (int)mi.arena ) + (int)mi.fordblks;
	// what is loaded after the heap (threads, audio and ad-hoc modules)
	// lives outside it: free at startup minus the lowest point tells how
	// much PSP_HEAP_RESERVE_KB really needs
	int outside_kb = (int)( sceKernelTotalFreeMemSize() / 1024 );

	if( outside_kb < outside_lowest_kb )
		outside_lowest_kb = outside_kb;

	Sys_PrintLog( va( "S_PSP: %s: outside heap free %d KB (largest block %d KB, lowest seen %d KB of %d reserved)\n",
		when, outside_kb, (int)( sceKernelMaxFreeMemSize() / 1024 ), outside_lowest_kb, PSP_HEAP_RESERVE_KB ));

	// usmblks: the furthest the heap ever reached since the game started,
	// gaps between blocks included. A heap smaller than that would have
	// run out somewhere, so it is the number PSP_HEAP_SIZE_KB must stay over
	Sys_PrintLog( va( "S_PSP: %s: heap used %d KB, free %d KB (largest chunk at least %d KB), highest point %d KB of %d\n",
		when, (int)( mi.uordblks / 1024 ), left / 1024, ( heap - (int)mi.arena ) / 1024,
		(int)(( mi.usmblks > mi.arena ? mi.usmblks : mi.arena ) / 1024 ), sce_newlib_heap_kb_size ));
}

/*
================
PSP_BootStep

startup steps before xash.log exists, into xash_boot.txt. Opened and closed
for every line: on a real PSP a crash powers it off, and a file left open
keeps the size it had when it was opened (empty)
================
*/
void PSP_BootStep( const char *step )
{
	static qboolean	started;
	FILE		*f = fopen( "xash_boot.txt", started ? "a" : "w" );

	started = true;
	if( !f )
		return;

	fprintf( f, "%s\n", step );
	fclose( f );
}

/*
================
PSP_EarlyStart

runs before every C++ constructor (priority 101 is the first one), right
after libc set up the heap. The FPU traps a division by zero or an invalid
operation on a real PSP (PPSSPP doesn't): Platform_Init turned that off,
but the constructors of the client and the game library ran before it.
The boot step tells a crash in there from one before any code ran
================
*/
__attribute__(( constructor( 101 ))) static void PSP_EarlyStart( void )
{
	pspSdkDisableFPUExceptions();
	PSP_BootStep( va( "module started: heap %d KB", sce_newlib_heap_kb_size ));
}

void Platform_Init( void )
{
	SceUID	kamID;
	int	result;

	PSP_BootStep( va( "platform init: heap %d KB, largest free block was %d KB, outside heap free %d KB",
		sce_newlib_heap_kb_size, heap_maxfree_kb, (int)( sceKernelTotalFreeMemSize() / 1024 )));

	// disable fpu exceptions (division by zero and etc...)
	pspSdkDisableFPUExceptions();

	// exit callback thread
	Platform_SetupCallbacks();
	PSP_BootStep( "callbacks" );

	// set max cpu/gpu frequency
	scePowerSetClockFrequency( 333, 333, 166 );
	PSP_BootStep( "clock 333 MHz" );

	// set max VRAM
	kamID = Platform_LoadModule( "ka.prx", 1, 0, NULL );
	PSP_BootStep( va( "ka.prx: 0x%08x", kamID ));
	if( kamID >= 0 )
	{
		result = kaGeEdramGetHwSize();
		if( result > 0 )
			result = kaGeEdramSetSize( result );
		Platform_UnloadModule( kamID, &result );
		PSP_BootStep( "VRAM size set" );
	}

	Platform_InitAudioLibs();
	PSP_BootStep( va( "audio modules: %s", psp_audiolib_init ? "ok" : "failed" ));

	// P5 Ram init
	result = P5Ram_Init();
	PSP_BootStep( va( "volatile memory: 0x%08x", result ));
}

void Platform_Shutdown( void )
{
	P5Ram_Shutdown();
	Platform_ShutdownAudioLibs();
#if XASH_PROFILING
	gprof_cleanup();
#endif
}
