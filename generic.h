/* generic.h: The internal shared definitions of the generic layer.
 * 
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_generic_h_
#define	HEADER_generic_h_

#include "oshwbind.h"

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

struct gamestate;

/* The dimensions of the visible area of the map (in tiles).
 */
#define	NXTILES		9
#define	NYTILES		9

/*
 * Values global to this module. All the globals are placed in here,
 * in order to minimize pollution of the main module's namespace.
 */

typedef	struct genericglobals
{
    /* 
     * Shared variables.
     */

    short		wtile;		/* width of one tile in pixels */
    short		htile;		/* height of one tile in pixels */
    short		cptile;		/* size of one tile in pixels */
    TW_Surface	       *screen;		/* the display */
    TW_Rect		maploc;		/* location of the map in the window */

    /* Coordinates of the NW corner of the visible part of the map
     * (measured in quarter-tiles), or -1 if no map is currently visible.
     */
    int			mapvieworigin;

} genericglobals;

/* generic module's structure of globals.
 */
extern genericglobals geng;

/* 
 * Shared functions.
 */

/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
OSHW_EXTERN void eventupdate(int wait);

/* Render the view of the visible area of the map to the display, with
 * the view position centered on the display as much as possible. The
 * gamestate's map and the list of creatures are consulted to
 * determine what to render.
 */
OSHW_EXTERN void displaymapview(struct gamestate const *state, TW_Rect disploc);

/* Draw a tile of the given id at the position (xpos, ypos).
 */
OSHW_EXTERN void drawfulltileid(TW_Surface *dest, int xpos, int ypos, int id);


/* The initialization functions for the various modules.
 */
OSHW_EXTERN int generictimerinitialize(int showhistogram);
OSHW_EXTERN int generictileinitialize(void);

#undef OSHW_EXTERN

#endif
