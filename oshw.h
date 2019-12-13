/* oshw.h: Platform-specific functions that talk with the OS/hardware.
 *
 * Copyright (C) 2001-2017 by Brian Raiter, Madhav Shanbhag, and Eric Schmidt
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_oshw_h_
#define	HEADER_oshw_h_

#include	<stdarg.h>
#include	"defs.h"

struct gamestate;
struct gameseries;

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* This is the declaration of the top layer's main function. It is
 * called directly from the real main() inside the OS/hardware layer.
 */
OSHW_EXTERN int tworld(); // tworld.c

/* Initialize the OS/hardware interface. This function must be called
 * before any others in the oshw library. If silence is TRUE, the
 * sound system will be disabled, as if no soundcard was present. If
 * showhistogram is TRUE, then during shutdown the timer module will
 * send a histogram to stdout describing the amount of time the
 * program explicitly yielded to other processes. (This feature is for
 * debugging purposes.) soundbufsize is a number between 0 and 3 which
 * is used to scale the size of the sound buffer. A larger number is
 * more efficient, but pushes the sound effects farther out of
 * synchronization with the video.
 */
OSHW_EXTERN int oshwinitialize(int silence, int soundbufsize, int showhistogram); // tworld.c


/*
 * Keyboard input functions.
 */

/* Turn keyboard repeat on or off. If enable is TRUE, the keys other
 * than the direction keys will repeat at the standard rate.
 */
OSHW_EXTERN int setkeyboardrepeat(int enable); // TWMainWnd.cpp

/* Alter the behavior of the keys used to indicate movement in the
 * game. If enable is TRUE, the direction keys repeat whenever the
 * program polls the keyboard. Otherwise, the direction keys do not
 * repeat until the program polls the keyboard three times.
 */
OSHW_EXTERN int setkeyboardarrowsrepeat(int enable); // in.cpp

/* Return the latest/current keystroke. If wait is TRUE and no
 * keystrokes are pending, the function blocks until a keystroke
 * arrives.
 */
OSHW_EXTERN int input(int wait); // in.cpp


/*
 * Video output functions.
 */

/* Create a display surface appropriate to the requirements of the
 * game (e.g., sized according to the tiles and the font). FALSE is
 * returned on error.
 */
OSHW_EXTERN int creategamedisplay(void); // TWMainWnd.cpp

/* Fill the display with the background color.
 */
OSHW_EXTERN void cleardisplay(void); // TWMainWnd.cpp

/* Display the current game state. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
OSHW_EXTERN int displaygame(struct gamestate const *state,
			    int timeleft, int besttime); // TWMainWnd.cpp

/* Display a short message appropriate to the end of a level's game
 * play. If the level was completed successfully, completed is TRUE,
 * and the other three arguments define the base score and time bonus
 * for the level, and the user's total score for the series; these
 * scores will be displayed to the user. If the return value is CmdNone,
 * the program will wait for subsequent user input, otherwise the
 * command returned will be used as the next action.
 */
OSHW_EXTERN int displayendmessage(int basescore, int timescore,
			     long totalscore, int completed); // TWMainWnd.cpp


/* Types of lists that can be displayed.
 */
typedef enum {
    LIST_MAPFILES,
    LIST_SERIES,
    LIST_SCORES,
    LIST_SOLUTIONFILES
} DisplayListType;

/* Display a scrollable table. title provides a title to display. The
 * table's first row provides a set of column headers which will not
 * scroll. index points to the index of the item to be initially
 * selected; upon return, the value will hold the current selection.
 * listtype specifies the type of list being displayed.
 * The function is passed a pointer to an integer. If the
 * callback returns TRUE, this integer should be set to either a new
 * index value or one of the following enum values. This value will
 * then cause the selection to be changed, whereupon the display will
 * be updated before the callback is called again. If the callback
 * returns FALSE, the table is removed from the display, and the value
 * stored in the integer will become displaylist()'s return value.
 */
OSHW_EXTERN int displaylist(tablespec const *table, int *index,
	DisplayListType listtype); // TWMainWnd.cpp

/* Input prompts.
 */

/* Display an confirm yes/no dialog
 */
OSHW_EXTERN int displayyesnoprompt(const char* prompt); // TWMainWnd.cpp

/* Display an text prompt for a password
 */
OSHW_EXTERN const char *displaypasswordprompt(); // TWMainWnd.cpp


/*
 * Miscellaneous functions.
 */

/* Ring the bell.
 */
OSHW_EXTERN void ding(void); // TWMainWnd.cpp

/* Set the program's subtitle. A NULL subtitle is equivalent to the
 * empty string. The subtitle is displayed in the window dressing (if
 * any).
 */
OSHW_EXTERN void setsubtitle(char const *subtitle); // TWMainWnd.cpp

/* Get the selected ruleset.
 */
OSHW_EXTERN int getselectedruleset(void); // TWMainWnd.cpp

/* Read any additional data for the series.
 */
OSHW_EXTERN void readextensions(struct gameseries *series); // TWMainWnd.cpp

/* Get number of seconds to skip at start of playback.
 */
OSHW_EXTERN int getreplaysecondstoskip(void); // TWMainWnd.cpp

/* Copy text to clipboard.
 */
OSHW_EXTERN void copytoclipboard(char const *text); // TWApp.cpp

/* Change play button symbol
 */
OSHW_EXTERN void setplaypausebutton(int p); // TWMainWnd.cpp

/* get app and config folders, create latter if necessary
 */
OSHW_EXTERN void initdirs(); // TWApp.cpp

#undef OSHW_EXTERN

#endif
