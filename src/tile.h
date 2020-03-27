/* Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_tile_h_
#define	HEADER_tile_h_

#include "oshwbind.h"

/* Constants
 */
#define TW_ALPHA_TRANSPARENT 0

/* The dimensions of the visible area of the map (in tiles).
 */
#define	NXTILES		9
#define	NYTILES		9

/* The width/height of a tile in pixels at 100% zoom
 */
#define DEFAULTTILE		48

/* Render the view of the visible area of the map to the display, with
 * the view position centered on the display as much as possible. The
 * gamestate's map and the list of creatures are consulted to
 * determine what to render.
 */
extern void displaymapview(struct gamestate const *state, TW_Rect disploc);

/* Draw a tile of the given id at the position (xpos, ypos).
 */
extern void drawfulltileid(Qt_Surface *dest, int xpos, int ypos, int id);

/* Extract the tile images stored in the given file and use them as
 * the current tile set. FALSE is returned if the attempt was
 * unsuccessful. If complain is FALSE, no error messages will be
 * displayed.
 */
extern bool loadtileset(char const *filename, bool complain);

#endif
