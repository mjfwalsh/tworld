/* in.c: Reading the keyboard and mouse.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	<string.h>
#include	"generic.h"
#include	"gen.h"
#include	"oshw.h"
#include	"defs.h"
#include	"err.h"

/* Structure describing a mapping of a key event to a game command.
 */
typedef	struct keycmdmap {
    int		scancode;	/* the key's scan code */
    int		shift;		/* the shift key's state */
    int		ctl;		/* the ctrl key's state */
    int		alt;		/* the alt keys' state */
    int		cmd;		/* the command */
    int		hold;		/* TRUE for repeating joystick-mode keys */
} keycmdmap;

/* Structure describing mouse activity.
 */
typedef struct mouseaction {
    int		state;		/* state of mouse action (KS_*) */
    int		x, y;		/* position of the mouse */
    int		button;		/* which button generated the event */
} mouseaction;

/* The possible states of keys.
 */
enum { KS_OFF = 0,		/* key is not currently pressed */
       KS_ON = 1,		/* key is down (shift-type keys only) */
       KS_DOWN,			/* key is being held down */
       KS_STRUCK,		/* key was pressed and released in one tick */
       KS_PRESSED,		/* key was pressed in this tick */
       KS_DOWNBUTOFF1,		/* key has been down since the previous tick */
       KS_DOWNBUTOFF2,		/* key has been down since two ticks ago */
       KS_DOWNBUTOFF3,		/* key has been down since three ticks ago */
       KS_REPEATING,		/* key is down and is now repeating */
       KS_count
};

/* The complete array of key states.
 */
static char		keystates[TWK_LAST];

/* The last mouse action.
 */
static mouseaction	mouseinfo;

/* TRUE if direction keys are to be treated as always repeating.
 */
static int		joystickstyle = FALSE;

/* The complete list of key commands recognized by the game while
 * playing. hold is TRUE for keys that are to be forced to repeat.
 * shift, ctl and alt are positive if the key must be down, zero if
 * the key must be up, or negative if it doesn't matter.
 */
static keycmdmap const gamekeycmds[] = {
    { TWK_UP,                     0,  0,  0,   CmdNorth,              TRUE },
    { TWK_LEFT,                   0,  0,  0,   CmdWest,               TRUE },
    { TWK_DOWN,                   0,  0,  0,   CmdSouth,              TRUE },
    { TWK_RIGHT,                  0,  0,  0,   CmdEast,               TRUE },
    { TWK_PAGEUP,                -1, -1,  0,   CmdPrev10,             FALSE },
    { TWK_PAGEDOWN,              -1, -1,  0,   CmdNext10,             FALSE },
    { TWK_BACKSPACE,             -1, -1,  0,   CmdPauseGame,          FALSE },
    { TWK_RETURN,                -1, -1,  0,   CmdProceed,            FALSE },
    { TWK_KP_ENTER,              -1, -1,  0,   CmdProceed,            FALSE },
    { ' ',                       -1, -1,  0,   CmdProceed,            FALSE },
#ifndef NDEBUG
    { 'd',                        0, +1,  0,   CmdDebugCmd1,          FALSE },
    { 'd',                       +1, +1,  0,   CmdDebugCmd2,          FALSE },
    { TWK_UP,                    +1,  0,  0,   CmdCheatNorth,         TRUE },
    { TWK_LEFT,                  +1,  0,  0,   CmdCheatWest,          TRUE },
    { TWK_DOWN,                  +1,  0,  0,   CmdCheatSouth,         TRUE },
    { TWK_RIGHT,                 +1,  0,  0,   CmdCheatEast,          TRUE },
    { TWK_HOME,                  +1,  0,  0,   CmdCheatHome,          FALSE },
    { TWK_F2,                     0,  0,  0,   CmdCheatICChip,        FALSE },
    { TWK_F3,                     0,  0,  0,   CmdCheatKeyRed,        FALSE },
    { TWK_F4,                     0,  0,  0,   CmdCheatKeyBlue,       FALSE },
    { TWK_F5,                     0,  0,  0,   CmdCheatKeyYellow,     FALSE },
    { TWK_F6,                     0,  0,  0,   CmdCheatKeyGreen,      FALSE },
    { TWK_F7,                     0,  0,  0,   CmdCheatBootsIce,      FALSE },
    { TWK_F8,                     0,  0,  0,   CmdCheatBootsSlide,    FALSE },
    { TWK_F9,                     0,  0,  0,   CmdCheatBootsFire,     FALSE },
    { TWK_F10,                    0,  0,  0,   CmdCheatBootsWater,    FALSE },
#endif

/* "Virtual" keys */
    { TWC_SEESCORES,              0,  0,  0,   CmdSeeScores,          FALSE },
    { TWC_SEESOLUTIONFILES,       0,  0,  0,   CmdSeeSolutionFiles,   FALSE },
    { TWC_TIMESCLIPBOARD,         0,  0,  0,   CmdTimesClipboard,     FALSE },
    { TWC_QUITLEVEL,              0,  0,  0,   CmdQuitLevel,          FALSE },
    { TWC_QUIT,                   0,  0,  0,   CmdQuit,               FALSE },

    { TWC_PROCEED,                0,  0,  0,   CmdProceed,            FALSE },
    { TWC_PAUSEGAME,              0,  0,  0,   CmdPauseGame,          FALSE },
    { TWC_SAMELEVEL,              0,  0,  0,   CmdSameLevel,          FALSE },
    { TWC_NEXTLEVEL,              0,  0,  0,   CmdNextLevel,          FALSE },
    { TWC_PREVLEVEL,              0,  0,  0,   CmdPrevLevel,          FALSE },
    { TWC_GOTOLEVEL,              0,  0,  0,   CmdGotoLevel,          FALSE },

    { TWC_PLAYBACK,               0,  0,  0,   CmdPlayback,           FALSE },
    { TWC_CHECKSOLUTION,          0,  0,  0,   CmdCheckSolution,      FALSE },
    { TWC_REPLSOLUTION,           0,  0,  0,   CmdReplSolution,       FALSE },
    { TWC_KILLSOLUTION,           0,  0,  0,   CmdKillSolution,       FALSE },
    { TWC_SEEK,                   0,  0,  0,   CmdSeek,               FALSE },
    { 0, 0, 0, 0, 0, 0 }
};

/* The list of key commands recognized when the program is obtaining
 * input from the user.
 */
static keycmdmap const inputkeycmds[] = {
    { TWK_UP,                    -1, -1,  0,   CmdNorth,              FALSE },
    { TWK_LEFT,                  -1, -1,  0,   CmdWest,               FALSE },
    { TWK_DOWN,                  -1, -1,  0,   CmdSouth,              FALSE },
    { TWK_RIGHT,                 -1, -1,  0,   CmdEast,               FALSE },
    { TWK_BACKSPACE,             -1, -1,  0,   CmdWest,               FALSE },
    { TWK_RETURN,                -1, -1,  0,   CmdProceed,            FALSE },
    { TWK_KP_ENTER,              -1, -1,  0,   CmdProceed,            FALSE },
    { 0, 0, 0, 0, 0, 0 }
};

/* The current map of key commands.
 */
static keycmdmap const *keycmds = gamekeycmds;

/* A map of keys that can be held down simultaneously to produce
 * multiple commands.
 */
static int mergeable[CmdKeyMoveLast + 1];

/*
 * Running the keyboard's state machine.
 */

/* This callback is called whenever the state of any keyboard key
 * changes. It records this change in the keystates array. The key can
 * be recorded as being struck, pressed, repeating, held down, or down
 * but ignored, as appropriate to when they were first pressed and the
 * current behavior settings. Shift-type keys are always either on or
 * off.
 */
static void _keyeventcallback(int scancode, int down)
{
    switch (scancode) {
      case TWK_LSHIFT:
      case TWK_RSHIFT:
      case TWK_LCTRL:
      case TWK_RCTRL:
      case TWK_LALT:
      case TWK_RALT:
      case TWK_LMETA:
      case TWK_RMETA:
      case TWK_NUMLOCK:
      case TWK_CAPSLOCK:
      case TWK_MODE:
	keystates[scancode] = down ? KS_ON : KS_OFF;
	break;
      default:
	if (scancode < TWK_LAST) {
	    if (down) {
		keystates[scancode] = keystates[scancode] == KS_OFF ?
						KS_PRESSED : KS_REPEATING;
	    } else {
		keystates[scancode] = keystates[scancode] == KS_PRESSED ?
						KS_STRUCK : KS_OFF;
	    }
	}
	break;
    }
}

/* Initialize (or re-initialize) all key states.
 */
static void restartkeystates(void)
{
    uint8_t    *keyboard;
    int		count, n;

    memset(keystates, KS_OFF, sizeof keystates);
    keyboard = TW_GetKeyState(&count);
    if (count > TWK_LAST)
	count = TWK_LAST;
    for (n = 0 ; n < count ; ++n)
	if (keyboard[n])
	    _keyeventcallback(n, TRUE);
}

/* Update the key states. This is done at the start of each polling
 * cycle. The state changes that occur depend on the current behavior
 * settings.
 */
static void resetkeystates(void)
{
    /* The transition table for keys in joystick behavior mode.
     */
    static char const joystick_trans[KS_count] = {
	/* KS_OFF         => */	KS_OFF,
	/* KS_ON          => */	KS_ON,
	/* KS_DOWN        => */	KS_DOWN,
	/* KS_STRUCK      => */	KS_OFF,
	/* KS_PRESSED     => */	KS_DOWN,
	/* KS_DOWNBUTOFF1 => */	KS_DOWN,
	/* KS_DOWNBUTOFF2 => */	KS_DOWN,
	/* KS_DOWNBUTOFF3 => */	KS_DOWN,
	/* KS_REPEATING   => */	KS_DOWN
    };
    /* The transition table for keys in keyboard behavior mode.
     */
    static char const keyboard_trans[KS_count] = {
	/* KS_OFF         => */	KS_OFF,
	/* KS_ON          => */	KS_ON,
	/* KS_DOWN        => */	KS_DOWN,
	/* KS_STRUCK      => */	KS_OFF,
	/* KS_PRESSED     => */	KS_DOWNBUTOFF1,
	/* KS_DOWNBUTOFF1 => */	KS_DOWNBUTOFF2,
	/* KS_DOWNBUTOFF2 => */	KS_DOWN,
	/* KS_DOWNBUTOFF3 => */	KS_DOWN,
	/* KS_REPEATING   => */	KS_DOWN
    };

    char const *newstate;
    int		n;

    newstate = joystickstyle ? joystick_trans : keyboard_trans;
    for (n = 0 ; n < TWK_LAST ; ++n)
	keystates[n] = newstate[(int)keystates[n]];
}

/*
 * Mouse event functions.
 */

/* This callback is called whenever there is a state change in the
 * mouse buttons. Up events are ignored. Down events are stored to
 * be examined later.
 */
static void _mouseeventcallback(int xpos, int ypos, int button, int down)
{
    if (down) {
	mouseinfo.state = KS_PRESSED;
	mouseinfo.x = xpos;
	mouseinfo.y = ypos;
	mouseinfo.button = button;
    }
}

/* Return the command appropriate to the most recent mouse activity.
 */
static int retrievemousecommand(void)
{
    int	n;

    switch (mouseinfo.state) {
      case KS_PRESSED:
	mouseinfo.state = KS_OFF;
	if (mouseinfo.button == TW_BUTTON_WHEELDOWN)
	    return CmdNext;
	if (mouseinfo.button == TW_BUTTON_WHEELUP)
	    return CmdPrev;
	if (mouseinfo.button == TW_BUTTON_LEFT) {
	    n = windowmappos(mouseinfo.x, mouseinfo.y);
	    if (n >= 0) {
		mouseinfo.state = KS_DOWNBUTOFF1;
		return CmdAbsMouseMoveFirst + n;
	    }
	}
	break;
      case KS_DOWNBUTOFF1:
	mouseinfo.state = KS_DOWNBUTOFF2;
	return CmdPreserve;
      case KS_DOWNBUTOFF2:
	mouseinfo.state = KS_DOWNBUTOFF3;
	return CmdPreserve;
      case KS_DOWNBUTOFF3:
	mouseinfo.state = KS_OFF;
	return CmdPreserve;
    }
    return 0;
}

/*
 * Exported functions.
 */

/* Poll the keyboard and return the command associated with the
 * selected key, if any. If no key is selected and wait is TRUE, block
 * until a key with an associated command is selected. In keyboard behavior
 * mode, the function can return CmdPreserve, indicating that if the key
 * command from the previous poll has not been processed, it should still
 * be considered active. If two mergeable keys are selected, the return
 * value will be the bitwise-or of their command values.
 */
int input(int wait)
{
    keycmdmap const    *kc;
    int			lingerflag = FALSE;
    int			cmd1, cmd, n;

    for (;;) {
	resetkeystates();
	eventupdate(wait);

	cmd1 = cmd = 0;
	for (kc = keycmds ; kc->scancode ; ++kc) {
	    n = keystates[kc->scancode];
	    if (!n)
		continue;
	    if (kc->shift != -1)
		if (kc->shift !=
			(keystates[TWK_LSHIFT] || keystates[TWK_RSHIFT]))
		    continue;
	    if (kc->ctl != -1)
		if (kc->ctl !=
			(keystates[TWK_LCTRL] || keystates[TWK_RCTRL]))
		    continue;
	    if (kc->alt != -1)
		if (kc->alt != (keystates[TWK_LALT] || keystates[TWK_RALT]))
		    continue;

	    if (n == KS_PRESSED || (kc->hold && n == KS_DOWN)) {
		if (!cmd1) {
		    cmd1 = kc->cmd;
		    if (!joystickstyle || cmd1 > CmdKeyMoveLast
				       || !mergeable[cmd1])
			return cmd1;
		} else {
		    if (cmd1 <= CmdKeyMoveLast
				&& (mergeable[cmd1] & kc->cmd) == kc->cmd)
			return cmd1 | kc->cmd;
		}
	    } else if (n == KS_STRUCK || n == KS_REPEATING) {
		cmd = kc->cmd;
	    } else if (n == KS_DOWNBUTOFF1 || n == KS_DOWNBUTOFF2) {
		lingerflag = TRUE;
	    }
	}
	if (cmd1)
	    return cmd1;
	if (cmd)
	    return cmd;
	cmd = retrievemousecommand();
	if (cmd)
	    return cmd;
	if (!wait)
	    break;
    }
    if (!cmd && lingerflag)
	cmd = CmdPreserve;
    return cmd;
}

/* Turn joystick behavior mode on or off. In joystick-behavior mode,
 * the arrow keys are always returned from input() if they are down at
 * the time of the polling cycle. Other keys are only returned if they
 * are pressed during a polling cycle (or if they repeat, if keyboard
 * repeating is on). In keyboard-behavior mode, the arrow keys have a
 * special repeating behavior that is kept synchronized with the
 * polling cycle.
 */
int setkeyboardarrowsrepeat(int enable)
{
    joystickstyle = enable;
    restartkeystates();
    return TRUE;
}

/* Turn input mode on or off. When input mode is on, the input key
 * command map is used instead of the game key command map.
 */
int setkeyboardinputmode(int enable)
{
    keycmds = enable ? inputkeycmds : gamekeycmds;
    return TRUE;
}

/* Given a pixel's coordinates, return the integer identifying the
 * tile's position in the map, or -1 if the pixel is not on the map view.
 */
static int _windowmappos(int x, int y)
{
    if (geng.mapvieworigin < 0)
	return -1;
    if (x < geng.maploc.x || y < geng.maploc.y)
	return -1;
    x = (x - geng.maploc.x) * 4 / geng.wtile;
    y = (y - geng.maploc.y) * 4 / geng.htile;
    if (x >= NXTILES * 4 || y >= NYTILES * 4)
	return -1;
    x = (x + geng.mapvieworigin % (CXGRID * 4)) / 4;
    y = (y + geng.mapvieworigin / (CXGRID * 4)) / 4;
    if (x < 0 || x >= CXGRID || y < 0 || y >= CYGRID) {
	warn("mouse moved off the map: (%d %d)", x, y);
	return -1;
    }
    return y * CXGRID + x;
}

/* Initialization.
 */
int _genericinputinitialize(void)
{
    geng.keyeventcallbackfunc = _keyeventcallback;
    geng.mouseeventcallbackfunc = _mouseeventcallback;
    geng.windowmapposfunc = _windowmappos;

    mergeable[CmdNorth] = mergeable[CmdSouth] = CmdWest | CmdEast;
    mergeable[CmdWest] = mergeable[CmdEast] = CmdNorth | CmdSouth;

    setkeyboardrepeat(TRUE);
    return TRUE;
}

