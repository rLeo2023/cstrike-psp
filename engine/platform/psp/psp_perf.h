/*
psp_perf.h - PSP frame time accounting for xash.log
Copyright (C) 2026

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef PSP_PERF_H
#define PSP_PERF_H

// Totals since the last report (CL_PSP_PerfReport, cl_main.c), in seconds.
// work = server + client + render + sound + whatever else Host_Frame does;
// the time the fps cap sleeps is not in it.
typedef struct
{
	double		start;	// beginning of this measuring window
	unsigned int	frames;
	double		work;	// Host_Frame after Host_FilterTime
	double		server;	// Host_ServerFrame, bots included
	double		bots;	// game StartFrame: bots and game rules
	double		render;	// SCR_UpdateScreen, GPU wait included
	double		scene;	// of render: V_RenderView, the 3D view
	double		hud;	// of render: client HUD_Redraw and VGUI
	double		endframe;	// of render: R_EndFrame, list finish + swap
	double		sound;	// SND_UpdateSound
	double		client;	// the rest of Host_ClientFrame
	double		worst;	// longest single frame
} psp_perf_t;

extern psp_perf_t psp_perf;

#endif // PSP_PERF_H
