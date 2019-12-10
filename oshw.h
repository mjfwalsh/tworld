/* oshw.h: Platform-specific functions that talk with the OS/hardware.
 *
 * Copyright (C) 2001-2017 by Brian Raiter, Madhav Shanbhag, and Eric Schmidt
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_oshw_h_
#define	HEADER_oshw_h_

#include	<stdarg.h>
#include	"gen.h"

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
OSHW_EXTERN int tworld();

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
OSHW_EXTERN int oshwinitialize(int silence, int soundbufsize, int showhistogram);

/*
 * Timer functions.
 */

/* Control the timer depending on the value of action. A negative
 * value turns off the timer if it is running and resets the counter
 * to zero. A zero value turns off the timer but does not reset the
 * counter. A positive value starts (or resumes) the timer.
 */
OSHW_EXTERN void settimer(int action);

/* Set the length (in real time) of a second of game time. A value of
 * zero selects the default of 1000 milliseconds.
 */
OSHW_EXTERN void settimersecond(int ms);

/* Return the number of ticks since the timer was last reset.
 */
OSHW_EXTERN int gettickcount(void);

/* Put the program to sleep until the next timer tick.
 */
OSHW_EXTERN int waitfortick(void);

/* Force the timer to advance to the next tick.
 */
OSHW_EXTERN int advancetick(void);

/*
 * Keyboard input functions.
 */

/* Turn keyboard repeat on or off. If enable is TRUE, the keys other
 * than the direction keys will repeat at the standard rate.
 */
OSHW_EXTERN int setkeyboardrepeat(int enable);

/* Alter the behavior of the keys used to indicate movement in the
 * game. If enable is TRUE, the direction keys repeat whenever the
 * program polls the keyboard. Otherwise, the direction keys do not
 * repeat until the program polls the keyboard three times.
 */
OSHW_EXTERN int setkeyboardarrowsrepeat(int enable);

/* Return the latest/current keystroke. If wait is TRUE and no
 * keystrokes are pending, the function blocks until a keystroke
 * arrives.
 */
OSHW_EXTERN int input(int wait);

/*
 * Resource-loading functions.
 */

/* Extract the tile images stored in the given file and use them as
 * the current tile set. FALSE is returned if the attempt was
 * unsuccessful. If complain is FALSE, no error messages will be
 * displayed.
 */
OSHW_EXTERN int loadtileset(char const *filename, int complain);

/* Free all memory associated with the current tile images.
 */
OSHW_EXTERN void freetileset(void);

/*
 * Video output functions.
 */

/* Create a display surface appropriate to the requirements of the
 * game (e.g., sized according to the tiles and the font). FALSE is
 * returned on error.
 */
OSHW_EXTERN int creategamedisplay(void);

/* Fill the display with the background color.
 */
OSHW_EXTERN void cleardisplay(void);

/* Display the current game state. timeleft and besttime provide the
 * current time on the clock and the best time recorded for the level,
 * measured in seconds.
 */
OSHW_EXTERN int displaygame(struct gamestate const *state,
			    int timeleft, int besttime);

/* Display a short message appropriate to the end of a level's game
 * play. If the level was completed successfully, completed is TRUE,
 * and the other three arguments define the base score and time bonus
 * for the level, and the user's total score for the series; these
 * scores will be displayed to the user. If the return value is CmdNone,
 * the program will wait for subsequent user input, otherwise the
 * command returned will be used as the next action.
 */
OSHW_EXTERN int displayendmessage(int basescore, int timescore,
			     long totalscore, int completed);


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
 * Either listtype or inputcallback must be used to tailor the UI.
 * listtype specifies the type of list being displayed.
 * inputcallback points to a function that is called to retrieve
 * input. The function is passed a pointer to an integer. If the
 * callback returns TRUE, this integer should be set to either a new
 * index value or one of the following enum values. This value will
 * then cause the selection to be changed, whereupon the display will
 * be updated before the callback is called again. If the callback
 * returns FALSE, the table is removed from the display, and the value
 * stored in the integer will become displaylist()'s return value.
 */
OSHW_EXTERN int displaylist(tablespec const *table, int *index, DisplayListType listtype);

/* Input prompts.
 */

/* Display an confirm yes/no dialog
 */
OSHW_EXTERN int displayyesnoprompt(const char* prompt);

/* Display an text prompt for a password
 */
OSHW_EXTERN const char *displaypasswordprompt();

/*
 * Sound functions.
 */

/* Activate or deactivate the sound system. The return value is TRUE
 * if the sound system is (or already was) active.
 */
OSHW_EXTERN int setaudiosystem(int active);

/* Load a wave file into memory. index indicates which sound effect to
 * associate the sound with. FALSE is returned if an error occurs.
 */
OSHW_EXTERN int loadsfxfromfile(int index, char const *filename);

/* Specify the sounds effects to be played at this time. sfx is the
 * bitwise-or of any number of sound effects. If a non-continuous
 * sound effect in sfx is already playing, it will be restarted. Any
 * continuous sound effects that are currently playing that are not
 * set in sfx will stop playing.
 */
OSHW_EXTERN void playsoundeffects(unsigned long sfx);

/* Control sound-effect production depending on the value of action.
 * A negative value turns off all sound effects that are playing. A
 * zero value temporarily suspends the playing of sound effects. A
 * positive value continues the sound effects at the point at which
 * they were suspended.
 */
OSHW_EXTERN void setsoundeffects(int action);

/* Set the current volume level. Volume ranges from 0 (silence) to 10
 * (the default). Setting the sound to zero causes sound effects to be
 * displayed as textual onomatopoeia. If display is TRUE, the new
 * volume level will be displayed to the user. FALSE is returned if
 * the sound system is not currently active.
 */
OSHW_EXTERN int setvolume(int volume);

/* Alters the current volume level by delta.
 */
OSHW_EXTERN int changevolume(int delta);

/* Release all memory used for the given sound effect's wave data.
 */
OSHW_EXTERN void freesfx(int index);

/*
 * Miscellaneous functions.
 */

/* Ring the bell.
 */
OSHW_EXTERN void ding(void);

/* Set the program's subtitle. A NULL subtitle is equivalent to the
 * empty string. The subtitle is displayed in the window dressing (if
 * any).
 */
OSHW_EXTERN void setsubtitle(char const *subtitle);

/* Display a message to the user. cfile and lineno can be NULL and 0
 * respectively; otherwise, they identify the source code location
 * where this function was called from. prefix is an optional string
 * that is displayed before and/or apart from the body of the message.
 * fmt and args define the formatted text of the message body. action
 * indicates how the message should be presented. NOTIFY_LOG causes
 * the message to be displayed in a way that does not interfere with
 * the program's other activities. NOTIFY_ERR presents the message as
 * an error condition. NOTIFY_DIE should indicate to the user that the
 * program is about to shut down.
 */
OSHW_EXTERN void usermessage(int action, char const *prefix,
			char const *cfile, unsigned long lineno,
			char const *fmt, va_list args);

/* Values used for the first argument of usermessage().
 */
enum { NOTIFY_DIE, NOTIFY_ERR, NOTIFY_LOG };

/* Get the selected ruleset.
 */
OSHW_EXTERN int getselectedruleset(void);

/* Read any additional data for the series.
 */
OSHW_EXTERN void readextensions(struct gameseries *series);

/* Get number of seconds to skip at start of playback.
 */
OSHW_EXTERN int getreplaysecondstoskip(void);

/* Copy text to clipboard.
 */
OSHW_EXTERN void copytoclipboard(char const *text);

/* Change play button symbol
 */
OSHW_EXTERN void setplaypausebutton(int p);

/* get app and config folders, create latter if necessary
 */
OSHW_EXTERN void initdirs();


/* The directory containing all the resource files.
 */
OSHW_EXTERN char	       *resdir;

/* Parse the rc file and initialize the resources that are needed at
 * the start of the program (i.e., the font and color settings).
 * FALSE is returned if the rc file contained errors or if a resource
 * could not be loaded.
 */
OSHW_EXTERN int initresources(void);

/* Load all resources, using the settings for the given ruleset. FALSE
 * is returned if any critical resources could not be loaded.
 */
OSHW_EXTERN int loadgameresources(int ruleset);

/* Release all memory allocated for the resources.
 */
OSHW_EXTERN void freeallresources(void);

#undef OSHW_EXTERN

#endif
