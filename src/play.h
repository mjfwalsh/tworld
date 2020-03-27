/* play.h: Functions to drive game-play and manage the game state.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_play_h_
#define	HEADER_play_h_

#include	"defs.h"

/* The different modes of the program with respect to gameplay.
 */
enum {
	NormalPlay, EndPlay,
	SuspendPlay, SuspendPlayShuttered,
	NonrenderPlay
};

/* TRUE if the program is running without a user interface.
 */
extern bool batchmode;


/* Change the current gameplay mode. This affects the running of the
 * timer and the handling of the keyboard.
 */
extern void setgameplaymode(int mode);

/* Initialize the current state to the starting position of the
 * given level.
 */
extern bool initgamestate(gamesetup *game, int ruleset);

/* Set up the current state to play from its prerecorded solution.
 * FALSE is returned if no solution is available for playback.
 */
extern bool prepareplayback(void);

/* Change the step value.
 */
extern void setstepping(int step);

/* Get the step value.
 */
extern int getstepping();

/* Return the amount of time passed in the current game, in seconds.
 */
extern int secondsplayed(void);

/* Handle one tick of the game. cmd is the current keyboard command
 * supplied by the user, or CmdPreserve if any pending command is to
 * be retained. The return value is positive if the game was completed
 * successfully, negative if the game ended unsuccessfully, and zero
 * if the game remains in progress.
 */
extern int doturn(int cmd);

/* Update the display during game play. If showframe is FALSE, then
 * nothing is actually displayed.
 */
extern void drawscreen(bool showframe);

/* Quit game play early.
 */
extern void quitgamestate();

/* Free any resources associates with the current game state.
 */
extern bool endgamestate(void);

/* Free all persistent resources in the module.
 */
extern void shutdowngamestate(void);

/* Initialize the current state to a small level used for display at
 * the completion of a series.
 */
extern void setenddisplay(void);

/* Return TRUE if a solution exists for the given level.
 */
extern bool hassolution(gamesetup const *game);

/* Replace the user's solution with the just-executed solution if it
 * beats the existing solution for shortest time. FALSE is returned if
 * nothing was changed.
 */
extern bool replacesolution();

/* Double-check the timing for a solution that has just been played
 * back. If the timing is incorrect, but the cause of the discrepancy
 * can be reasonably ascertained to be benign, the timings will be
 * corrected and the return value will be TRUE.
 */
extern bool checksolution(void);

/* Turn pedantic mode on. The ruleset will be slightly changed to be
 * as faithful as possible to the original source material.
 */
extern void setpedanticmode(bool v);


#endif
