/* play.cpp: Top-level game-playing functions.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag,
 * Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#include	<cstdlib>
#include	<cstring>

#include	"defs.h"
#include	"state.h"
#include	"encoding.h"
#include	"TWMainWnd.h"
#include	"res.h"
#include	"random.h"
#include	"solution.h"
#include	"play.h"
#include	"timer.h"
#include	"sdlsfx.h"
#include	"logic.h"
#include	"err.h"

/* The current state of the current game.
 */
static gamestate	state;

/* The current logic module.
 */
static gamelogic       *logic = NULL;

/* TRUE if the program is running without a user interface.
 */
bool			batchmode = false;

/* How much mud to make the timer suck (i.e., the slowdown factor).
 */
static int		mudsucking = 1;

/* Turn on the pedantry.
 */
void setpedanticmode(bool v)
{
	pedanticmode = v;
}

/* Configure the game logic, and some of the OS/hardware layer, as
 * required for the given ruleset. Do nothing if the requested ruleset
 * is already the current ruleset.
 */
static bool setrulesetbehavior(int ruleset)
{
	if (logic) {
		if (ruleset == logic->ruleset)
			return true;
		(*logic->shutdown)(logic);
		logic = NULL;
	}
	if (ruleset == Ruleset_None)
		return true;

	switch (ruleset) {
		case Ruleset_Lynx:
			logic = lynxlogicstartup();
			if (!logic)
				return false;
			if (!batchmode)
				g_pMainWnd->SetKeyboardArrowsRepeat(true);
			settimersecond(1000 * mudsucking);
			break;
		case Ruleset_MS:
			logic = mslogicstartup();
			if (!logic)
				return false;
			if (!batchmode)
				g_pMainWnd->SetKeyboardArrowsRepeat(false);
			settimersecond(1100 * mudsucking);
			break;
		default:
			warn("unknown ruleset requested (ruleset=%d)", ruleset);
			return false;
	}

	if (!batchmode) {
		loadgameresources(ruleset);
		g_pMainWnd->CreateGameDisplay();
	}

	logic->state = &state;
	return true;
}

/* Initialize the current state to the starting position of the
 * given level.
 */
bool initgamestate(gamesetup *game, int ruleset)
{
	if (!setrulesetbehavior(ruleset))
		die("unable to initialize the system for the requested ruleset");

	memset(state.map, 0, sizeof state.map);
	state.game = game;
	state.ruleset = ruleset;
	state.replay = -1;
	state.currenttime = -1;
	state.timeoffset = 0;
	state.currentinput = NIL;
	state.lastmove = NIL;
	state.initrndslidedir = NIL;
	state.stepping = -1;
	state.statusflags = 0;
	state.soundeffects = 0;
	state.timelimit = game->time * TICKS_PER_SECOND;
	initmovelist(&state.moves);
	resetprng(&state.mainprng);

	if (!expandleveldata(&state))
		return false;

	return (*logic->initgame)(logic);
}

/* Change the current state to run from the recorded solution.
 */
bool prepareplayback(void)
{
	solutioninfo	solution;

	if (!state.game->solutionsize)
		return false;
	solution.moves.list = NULL;
	solution.moves.allocated = 0;
	if (!expandsolution(&solution, state.game) || !solution.moves.count)
		return false;

	destroymovelist(&state.moves);
	state.moves = solution.moves;
	restartprng(&state.mainprng, solution.rndseed);
	state.initrndslidedir = solution.rndslidedir;
	state.stepping = solution.stepping;
	state.replay = 0;
	return true;
}

/* Return the amount of time passed in the current game, in seconds.
 */
int secondsplayed(void)
{
	return (state.currenttime + state.timeoffset) / TICKS_PER_SECOND;
}

/* Change the system behavior according to the given gameplay mode.
 */
void setgameplaymode(int mode)
{
	switch (mode) {
		case NormalPlay:
			g_pMainWnd->SetKeyboardRepeat(false);
			settimer(+1);
			setsoundeffects(+1);
			state.statusflags &= ~SF_SHUTTERED;
			break;
		case EndPlay:
			g_pMainWnd->SetKeyboardRepeat(true);
			settimer(-1);
			setsoundeffects(+1);
			break;
		case NonrenderPlay:
			settimer(+1);
			setsoundeffects(0);
			break;
		case SuspendPlayShuttered:
			if (state.ruleset == Ruleset_MS)
				state.statusflags |= SF_SHUTTERED;
		case SuspendPlay:
			g_pMainWnd->SetKeyboardRepeat(true);
			settimer(0);
			setsoundeffects(0);
			break;
	}
}

/* Alter the stepping. Force the stepping to be appropriate
 * to the current ruleset.
 */
void setstepping(int step)
{
	if(state.ruleset == Ruleset_MS) {
		if(step > 3) step = 4;
		else step = 0;
	} else {
		if(step < 0) step = 0;
		else if(step > 7) step = 7;
	}

	state.stepping = step;
}

int getstepping()
{
	return state.stepping;
}

/* Advance the game one tick and update the game state. cmd is the
 * current keyboard command supplied by the user. The return value is
 * positive if the game was completed successfully, negative if the
 * game ended unsuccessfully, and zero otherwise.
 */
int doturn(int cmd)
{
	action	act;
	int		n;

	state.soundeffects &= ~((1 << SND_ONESHOT_COUNT) - 1);
	state.currenttime = gettickcount();
	if (state.currenttime >= MAXIMUM_TICK_COUNT) {
		warn("timer reached its maximum of %d.%d hours; quitting now",
			MAXIMUM_TICK_COUNT / (TICKS_PER_SECOND * 3600),
			(MAXIMUM_TICK_COUNT / (TICKS_PER_SECOND * 360)) % 10);
		return -1;
	}
	if (state.replay < 0) {
		if (cmd != CmdPreserve)
			state.currentinput = cmd;
	} else {
		if (state.replay < state.moves.count) {
			if (state.currenttime > state.moves.list[state.replay].when)
				warn("Replay: Got ahead of saved solution: %d > %d!",
					state.currenttime, state.moves.list[state.replay].when);
			if (state.currenttime == state.moves.list[state.replay].when) {
				state.currentinput = state.moves.list[state.replay].dir;
				++state.replay;
			}
		} else {
			n = state.currenttime + state.timeoffset - 1;
			if (n > state.game->besttime)
				return -1;
		}
	}

	n = (*logic->advancegame)(logic);

	if (state.replay < 0 && state.lastmove) {
		act.when = state.currenttime;
		act.dir = state.lastmove;
		addtomovelist(&state.moves, act);
		state.lastmove = NIL;
	}

	return n;
}

/* Update the display to show the current game state (including sound
 * effects, if any). If showframe is FALSE, then nothing is actually
 * displayed.
 */
void drawscreen(bool showframe)
{
	int	currenttime;
	int timeleft, besttime;

	playsoundeffects(state.soundeffects);
	state.soundeffects &= ~((1 << SND_ONESHOT_COUNT) - 1);

	if (!showframe)
		return;

	currenttime = state.currenttime + state.timeoffset;

	int const starttime = (state.game->time ? state.game->time : 999);
	if (hassolution(state.game))
		besttime = starttime - state.game->besttime / TICKS_PER_SECOND;
	else
		besttime = TIME_NIL;

	timeleft = starttime - currenttime / TICKS_PER_SECOND;
	if (state.game->time && timeleft <= 0) {
		timeleft = 0;
	}

	g_pMainWnd->DisplayGame(&state, timeleft, besttime);
}

/* Stop game play and clean up.
 */
void quitgamestate(void)
{
	state.soundeffects = 0;
	setsoundeffects(-1);
}

/* Clean up after game play is over.
 */
bool endgamestate()
{
	setsoundeffects(-1);
	return (*logic->endgame)(logic);
}

/* Close up shop.
 */
void shutdowngamestate(void)
{
	setrulesetbehavior(Ruleset_None);
	destroymovelist(&state.moves);
}

/* Initialize the current game state to a small level used for display
 * at the completion of a series.
 */
void setenddisplay(void)
{
	state.replay = -1;
	state.timelimit = 0;
	state.currenttime = -1;
	state.timeoffset = 0;
	state.chipsneeded = 0;
	state.currentinput = NIL;
	state.statusflags = 0;
	state.soundeffects = 0;
	getenddisplaysetup(&state);
	(*logic->initgame)(logic);
}

/*
 * Solution handling functions.
 */

/* Return TRUE if a solution exists for the given level.
 */
bool hassolution(gamesetup const *game)
{
	return game->besttime != TIME_NIL;
}

/* Compare the most recent solution for the current game with the
 * user's best solution (if any). If this solution beats what's there,
 * or if the current solution has been marked as replaceable, then
 * replace it. TRUE is returned if the solution was replaced.
 */
bool replacesolution(void)
{
	solutioninfo	solution;
	int			currenttime;

	if (state.statusflags & SF_NOSAVING)
		return false;
	currenttime = state.currenttime + state.timeoffset;
	if (hassolution(state.game) && !(state.game->sgflags & SGF_REPLACEABLE)
		&& currenttime >= state.game->besttime)
		return false;

	state.game->besttime = currenttime;
	state.game->sgflags &= ~SGF_REPLACEABLE;
	solution.moves = state.moves;
	solution.rndseed = getinitialseed(&state.mainprng);
	solution.flags = 0;
	solution.rndslidedir = state.initrndslidedir;
	solution.stepping = state.stepping;
	if (!contractsolution(&solution, state.game))
		return false;

	return true;
}

/* Double-checks the timing for a solution that has just been played
 * back. If the timing is off, and the cause of the discrepancy can be
 * reasonably ascertained to be benign, the timing will be corrected
 * and TRUE is returned.
 */
bool checksolution(void)
{
	int	currenttime;

	if (!hassolution(state.game))
		return false;
	currenttime = state.currenttime + state.timeoffset;
	if (currenttime == state.game->besttime)
		return false;
	warn("saved game has solution time of %d ticks, but replay took %d ticks",
		state.game->besttime, currenttime);
	if (state.game->besttime == state.currenttime) {
		warn("difference matches clock offset; fixing.");
		state.game->besttime = currenttime;
		return true;
	} else if (currenttime - state.game->besttime == 1) {
		warn("difference matches pre-0.10.1 error; fixing.");
		state.game->besttime = currenttime;
		return true;
	}
	warn("reason for difference unknown.");
	state.game->besttime = currenttime;
	return false;
}
