/* tworld.cpp: The top-level module.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag,
 * Eric Schmidt and Michael Walsh
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"tworld.h"
#include	"defs.h"
#include	"err.h"
#include	"series.h"
#include	"play.h"
#include	"score.h"
#include	"settings.h"
#include	"solution.h"
#include	"oshw.h"
#include	"fileio.h"
#include	"timer.h"
#include	"sdlsfx.h"

/* History of levelsets in order of last used date/time.
 */
static history *historylist = NULL;
static int	historycount = 0;

/* FALSE suppresses all password checking.
 */
static int	usepasswds = TRUE;

/* Frame-skipping disable flag.
 */
static int	noframeskip = FALSE;

/*
 * Basic game activities.
 */

/* Return TRUE if the given level is a final level.
 */
static int islastinseries(gamespec const *gs, int index)
{
	return index == gs->series.count - 1
		|| gs->series.games[index].number == gs->series.final;
}

/* Return TRUE if the current level has a solution.
 */
static int issolved(gamespec const *gs, int index)
{
	return hassolution(gs->series.games + index);
}

/* Mark the current level's solution as replaceable.
 */
static void replaceablesolution(gamespec *gs, int change)
{
	if (change < 0)			// toggle
		gs->series.games[gs->currentgame].sgflags ^= SGF_REPLACEABLE;
	else if (change > 0)	// set
		gs->series.games[gs->currentgame].sgflags |= SGF_REPLACEABLE;
	else					// unset
		gs->series.games[gs->currentgame].sgflags &= ~SGF_REPLACEABLE;
}

/* Mark the current level's password as known to the user.
 */
static void passwordseen(gamespec *gs, int number)
{
	if (!(gs->series.games[number].sgflags & SGF_HASPASSWD)) {
		gs->series.games[number].sgflags |= SGF_HASPASSWD;
		savesolutions(&gs->series);
	}
}

/* Change the current level, ensuring that the user is not granted
 * access to a forbidden level. FALSE is returned if the specified
 * level is not available to the user.
 */
static int setcurrentgame(gamespec *gs, int n)
{
	if (n == gs->currentgame)
		return TRUE;
	if (n < 0 || n >= gs->series.count)
		return FALSE;

	if (gs->usepasswds)
		if (n > 0 && !(gs->series.games[n].sgflags & SGF_HASPASSWD)
			&& !issolved(gs, n -1))
			return FALSE;

	gs->currentgame = n;
	gs->melindacount = 0;
	return TRUE;
}

/* Change the current level by a delta value. If the user cannot go to
 * that level, the "nearest" level in that direction is chosen
 * instead. FALSE is returned if the current level remained unchanged.
 */
static int changecurrentgame(gamespec *gs, int offset)
{
	if (offset == 0)
		return FALSE;

	int m = gs->currentgame;
	int n = m + offset;
	if (n < 0)
		n = 0;
	else if (n >= gs->series.count)
		n = gs->series.count - 1;

	if (gs->usepasswds && n > 0) {
		int sign = offset < 0 ? -1 : +1;
		for ( ; n >= 0 && n < gs->series.count ; n += sign) {
			if (!n || (gs->series.games[n].sgflags & SGF_HASPASSWD)
				|| issolved(gs, n - 1)) {
				m = n;
				break;
			}
		}
		n = m;
		if (n == gs->currentgame && offset != sign) {
			n = gs->currentgame + offset - sign;
			for ( ; n != gs->currentgame ; n -= sign) {
				if (n < 0 || n >= gs->series.count)
					continue;
				if (!n || (gs->series.games[n].sgflags & SGF_HASPASSWD)
					|| issolved(gs, n - 1))
					break;
			}
		}
	}

	if (n == gs->currentgame)
		return FALSE;

	gs->currentgame = n;
	gs->melindacount = 0;
	return TRUE;
}

/* Return TRUE if Melinda is watching Chip's progress on this level --
 * i.e., if it is possible to earn a pass to the next level.
 */
static int melindawatching(gamespec const *gs)
{
	if (!gs->usepasswds)
		return FALSE;
	if (islastinseries(gs, gs->currentgame))
		return FALSE;
	if (gs->series.games[gs->currentgame + 1].sgflags & SGF_HASPASSWD)
		return FALSE;
	if (issolved(gs, gs->currentgame))
		return FALSE;
	return TRUE;
}


/* Display a scrolling list of the available solution files, and allow
 * the user to select one. Return TRUE if the user selected a solution
 * file different from the current one. Do nothing if there is only
 * one solution file available. (If for some reason the new solution
 * file cannot be read, TRUE will still be returned, as the list of
 * solved levels will still need to be updated.)
 */
static int showsolutionfiles(gamespec *gs)
{
	tablespec		table;
	char const	      **filelist;
	int			count, current, f, n;

	if (haspathname(gs->series.name) || (gs->series.savefilename
			&& haspathname(gs->series.savefilename))) {
		bell();
		return FALSE;
	} else if (!createsolutionfilelist(&gs->series, FALSE, &filelist,
			&count, &table)) {
		bell();
		return FALSE;
	}

	current = -1;
	n = 0;
	if (gs->series.savefilename) {
		for (n = 0 ; n < count ; ++n)
			if (!strcmp(filelist[n], gs->series.savefilename))
				break;
		if (n == count)
			n = 0;
		else
			current = n;
	}

	pushsubtitle(gs->series.name);
	for (;;) {
		f = displaylist(&table, &n, LIST_SOLUTIONFILES);
		if (f == CmdProceed) {
			break;
		} else if (f == CmdQuitLevel) {
			n = -1;
			break;
		}
	}
	popsubtitle();

	f = n >= 0 && n != current;
	if (f) {
		clearsolutions(&gs->series);
		int l = (sizeof(char*) * strlen(filelist[n])) + 1;
		x_cmalloc(gs->series.savefilename, l);
		strcpy(gs->series.savefilename, filelist[n]);
		if (!readsolutions(&gs->series)) {
			bell();
		}
		n = gs->currentgame;
		gs->currentgame = 0;
		passwordseen(gs, 0);
		changecurrentgame(gs, n);
	}

	freesolutionfilelist(filelist, &table);
	return f;
}

/* Display the scrolling list of the user's current scores, and allow
 * the user to select a current level.
 */
static int showscores(gamespec *gs)
{
	tablespec	table;
	int	       *levellist;
	int		ret = FALSE;
	int		count, f, n;

restart:
	if (!createscorelist(&gs->series, gs->usepasswds, &levellist,
			&count, &table)) {
		bell();
		return ret;
	}
	for (n = 0 ; n < count ; ++n)
		if (levellist[n] == gs->currentgame)
			break;
	pushsubtitle(gs->series.name);
	for (;;) {
		f = displaylist(&table, &n, LIST_SCORES);
		if (f == CmdProceed) {
			n = levellist[n];
			break;
		} else if (f == CmdSeeSolutionFiles) {
			if (!(gs->series.gsflags & GSF_NODEFAULTSAVE)) {
				n = levellist[n];
				break;
			}
		} else if (f == CmdQuitLevel) {
			n = -1;
			break;
		}
	}
	popsubtitle();
	freescorelist(levellist, &table);
	if (f == CmdSeeSolutionFiles) {
		setcurrentgame(gs, n);
		ret = showsolutionfiles(gs);
		goto restart;
	}
	if (n < 0)
		return ret;
	return setcurrentgame(gs, n) || ret;
}

/* Obtain a password from the user and move to the requested level.
 */
static int selectlevelbypassword(gamespec *gs)
{
	int		n;

	const char *passwd = displaypasswordprompt();

	if (strlen(passwd) != 4) return FALSE;

	n = findlevelinseries(&gs->series, 0, passwd);
	if (n < 0) {
		bell();
		return FALSE;
	}
	passwordseen(gs, n);
	return setcurrentgame(gs, n);
}

/*
 * The levelset history functions.
 */

/* Load the levelset history.
 */
int loadhistory(void)
{
	fileinfo	file;
	char	buf[256];
	int		n;
	char       *hdate, *htime, *hpasswd, *hnumber, *hname;
	int		hyear, hmon, hmday, hhour, hmin, hsec;
	history    *h;

	historycount = 0;
	free(historylist);

	clearfileinfo(&file);
	if (!openfileindir(&file, SETTINGSDIR, "history", "r", NULL))
		return FALSE;

	for (;;) {
		n = sizeof buf - 1;
		if (!filegetline(&file, buf, &n, NULL))
			break;

		if (buf[0] == '#')
			continue;

		hdate	= strtok(buf , " \t");
		htime	= strtok(NULL, " \t");
		hpasswd	= strtok(NULL, " \t");
		hnumber	= strtok(NULL, " \t");
		hname	= strtok(NULL, "\r\n");

		if ( ! (hdate && htime && hpasswd && hnumber && hname  &&
				sscanf(hdate, "%4d-%2d-%2d", &hyear, &hmon, &hmday) == 3  &&
				sscanf(htime, "%2d:%2d:%2d", &hhour, &hmin, &hsec) == 3  &&
				*hpasswd  && *hnumber && *hname) )
			continue;

		++historycount;
		x_type_alloc(history, historylist, historycount * sizeof *historylist);
		h = historylist + historycount - 1;

		sprintf(h->name, "%.*s", (int)(sizeof h->name - 1), hname);
		sprintf(h->passwd, "%.*s", (int)(sizeof h->passwd - 1), hpasswd);
		h->levelnumber = (int)strtol(hnumber, NULL, 0);
		h->dt.tm_year  = hyear - 1900;
		h->dt.tm_mon   = hmon - 1;
		h->dt.tm_mday  = hmday;
		h->dt.tm_hour  = hhour;
		h->dt.tm_min   = hmin;
		h->dt.tm_sec   = hsec;
		h->dt.tm_isdst = -1;
	}

	fileclose(&file, NULL);

	return TRUE;
}

/* Update the levelset history for the set and level being played.
 */
static void updatehistory(char const *name, char const *passwd, int number)
{
	time_t	t = time(NULL);
	int		i, j;
	history    *h;

	h = historylist;
	for (i = 0; i < historycount; ++i, ++h) {
		if (strcasecmp(h->name, name) == 0)
			break;
	}

	if (i == historycount) {
		++historycount;
		x_type_alloc(history, historylist, historycount * sizeof *historylist);
	}

	for (j = i; j > 0; --j) {
		historylist[j] = historylist[j-1];
	}

	h = historylist;
	sprintf(h->name, "%.*s", (int)(sizeof h->name - 1), name);
	sprintf(h->passwd, "%.*s", (int)(sizeof h->passwd - 1), passwd);
	h->levelnumber = number;
	h->dt = *localtime(&t);
}

/* Save the levelset history.
 */
void savehistory(void)
{
	fileinfo	file;
	history    *h;
	int		i;

	clearfileinfo(&file);
	if (!openfileindir(&file, SETTINGSDIR, "history", "w", NULL))
		return;

	h = historylist;
	for (i = 0; i < historycount; ++i, ++h) {
		fprintf(file.fp, "%04d-%02d-%02d %02d:%02d:%02d\t%s\t%d\t%s\n",
			1900 + h->dt.tm_year, 1 + h->dt.tm_mon, h->dt.tm_mday,
			h->dt.tm_hour, h->dt.tm_min, h->dt.tm_sec,
			h->passwd, h->levelnumber, h->name);
	}

	fileclose(&file, NULL);
}

/*
 * The game-playing functions.
 */

#define	leveldelta(n)	if (!changecurrentgame(gs, (n))) { bell(); continue; }

/* Get a key command from the user at the start of the current level.
 */
static int startinput(gamespec *gs)
{
	static int	lastlevel = -1;

	if (gs->currentgame != lastlevel) {
		lastlevel = gs->currentgame;
		setstepping(0);
	}
	drawscreen(TRUE);
	gs->playmode = Play_None;
	for (;;) {
		int cmd = input(TRUE);
		if (cmd >= CmdMoveFirst && cmd <= CmdMoveLast) {
			gs->playmode = Play_Normal;
			return cmd;
		}
		switch (cmd) {
		case CmdPauseGame:
		case CmdProceed:	gs->playmode = Play_Normal;	return CmdProceed;
		case CmdQuitLevel:					return cmd;
		case CmdPrevLevel:	leveldelta(-1);			return CmdNone;
		case CmdNextLevel:	leveldelta(+1);			return CmdNone;
		case CmdQuit:						exit(0);
		case CmdPlayback:
			if (prepareplayback()) {
				gs->playmode = Play_Back;
				return cmd;
			}
			bell();
			break;
		case CmdSeek:
			if (getreplaysecondstoskip() > 0) {
				gs->playmode = Play_Back;
				return CmdProceed;
			}
			break;
		case CmdCheckSolution:
			if (prepareplayback()) {
				gs->playmode = Play_Verify;
				return CmdProceed;
			}
			bell();
			break;
		case CmdDelSolution:
			if (issolved(gs, gs->currentgame)) {
				replaceablesolution(gs, -1);
				savesolutions(&gs->series);
			} else {
				bell();
			}
			break;
		case CmdSeeScores:
			if (showscores(gs))
				return CmdNone;
			break;
		case CmdSeeSolutionFiles:
			if (showsolutionfiles(gs))
				return CmdNone;
			break;
		case CmdTimesClipboard:
			copytoclipboard(leveltimes(&gs->series));
			break;
		case CmdGotoLevel:
			if (selectlevelbypassword(gs))
				return CmdNone;
		default:
			continue;
		}
		drawscreen(TRUE);
	}
}

/* Get a key command from the user at the completion of the current
 * level.
 */
static int endinput(gamespec *gs)
{
	int		bscore = 0, tscore = 0;
	long	gscore = 0;
	int		cmd = CmdNone;

	if (gs->status < 0) {
		if (melindawatching(gs) && secondsplayed() >= 10) {
			++gs->melindacount;
			if (gs->melindacount >= 10) {
				if (displayyesnoprompt("Skip level?")) {
					passwordseen(gs, gs->currentgame + 1);
					changecurrentgame(gs, +1);
				}
				gs->melindacount = 0;
				return TRUE;
			}
		}
	} else {
		getscoresforlevel(&gs->series, gs->currentgame,
			&bscore, &tscore, &gscore);
	}

	cmd = displayendmessage(bscore, tscore, gscore, gs->status);

	for (;;) {
		if (cmd == CmdNone)
			cmd = input(TRUE);
		switch (cmd) {
		case CmdPrevLevel:	changecurrentgame(gs, -1);	return TRUE;
		case CmdSameLevel:								return TRUE;
		case CmdNextLevel:	changecurrentgame(gs, +1);	return TRUE;
		case CmdGotoLevel:	selectlevelbypassword(gs);	return TRUE;
		case CmdPlayback:									return TRUE;
		case CmdSeeScores:	showscores(gs);				return TRUE;
		case CmdSeeSolutionFiles: showsolutionfiles(gs);	return TRUE;
		case CmdQuitLevel:					return FALSE;
		case CmdQuit:						exit(0);
		case CmdCheckSolution:
		case CmdProceed:
			if (gs->status > 0) {
				if (islastinseries(gs, gs->currentgame))
					gs->enddisplay = TRUE;
				else
					changecurrentgame(gs, +1);
			}
			return TRUE;
		case CmdDelSolution:
			if (issolved(gs, gs->currentgame)) {
				replaceablesolution(gs, -1);
				savesolutions(&gs->series);
			} else {
				bell();
			}
			return TRUE;
		}
		cmd = CmdNone;
	}
}

/* Get a key command from the user at the completion of the current
 * series.
 */
static int finalinput(gamespec *gs)
{
	for (;;) {
		int cmd = input(TRUE);
		switch (cmd) {
		case CmdSameLevel:
			return TRUE;
		case CmdPrevLevel:
		case CmdNextLevel:
			setcurrentgame(gs, 0);
			return TRUE;
		case CmdQuit:
			exit(0);
		default:
			return FALSE;
		}
	}
}

#define SETPAUSED(paused, shutter) do { \
	if(!paused) { \
		setgameplaymode(NormalPlay); \
		gamepaused = FALSE; \
	} else if(shutter) { \
		setgameplaymode(SuspendPlayShuttered); \
		drawscreen(TRUE); \
		gamepaused = TRUE; \
	} else { \
		setgameplaymode(SuspendPlay); \
		gamepaused = TRUE; \
	} \
	setplaypausebutton(gamepaused); \
} while (0)

/* Play the current level, using firstcmd as the initial key command,
 * and returning when the level's play ends. The return value is FALSE
 * if play ended because the user restarted or changed the current
 * level (indicating that the program should not prompt the user
 * before continuing). If the return value is TRUE, the gamespec
 * structure's status field will contain the return value of the last
 * call to doturn() -- i.e., positive if the level was completed
 * successfully, negative if the level ended unsuccessfully. Likewise,
 * the gamespec structure will be updated if the user ended play by
 * changing the current level.
 */
static int playgame(gamespec *gs, int firstcmd)
{
	int	render, lastrendered;
	int	cmd, n;

	cmd = firstcmd;
	if (cmd == CmdProceed)
		cmd = CmdNone;

	gs->status = 0;
	setgameplaymode(NormalPlay);
	render = lastrendered = TRUE;

	int gamepaused = FALSE;
	setplaypausebutton(gamepaused);
	for (;;) {
		if (gamepaused)
			cmd = input(TRUE);
		else {
			n = doturn(cmd);
			drawscreen(render);
			lastrendered = render;
			if (n)
				break;
			render = waitfortick() || noframeskip;
			cmd = input(FALSE);
		}
		if (cmd == CmdQuitLevel) {
			quitgamestate();
			n = -2;
			break;
		}
		if (!(cmd >= CmdMoveFirst && cmd <= CmdMoveLast)) {
			switch (cmd) {
			case CmdPreserve:					break;
			case CmdPrevLevel:		n = -1;		goto quitloop;
			case CmdNextLevel:		n = +1;		goto quitloop;
			case CmdSameLevel:		n = 0;		goto quitloop;
			case CmdQuit:					exit(0);
			case CmdPauseGame:
				SETPAUSED(!gamepaused, TRUE);
				if (!gamepaused)
					cmd = CmdNone;
				break;
#ifndef NDEBUG
			case CmdDebugCmd1:				break;
			case CmdDebugCmd2:				break;
			case CmdCheatNorth:     case CmdCheatWest:	break;
			case CmdCheatSouth:     case CmdCheatEast:	break;
			case CmdCheatHome:				break;
			case CmdCheatKeyRed:    case CmdCheatKeyBlue:	break;
			case CmdCheatKeyYellow: case CmdCheatKeyGreen:	break;
			case CmdCheatBootsIce:  case CmdCheatBootsSlide:	break;
			case CmdCheatBootsFire: case CmdCheatBootsWater:	break;
			case CmdCheatICChip:				break;
#endif
			default:
				cmd = CmdNone;
				break;
			}
		}
	}
	if (!lastrendered)
		drawscreen(TRUE);
	setgameplaymode(EndPlay);
	if (n > 0)
		if (replacesolution())
			savesolutions(&gs->series);
	gs->status = n;
	return TRUE;

quitloop:
	if (!lastrendered)
		drawscreen(TRUE);
	quitgamestate();
	setgameplaymode(EndPlay);
	if (n)
		changecurrentgame(gs, n);
	return FALSE;
}

/* Skip past secondstoskip seconds from the beginning of the solution.
 */
static int hideandseek(gamespec *gs, int secondstoskip)
{
	int n = 0;

	quitgamestate();
	setgameplaymode(EndPlay);
	gs->playmode = Play_None;
	endgamestate();
	initgamestate(gs->series.games + gs->currentgame,
		gs->series.ruleset);
	prepareplayback();
	gs->playmode = Play_Back;
	gs->status = 0;
	setgameplaymode(NonrenderPlay);

	while (secondsplayed() < secondstoskip) {
		n = doturn(CmdNone);
		if (n)
			break;
		advancetick();
	}
	drawscreen(TRUE);
	setsoundeffects(-1);
	setgameplaymode(NormalPlay);

	return n;
}

/* Play back the user's best solution for the current level in real
 * time. Other than the fact that this function runs from a
 * prerecorded series of moves, it has the same behavior as
 * playgame().
 */
static int playbackgame(gamespec *gs)
{
	int	render, lastrendered, n = 0, cmd;
	int secondstoskip;
	int gamepaused = FALSE;
	setplaypausebutton(gamepaused);

	secondstoskip = getreplaysecondstoskip();
	if (secondstoskip > 0) {
		n = hideandseek(gs, secondstoskip);
		SETPAUSED(TRUE, FALSE);
	} else {
		drawscreen(TRUE);
		gs->status = 0;
		setgameplaymode(NormalPlay);
	}

	render = lastrendered = TRUE;

	while (!n) {
		if (gamepaused) {
			setgameplaymode(SuspendPlay);
			cmd = input(TRUE);
		} else {
			n = doturn(CmdNone);
			drawscreen(render);
			lastrendered = render;
			if (n)
				break;
			render = waitfortick() || noframeskip;
			cmd = input(FALSE);
		}
		switch (cmd) {
		case CmdSeek:
		case CmdWest:
		case CmdEast:
			if (cmd == CmdSeek) {
				secondstoskip = getreplaysecondstoskip();
			} else {
				secondstoskip = secondsplayed() + ((cmd == CmdEast) ? +3 : -3);
			}
			n = hideandseek(gs, secondstoskip);
			lastrendered = TRUE;
			break;
		case CmdPrevLevel:	changecurrentgame(gs, -1);	goto quitloop;
		case CmdNextLevel:	changecurrentgame(gs, +1);	goto quitloop;
		case CmdSameLevel:
		case CmdPlayback:
		case CmdQuitLevel:	goto quitloop;
		case CmdQuit:			exit(0);
		case CmdPauseGame:
			SETPAUSED(!gamepaused, FALSE);
			break;
		}
	}
	if (!lastrendered)
		drawscreen(TRUE);
	setgameplaymode(EndPlay);
	gs->playmode = Play_None;
	if (n < 0)
		replaceablesolution(gs, +1);
	if (n > 0) {
		if (checksolution())
			savesolutions(&gs->series);
	}
	gs->status = n;
	return TRUE;

quitloop:
	if (!lastrendered)
		drawscreen(TRUE);
	quitgamestate();
	setgameplaymode(EndPlay);
	gs->playmode = Play_None;
	return FALSE;
}

#undef SETPAUSED

/* Quickly play back the user's best solution for the current level
 * without rendering and without using the timer the keyboard. The
 * playback stops when the solution is finished or gameplay has
 * ended.
 */
static int verifyplayback(gamespec *gs)
{
	int	n;

	gs->status = 0;
	setgameplaymode(NonrenderPlay);
	for (;;) {
		n = doturn(CmdNone);
		if (n)
			break;
		advancetick();
		switch (input(FALSE)) {
		case CmdPrevLevel:	changecurrentgame(gs, -1);	goto quitloop;
		case CmdNextLevel:	changecurrentgame(gs, +1);	goto quitloop;
		case CmdSameLevel:					goto quitloop;
		case CmdPlayback:					goto quitloop;
		case CmdQuitLevel:					goto quitloop;
		case CmdQuit:						exit(0);
		}
	}
	gs->playmode = Play_None;
	quitgamestate();
	drawscreen(TRUE);
	setgameplaymode(EndPlay);
	if (n < 0) {
		replaceablesolution(gs, +1);
	}
	if (n > 0) {
		if (checksolution())
			savesolutions(&gs->series);
	}
	gs->status = n;
	return TRUE;

quitloop:
	gs->playmode = Play_None;
	setgameplaymode(EndPlay);
	return FALSE;
}

/* Manage a single session of playing the current level, from start to
 * finish. A return value of FALSE indicates that the user is done
 * playing levels from the current series; otherwise, the gamespec
 * structure is updated as necessary upon return.
 */
static int runcurrentlevel(gamespec *gs)
{
	int ret = TRUE;
	int	cmd;
	int	valid, f;
	char const *name;

	setplaypausebutton(TRUE);

	name = gs->series.name;

	updatehistory(name,
		gs->series.games[gs->currentgame].passwd,
		gs->series.games[gs->currentgame].number);

	if (gs->enddisplay) {
		gs->enddisplay = FALSE;
		changesubtitle(NULL);
		setenddisplay();
		drawscreen(TRUE);
		displayendmessage(0, 0, 0, 0);
		endgamestate();
		return finalinput(gs);
	}

	valid = initgamestate(gs->series.games + gs->currentgame,
		gs->series.ruleset);
	changesubtitle(gs->series.games[gs->currentgame].name);
	passwordseen(gs, gs->currentgame);
	if (!islastinseries(gs, gs->currentgame))
		if (!valid || gs->series.games[gs->currentgame].unsolvable)
			passwordseen(gs, gs->currentgame + 1);

	cmd = startinput(gs);

	if (cmd == CmdQuitLevel) {
		ret = FALSE;
	} else {
		if (cmd != CmdNone) {
			if (valid) {
				switch (gs->playmode) {
				case Play_Normal:	f = playgame(gs, cmd);		break;
				case Play_Back:	f = playbackgame(gs);	break;
				case Play_Verify:	f = verifyplayback(gs);		break;
				default:			f = FALSE;			break;
				}
				if (f)
					ret = endinput(gs);
			} else
				bell();
		}
	}

	endgamestate();
	return ret;
}

// POSSIBLE TODO rewrite with GUI
/*static int batchverify(gameseries *series, int display)
{
gamesetup  *game;
int		valid = 0, invalid = 0;
int		i, f;

batchmode = TRUE;

for (i = 0, game = series->games ; i < series->count ; ++i, ++game) {
	if (!hassolution(game))
	continue;
	if (initgamestate(game, series->ruleset) && prepareplayback()) {
	setgameplaymode(NonrenderPlay);
	while (!(f = doturn(CmdNone)))
		advancetick();
	setgameplaymode(EndPlay);
	if (f > 0) {
		++valid;
		checksolution();
	} else {
		++invalid;
		game->sgflags |= SGF_REPLACEABLE;
		if (display)
		printf("Solution for level %d is invalid\n", game->number);
	}
	}
	endgamestate();
}

if (display) {
	if (valid + invalid == 0) {
	printf("No solutions were found.\n");
	} else {
	printf("  Valid solutions:%4d\n", valid);
	printf("Invalid solutions:%4d\n", invalid);
	}
}
return invalid;
}*/

/*
 * Game selection functions
 */

/* Set the current level to that specified in the history. */
static void findlevelfromhistory(gamespec *gs, char const *name)
{
	int i, n;
	history *h;

	h = historylist;
	for (i = 0; i < historycount; ++i, ++h) {
		if (strcasecmp(h->name, name) == 0) {
			n = findlevelinseries(&gs->series, h->levelnumber, h->passwd);
			if (n < 0)
				n = findlevelinseries(&gs->series, 0, h->passwd);
			if (n >= 0) {
				gs->currentgame = n;
				if (gs->usepasswds && !(gs->series.games[n].sgflags & SGF_HASPASSWD))
					changecurrentgame(gs, -1);
			}
			break;
		}
	}
}

#define PRODUCE_SINGLE_COLUMN_TABLE(table, heading, data, count, L, R) do { \
size_t _alloc = 0; \
_alloc += 3 + strlen(heading); \
for (int _n = 0; _n < (count); ++_n) \
	_alloc += 3 + strlen(L(data)[_n] R); \
char const **_ptrs = (const char **)malloc(((count) + 1) * sizeof *_ptrs); \
char *_textheap = (char *)malloc(_alloc); \
if (!_ptrs || !_textheap) memerrexit(); \
\
int _n = 0; \
int _used = 0; \
_ptrs[_n++] = _textheap + _used; \
_used += 1 + sprintf(_textheap + _used, "1-" heading); \
\
for (int _y = 0 ; _y < (count) ; ++_y) { \
_ptrs[_n++] = _textheap + _used; \
	_used += 1 + sprintf(_textheap + _used, \
			"1-%s", L(data)[_y] R); \
} \
(table).rows = (count) + 1; \
(table).cols = 1; \
(table).items = _ptrs; \
} while (0)

/* Free a table generated by the preceding macro */
static void free_table(tablespec * table)
{
	free((void*)table->items[0]);
	free(table->items);
}

/* Determine the index in series->mflist where the gameseries with index idx
 * is found. Returns 0 if there is no such index. */
static int findseries(seriesdata *series, int idx)
{
	for (int n = 0; n < series->mfcount; ++n) {
		mapfileinfo *mfi = &series->mflist[n];
		for (int i = Ruleset_First; i < Ruleset_Count; ++i) {
			intlist *gsl = &mfi->sfilelst[i];
			for (int j = 0; j < gsl->count; ++j) {
				if (gsl->list[j] == idx)
					return n;
			}
		}
	}
	return 0;
}

/* Helper function for selectseriesandlevel */
static int chooseseries(seriesdata *series, int *pn, int founddefault)
{
	tablespec mftable;
	PRODUCE_SINGLE_COLUMN_TABLE(mftable, "Levelset",
		series->mflist, series->mfcount, , .filename);

	/* Choose mapfile to be selected by default */
	int n = (founddefault ? findseries(series, *pn) : 0);

	int chosenseries = -1;
	while (chosenseries < 0) {
		int f = displaylist(&mftable, &n, LIST_MAPFILES);
		if (f != CmdProceed) {
			free_table(&mftable);
			return f;
		}
		int ruleset = getselectedruleset();
		intlist *chosengsl = &series->mflist[n].sfilelst[ruleset];

		if (chosengsl->count < 1) /* Can happen if .dac name was taken */
			continue;
		if (chosengsl->count == 1)
			chosenseries = chosengsl->list[0];
		else {
			tablespec gstable;
			PRODUCE_SINGLE_COLUMN_TABLE(gstable, "Profile",
				chosengsl->list, chosengsl->count, series->list[,].name);
			int m = 0;
			for (;;) {
				f = displaylist(&gstable, &m, LIST_SERIES);
				if (f == CmdProceed) {
					chosenseries = chosengsl->list[m];
					break;
				} else if (f == CmdQuitLevel)
					break;
			}
			free_table(&gstable);
		}
	}
	free_table(&mftable);
		*pn = chosenseries;
	return CmdProceed;
}

/* Display the full selection of available series to the user as a
 * scrolling list, and permit one to be selected. When one is chosen,
 * pick one of levels to be the current level. All fields of the
 * gamespec structure are initialized. If autoplay is TRUE, then the
 * function will skip the display if there is only one series
 * available or if defaultseries is not NULL and matches the name of
 * one of the series in the array. Otherwise a scrolling list will be
 * initialized with that series selected. If defaultlevel is not zero,
 * and a level in the selected series that the user is permitted to
 * access matches it, then that level will be the initial current
 * level. The return value is zero if nothing was selected, negative
 * if an error occurred, or positive otherwise.
 */
static int selectseriesandlevel(gamespec *gs, seriesdata *series, int autoplay,
	char const *defaultseries)
{
	int n;

	if (series->count < 1) {
		errmsg(NULL, "no level sets found");
		return -1;
	}

	if (series->count == 1 && autoplay) {
		getseriesfromlist(&gs->series, series->list, 0);
	} else {
		n = 0;
		int founddefault = FALSE;
		if (defaultseries) {
			n = series->count;
			while (n)
				if (!strcmp(series->list[--n].name, defaultseries)) {
					founddefault = TRUE;
					break;
				}
		}

		if(founddefault && autoplay) {
			getseriesfromlist(&gs->series, series->list, n);
		} else {
			int preLevelSet = n;

			for (;;) {
				int f = chooseseries(series, &n, founddefault);
				if (f == CmdProceed) {
					getseriesfromlist(&gs->series, series->list, n);
					break;
				} else if (f == CmdQuitLevel) {
					if(founddefault) {
						getseriesfromlist(&gs->series, series->list, preLevelSet);
						break;
					} else {
						bell();
					}
				}
			}
		}
	}
	freeserieslist(series->list, series->count, series->mflist, series->mfcount);

	setstringsetting("selectedseries", gs->series.name);

	if (!readseriesfile(&gs->series)) {
		errmsg(gs->series.name, "cannot read data file");
		freeseriesdata(&gs->series);
		return -1;
	}
	if (gs->series.count < 1) {
		errmsg(gs->series.name, "no levels found in data file");
		freeseriesdata(&gs->series);
		return -1;
	}

	gs->enddisplay = FALSE;
	gs->playmode = Play_None;
	gs->usepasswds = usepasswds && !(gs->series.gsflags & GSF_IGNOREPASSWDS);
	gs->currentgame = -1;
	gs->melindacount = 0;

	findlevelfromhistory(gs, gs->series.name);

	if (gs->currentgame < 0) {
		gs->currentgame = 0;
		for (n = 0 ; n < gs->series.count ; ++n) {
			if (!issolved(gs, n)) {
				gs->currentgame = n;
				break;
			}
		}
	}

	return +1;
}

/* Get the list of available series and permit the user to select one
 * to play. If lastseries is not NULL, use that series as the default.
 * The return value is zero if nothing was selected, negative if an
 * error occurred, or positive otherwise.
 */
static int choosegame(gamespec *gs, char const *lastseries)
{
	seriesdata	s;

	if (!createserieslist(&s.list, &s.count, &s.mflist, &s.mfcount))
		return -1;
	return selectseriesandlevel(gs, &s, FALSE, lastseries);
}

/* Determine what to play. A list of available series is drawn up; if
 * only one is found, it is selected automatically. Otherwise, if the
 * listseries option is TRUE, the available series are displayed on
 * stdout and the program exits. Otherwise, if listscores or listtimes
 * is TRUE, the scores or times for a single series is display on
 * stdout and the program exits. (These options need to be checked for
 * before initializing the graphics subsystem.) Otherwise, the
 * selectseriesandlevel() function handles the rest of the work. Note
 * that this function is only called during the initial startup; if
 * the user returns to the series list later on, the choosegame()
 * function is called instead.
 */
static int choosegameatstartup(gamespec *gs, char const *lastseries)
{
	seriesdata	series;

	if (!createserieslist(&series.list, &series.count,
			&series.mflist, &series.mfcount))
		return -1;

	if (series.count <= 0) {
		errmsg(NULL, "no level sets found");
		return -1;
	}

	/* extensions cannot be read until the system is initialized */
	if (series.count == 1)
		readextensions(series.list);

	return selectseriesandlevel(gs, &series, TRUE, lastseries);
}

/*
 * The old main function.
 */

int tworld()
{
	gamespec	spec;
	char	lastseries[sizeof spec.series.name];
	int		f;

	atexit(shutdowngamestate);

	// determine the current selected series
	char const *selectedseries = getstringsetting("selectedseries");
	if (selectedseries && strlen(selectedseries) < sizeof lastseries)
		strcpy(lastseries, selectedseries);
	else
		lastseries[0] = '\0';

	// Pick the level to play. Defaults to last played if available.
	f = choosegameatstartup(&spec, lastseries);
	if (f < 0)
		return EXIT_FAILURE;
	else if (f == 0)
		return EXIT_SUCCESS;

	// plays the selected level
	while (f > 0) {
		pushsubtitle(NULL);
		while (runcurrentlevel(&spec)) { }
		savehistory();
		popsubtitle();
		cleardisplay();
		strcpy(lastseries, spec.series.name);
		freeseriesdata(&spec.series);
		f = choosegame(&spec, lastseries);
	};

	return (f == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
