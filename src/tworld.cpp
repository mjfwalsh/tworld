/* tworld.cpp: The top-level module.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag,
 * Eric Schmidt and Michael Walsh
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#include    <QtCore/QString>
#include    <vector>

#include    "TWApp.h"
#include    "TWTableSpec.h"
#include    "tworld.h"
#include    "defs.h"
#include    "series.h"
#include    "play.h"
#include    "score.h"
#include    "settings.h"
#include    "solution.h"
#include    "TWMainWnd.h"
#include    "fileio.h"
#include    "timer.h"
#include    "sdlsfx.h"
#include    "utils.h"
#include    "err.h"

/* basic game struct
 */
gamespec    gs;

/* History of levelsets in order of last used date/time.
 */
static std::vector<history> historylist;

/* FALSE suppresses all password checking.
 */
static bool usepasswds = true;

/* Frame-skipping disable flag.
 */
static bool noframeskip = false;


/*
 * Basic game activities.
 */

/* Return TRUE if the given level is a final level.
 */
static bool islastinseries(int index)
{
    return index == gs.series.count - 1;
}

/* Return TRUE if the current level has a solution.
 */
static int issolved(int index)
{
    return hassolution(&gs.series.games[index]);
}

/* Mark the current level's solution as replaceable.
 */
static void replaceablesolution(int change)
{
    if (change < 0)         // toggle
        gs.series.games[gs.currentgame].sgflags ^= SGF_REPLACEABLE;
    else if (change > 0)    // set
        gs.series.games[gs.currentgame].sgflags |= SGF_REPLACEABLE;
    else                    // unset
        gs.series.games[gs.currentgame].sgflags &= ~SGF_REPLACEABLE;
}

/* Mark the current level's password as known to the user.
 */
static void passwordseen(int number)
{
    if (!(gs.series.games[number].sgflags & SGF_HASPASSWD)) {
        gs.series.games[number].sgflags |= SGF_HASPASSWD;
        savesolutions(gs.series);
    }
}

/* Change the current level, ensuring that the user is not granted
 * access to a forbidden level. FALSE is returned if the specified
 * level is not available to the user.
 */
static bool setcurrentgame(int n)
{
    if (n == gs.currentgame)
        return true;
    if (n < 0 || n >= gs.series.count)
        return false;

    if (usepasswds)
        if (n > 0 && !(gs.series.games[n].sgflags & SGF_HASPASSWD)
            && !issolved(n -1))
            return false;

    gs.currentgame = n;
    gs.melindacount = 0;
    return true;
}

/* Change the current level by a delta value. If the user cannot go to
 * that level, the "nearest" level in that direction is chosen
 * instead. FALSE is returned if the current level remained unchanged.
 */
static bool changecurrentgame(int offset)
{
    int m = gs.currentgame;
    int n = m + offset;
    if (n < 0) {
        n = 0;
        offset = n - m;
    } else if (n >= gs.series.count) {
        n = gs.series.count - 1;
        offset = n - m;
    }

    if (offset == 0)
        return false;

    if (usepasswds && n > 0) {
        int sign = offset < 0 ? -1 : +1;
        for ( ; n >= 0 && n < gs.series.count ; n += sign) {
            if (!n || (gs.series.games[n].sgflags & SGF_HASPASSWD)
                || issolved(n - 1)) {
                m = n;
                break;
            }
        }
        n = m;
        if (n == gs.currentgame && offset != sign) {
            n = gs.currentgame + offset - sign;
            for ( ; n != gs.currentgame ; n -= sign) {
                if (n < 0 || n >= gs.series.count)
                    continue;
                if (!n || (gs.series.games[n].sgflags & SGF_HASPASSWD)
                    || issolved(n - 1))
                    break;
            }
        }
    }

    if (n == gs.currentgame)
        return false;

    gs.currentgame = n;
    gs.melindacount = 0;
    return true;
}

/* Return TRUE if Melinda is watching Chip's progress on this level --
 * i.e., if it is possible to earn a pass to the next level.
 */
static bool melindawatching()
{
    if (!usepasswds)
        return false;
    if (islastinseries(gs.currentgame))
        return false;
    if (gs.series.games[gs.currentgame + 1].sgflags & SGF_HASPASSWD)
        return false;
    if (issolved(gs.currentgame))
        return false;
    return true;
}

/* Display the scrolling list of the user's current scores, and allow
 * the user to select a current level.
 */
static int showscores()
{
    TWTableSpec table(g_mainWindow);
    std::vector<int> levellist;
    int     n;

    createscorelist(&gs.series, usepasswds, levellist, &table);

    for (n = 0; n < (int)levellist.size(); ++n)
        if (levellist[n] == gs.currentgame)
            break;

    g_mainWindow->PushSubtitle(gs.series.name.c_str());
    g_mainWindow->DisplayList(table, n, Ruleset_None);
    for (;;) {
        int f = g_mainWindow->Input(true);
        if (f == CmdProceed) {
            n = g_mainWindow->GetSelectedRow();
            n = levellist[n];
            if (n >= 0 && setcurrentgame(n))
                break;
        } else if (f == CmdChooseLevelset) {
            n = -1;
            break;
        } else if (f == CmdQuit) {
            return CmdQuit;
        }
    }
    g_mainWindow->PopSubtitle();

    return CmdNone;
}

/* Obtain a password from the user and move to the requested level.
 */
static bool selectlevelbypassword()
{
    char passwd[5];
    int n;

    g_mainWindow->DisplayPasswordPrompt(passwd);

    if (strlen(passwd) != 4) goto fail;

    n = findlevelinseries(gs.series, 0, passwd);
    if (n < 0) goto fail;

    passwordseen(n);
    if (setcurrentgame(n)) return true;

fail:
    TileWorldApp::Bell();
    return false;
}

/*
 * The levelset history functions.
 */

/* Load the levelset history.
 */
bool loadhistory(void)
{
    char    buf[256];
    char       *hdate, *htime, *hpasswd, *hnumber, *hname;
    int     hyear, hmon, hmday, hhour, hmin, hsec;

    historylist.clear();

    fileinfo    file(SETTINGSDIR, "history");

    if (!file.open("r", nullptr))
        return false;

    for (;;) {
        if (!file.getline(buf, sizeof buf - 1))
            break;

        if (buf[0] == '#')
            continue;

        hdate   = strtok(buf , " \t");
        htime   = strtok(nullptr, " \t");
        hpasswd = strtok(nullptr, " \t");
        hnumber = strtok(nullptr, " \t");
        hname   = strtok(nullptr, "\r\n");

        if ( ! (hdate && htime && hpasswd && hnumber && hname  &&
                sscanf(hdate, "%4d-%2d-%2d", &hyear, &hmon, &hmday) == 3  &&
                sscanf(htime, "%2d:%2d:%2d", &hhour, &hmin, &hsec) == 3  &&
                *hpasswd  && *hnumber && *hname) )
            continue;

        history &h = historylist.emplace_back();
        h.name = hname;
        stringcopy(h.passwd, hpasswd, (int)(sizeof h.passwd));
        h.levelnumber = (int)strtol(hnumber, nullptr, 0);
        h.dt.tm_year  = hyear - 1900;
        h.dt.tm_mon   = hmon - 1;
        h.dt.tm_mday  = hmday;
        h.dt.tm_hour  = hhour;
        h.dt.tm_min   = hmin;
        h.dt.tm_sec   = hsec;
        h.dt.tm_isdst = -1;
    }

    file.close();

    return true;
}

/* Update the levelset history for the set and level being played.
 */
static void updatehistory()
{
    time_t t = time(nullptr);
    char const *name = gs.series.dacfilename.c_str();
    
    for (auto h = historylist.begin(); h < historylist.end(); ++h) {
        if (strcasecmp(h->name.c_str(), name) == 0) {
            historylist.erase(h);
            break;
        }
    }

    auto h = historylist.emplace(historylist.begin());
    h->name = name;
    stringcopy(h->passwd, gs.series.games[gs.currentgame].passwd, (int)(sizeof h->passwd));
    h->levelnumber = gs.series.games[gs.currentgame].number;
    h->dt = *localtime(&t);
}

/* Save the levelset history.
 */
static void savehistory(void)
{
    fileinfo    file(SETTINGSDIR, "history");

    if (!file.open("w", nullptr))
        return;

    for (const history &h : historylist) {
        file.writef("%04d-%02d-%02d %02d:%02d:%02d\t%s\t%d\t%s\n",
            1900 + h.dt.tm_year, 1 + h.dt.tm_mon, h.dt.tm_mday,
            h.dt.tm_hour, h.dt.tm_min, h.dt.tm_sec,
            h.passwd, h.levelnumber, h.name.c_str());
    }

    file.close();
}

/*
 * The game-playing functions.
 */

/* Show narration text
 */
static int narrate(int cmd)
{
    int levelnum = gs.series.games[gs.currentgame].number;

    CCX::Text *doc;
    switch (cmd) {
        case CmdNarratePrologue:
        case CmdNarratePrologueForced:
            doc = &gs.series.ccxLevelset.vecLevels[levelnum].txtPrologue;
            break;

        case CmdNarrateEpilogue:
        case CmdNarrateEpilogueForced:
            doc = &gs.series.ccxLevelset.vecLevels[levelnum].txtEpilogue;
            break;

        default:
            return cmd;
    }

    switch (cmd) {
        case CmdNarratePrologue:
        case CmdNarrateEpilogue:
            if (doc->bSeen || doc->vecPages.empty() || !g_mainWindow->GetAutoShowNarration())
                return CmdNone;
    }

    doc->bSeen = true;

    TileWorldMainWnd::Narration narrate(g_mainWindow, *doc, gs.series.ccxLevelset.sStyleSheet);
    for (;;) {
        cmd = g_mainWindow->Input(true);
        switch(cmd) {
        case CmdWest:
            if(narrate.ChangePage(-1))
                continue;
            else
                return CmdNone;

        case CmdEast:
            if(narrate.ChangePage(1))
                continue;
            else
                return CmdNone;

        case CmdProceed:
            return CmdNone;

        default:
            return cmd;
        }
    }
}

/* Get a key command from the user at the start of the current level.
 */
static int startinput()
{
    static int  lastlevel = -1;

    if (gs.currentgame != lastlevel) {
        lastlevel = gs.currentgame;
        setstepping(0);
    }

    initgamescreen(gs.series);
    int cmd = narrate(CmdNarratePrologue);
    drawscreen(true);
    gs.playmode = Play_None;
    for (;;) {
        if (cmd == CmdNone)
            cmd = g_mainWindow->Input(true);
        switch (cmd) {
        case CmdMoveFirst...CmdMoveLast:
            gs.playmode = Play_Normal;
            return cmd;
        case CmdPauseGame:
        case CmdProceed:    gs.playmode = Play_Normal; return CmdProceed;
        case CmdPrevLevel:
        case CmdNextLevel:
            if (changecurrentgame(cmd == CmdPrevLevel ? -1 : +1))
                return CmdNone;
            TileWorldApp::Bell();
            cmd = CmdNone;
            break;
        case CmdChooseLevelset:
        case CmdQuit:
            return cmd;
        case CmdPlayback:
            if (prepareplayback()) {
                gs.playmode = Play_Back;
                return cmd;
            }
            TileWorldApp::Bell();
            cmd = CmdNone;
            break;
        case CmdSeek:
            if (g_mainWindow->GetReplaySecondsToSkip() > 0) {
                gs.playmode = Play_Back;
                return CmdProceed;
            }
            cmd = CmdNone;
            break;
        case CmdCheckSolution:
            if (prepareplayback()) {
                gs.playmode = Play_Verify;
                return CmdProceed;
            }
            TileWorldApp::Bell();
            cmd = CmdNone;
            break;
        case CmdDelSolution:
            if (issolved(gs.currentgame)) {
                replaceablesolution(-1);
                savesolutions(gs.series);
            } else {
                TileWorldApp::Bell();
            }
            cmd = CmdNone;
            break;
        case CmdSeeScores:
            return showscores();
        case CmdTimesClipboard:
            copyleveltimestoclipboard(&gs.series);
            cmd = CmdNone;
            break;
        case CmdGotoLevel:
            if (selectlevelbypassword())
                return CmdNone;
            cmd = CmdNone;
            break;
        case CmdNarratePrologue:
        case CmdNarratePrologueForced:
        case CmdNarrateEpilogue:
        case CmdNarrateEpilogueForced:
            cmd = narrate(cmd);
            break;
        default:
            cmd = CmdNone;
            break;
        }
    }
}

/* Get a key command from the user at the completion of the current
 * level.
 */
static int endinput()
{
    if (gs.status < 0) {
        if (melindawatching() && secondsplayed() >= 10) {
            ++gs.melindacount;
            if (gs.melindacount >= 10) {
                if (g_mainWindow->DisplayYesNoPrompt("Skip level?")) {
                    g_mainWindow->ReleaseAllKeys();
                    passwordseen(gs.currentgame + 1);
                    changecurrentgame(+1);
                }
                gs.melindacount = 0;
                return CmdSameLevel;
            }
        }

        g_mainWindow->DisplayEndMessageFailure();
        return CmdSameLevel;
    } else {
        int     bscore = 0, tscore = 0;
        long    gscore = 0;

        getscoresforlevel(&gs.series, gs.currentgame,
            &bscore, &tscore, &gscore);

        int cmd = g_mainWindow->DisplayEndMessageSuccess(bscore, tscore, gscore);
        if(cmd == CmdSameLevel) return CmdSameLevel;

        // display narration (if any)
        cmd = narrate(CmdNarrateEpilogue);

        // move to next level
        if (cmd == CmdNone) {
            if (islastinseries(gs.currentgame))
                gs.enddisplay = true;
            else
                changecurrentgame(+1);

            return CmdSameLevel;
        }

        return cmd;
    }
}

/* Get a key command from the user at the completion of the current
 * series.
 */
static int finalinput()
{
    for (;;) {
        int cmd = g_mainWindow->Input(true);
        switch (cmd) {
        case CmdSameLevel:
            return CmdSameLevel;
        case CmdPrevLevel:
        case CmdNextLevel:
            setcurrentgame(0);
            return CmdSameLevel;
        case CmdQuit:
            return CmdQuit;
        default:
            return CmdChooseLevelset;
        }
    }
}

static void setpaused(bool &gamepaused, bool shutter)
{
    gamepaused = !gamepaused;
    if(!gamepaused)
        setgameplaymode(NormalPlay);
    else if(shutter)
        setgameplaymode(SuspendPlayShuttered);
    else
        setgameplaymode(SuspendPlay);

    g_mainWindow->SetPlayPauseButton(gamepaused);
}

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
static int playgame(int firstcmd)
{
    bool    render, lastrendered;
    int cmd, n;

    cmd = firstcmd;
    if (cmd == CmdProceed)
        cmd = CmdNone;

    gs.status = 0;
    setgameplaymode(NormalPlay);
    render = lastrendered = true;

    bool gamepaused = false;
    g_mainWindow->SetPlayPauseButton(gamepaused);
    startgame();
    for (;;) {
        if (gamepaused)
            cmd = g_mainWindow->Input(true);
        else {
            n = doturn(cmd);
            drawscreen(render);
            lastrendered = render;
            if (n)
                break;
            render = waitfortick() || noframeskip;
            cmd = g_mainWindow->Input(false);
        }

        switch (cmd) {
        case CmdMoveFirst...CmdMoveLast:
            continue;

        case CmdChooseLevelset:
            quitgamestate();
            n = -2;
            goto exitloop;

        case CmdPreserve:                   break;
        case CmdPrevLevel:      n = -1;     goto endgame;
        case CmdNextLevel:      n = +1;     goto endgame;
        case CmdSameLevel:      n = 0;      goto endgame;
        case CmdQuit:                   return CmdQuit;
        case CmdLostFocus:
            if(gamepaused) break;
        case CmdPauseGame:
            setpaused(gamepaused, true);
            if (!gamepaused)
                cmd = CmdNone;
            break;
#ifndef NDEBUG
        case CmdDebugCmd1:              break;
        case CmdDebugCmd2:              break;
        case CmdCheatNorth:     case CmdCheatWest:  break;
        case CmdCheatSouth:     case CmdCheatEast:  break;
        case CmdCheatHome:              break;
        case CmdCheatKeyRed:    case CmdCheatKeyBlue:   break;
        case CmdCheatKeyYellow: case CmdCheatKeyGreen:  break;
        case CmdCheatBootsIce:  case CmdCheatBootsSlide:    break;
        case CmdCheatBootsFire: case CmdCheatBootsWater:    break;
        case CmdCheatICChip:                break;
#endif
        default:
            cmd = CmdNone;
            break;
        }
    }

exitloop:
    if (!lastrendered)
        drawscreen(true);
    setgameplaymode(EndPlay);
    if (n > 0)
        if (replacesolution())
            savesolutions(gs.series);
    gs.status = n;
    return endinput();

endgame:
    if (!lastrendered)
        drawscreen(true);
    quitgamestate();
    setgameplaymode(EndPlay);
    if (n)
        changecurrentgame(n);
    return CmdRestartLevel;
}

/* Skip past secondstoskip seconds from the beginning of the solution.
 */
static int hideandseek(int secondstoskip, bool initgame = false)
{
    int n = 0;

    quitgamestate();
    setgameplaymode(EndPlay);
    gs.playmode = Play_None;
    endgamestate();
    initgamestate(gs.series, gs.currentgame);
    prepareplayback();
    gs.playmode = Play_Back;
    gs.status = 0;
    setgameplaymode(NonrenderPlay);

    while (secondsplayed() < secondstoskip) {
        n = doturn(CmdNone);
        if (n)
            break;
        advancetick();
    }

    if(initgame) startgame();

    drawscreen(true);
    setsoundeffects(-1);
    setgameplaymode(NormalPlay);

    return n;
}

/* Play back the user's best solution for the current level in real
 * time. Other than the fact that this function runs from a
 * prerecorded series of moves, it has the same behavior as
 * playgame().
 */
static int playbackgame()
{
    bool    render, lastrendered;
    int n = 0, cmd;
    int secondstoskip;
    bool gamepaused = false;

    secondstoskip = g_mainWindow->GetReplaySecondsToSkip();
    if (secondstoskip > 0) {
        gamepaused = true;
        n = hideandseek(secondstoskip, true);
        setgameplaymode(SuspendPlay);
    } else {
        gamepaused = false;
        startgame();
        drawscreen(true);
        gs.status = 0;
        setgameplaymode(NormalPlay);
    }
    g_mainWindow->SetPlayPauseButton(gamepaused);

    render = lastrendered = true;

    while (!n) {
        if (gamepaused) {
            setgameplaymode(SuspendPlay);
            cmd = g_mainWindow->Input(true);
        } else {
            n = doturn(CmdNone);
            drawscreen(render);
            lastrendered = render;
            if (n)
                break;
            render = waitfortick() || noframeskip;
            cmd = g_mainWindow->Input(false);
        }
        switch (cmd) {
        case CmdSeek:
        case CmdWest:
        case CmdEast:
            if (cmd == CmdSeek) {
                secondstoskip = g_mainWindow->GetReplaySecondsToSkip();
            } else {
                secondstoskip = secondsplayed() + ((cmd == CmdEast) ? +3 : -3);
            }
            n = hideandseek(secondstoskip);
            lastrendered = true;
            break;
        case CmdPrevLevel:  changecurrentgame(-1);  goto quitloop;
        case CmdNextLevel:  changecurrentgame(+1);  goto quitloop;
        case CmdSameLevel:
        case CmdPlayback:
        case CmdChooseLevelset:  goto quitloop;
        case CmdQuit:           return CmdQuit;
        case CmdLostFocus:
            if(gamepaused) break;
        case CmdPauseGame:
            setpaused(gamepaused, false);
            break;
        }
    }
    if (!lastrendered)
        drawscreen(true);
    setgameplaymode(EndPlay);
    gs.playmode = Play_None;
    if (n < 0)
        replaceablesolution(+1);
    if (n > 0) {
        if (checksolution())
            savesolutions(gs.series);
    }
    gs.status = n;
    return endinput();

quitloop:
    if (!lastrendered)
        drawscreen(true);
    quitgamestate();
    setgameplaymode(EndPlay);
    gs.playmode = Play_None;
    return CmdRestartLevel;
}


/* Quickly play back the user's best solution for the current level
 * without rendering and without using the timer the keyboard. The
 * playback stops when the solution is finished or gameplay has
 * ended.
 */
static int verifyplayback()
{
    int n;

    gs.status = 0;
    setgameplaymode(NonrenderPlay);
    for (;;) {
        n = doturn(CmdNone);
        if (n)
            break;
        advancetick();
        switch (g_mainWindow->Input(false)) {
        case CmdPrevLevel:  changecurrentgame(-1);  goto quitloop;
        case CmdNextLevel:  changecurrentgame(+1);  goto quitloop;
        case CmdSameLevel:                  goto quitloop;
        case CmdPlayback:                   goto quitloop;
        case CmdChooseLevelset:                  goto quitloop;
        case CmdQuit:                       return CmdQuit;
        }
    }
    gs.playmode = Play_None;
    quitgamestate();
    drawscreen(true);
    setgameplaymode(EndPlay);
    if (n < 0) {
        replaceablesolution(+1);
    }
    if (n > 0) {
        if (checksolution())
            savesolutions(gs.series);
    }
    gs.status = n;
    return endinput();

quitloop:
    gs.playmode = Play_None;
    setgameplaymode(EndPlay);
    return CmdRestartLevel;
}

/* Manage a single session of playing the current level, from start to
 * finish. A return value of FALSE indicates that the user is done
 * playing levels from the current series; otherwise, the gamespec
 * structure is updated as necessary upon return.
 */
static int runcurrentlevel()
{
    int ret = CmdSameLevel;
    int cmd;
    int valid;

    g_mainWindow->SetPlayPauseButton(true);

    updatehistory();

    if (gs.enddisplay) {
        gs.enddisplay = false;
        g_mainWindow->ChangeSubtitle("");
        setenddisplay();
        drawscreen(true);
        endgamestate();
        return finalinput();
    }

    valid = initgamestate(gs.series, gs.currentgame);
    g_mainWindow->ChangeSubtitle(gs.series.games[gs.currentgame].name.c_str());
    passwordseen(gs.currentgame);
    if (!islastinseries(gs.currentgame))
        if (!valid || gs.series.games[gs.currentgame].unsolvable)
            passwordseen(gs.currentgame + 1);

    cmd = startinput();

    if (cmd == CmdChooseLevelset) {
        ret = CmdChooseLevelset;
    } else if (cmd == CmdQuit) {
        ret = CmdQuit;
    } else if (cmd != CmdNone) {
        if (valid) {
            switch (gs.playmode) {
            case Play_Normal:   ret = playgame(cmd);      break;
            case Play_Back: ret = playbackgame();   break;
            case Play_Verify:   ret = verifyplayback();     break;
            default:            ret = CmdRestartLevel;          break;
            }
        } else
            TileWorldApp::Bell();
    }

    endgamestate();
    return ret;
}

/*
 * Game selection functions
 */

/* Set the current level to that specified in the history. */
static void findlevelfromhistory()
{
    const char *name = gs.series.dacfilename.c_str();

    for (history &h : historylist) {
        if (strcasecmp(h.name.c_str(), name) == 0) {
            int n = findlevelinseries(gs.series, h.levelnumber, h.passwd);
            if (n < 0)
                n = findlevelinseries(gs.series, 0, h.passwd);
            if (n >= 0) {
                gs.currentgame = n;
                if (usepasswds && !(gs.series.games[n].sgflags & SGF_HASPASSWD))
                    changecurrentgame(-1);
            }
            break;
        }
    }
}

// Find the currentseries in seriesdata
static int findseries(std::vector<gameseries> &serieslist, const std::string &currentseries)
{
    if (currentseries.empty()) return -1;

    for(int levelset = 0; levelset < (int)serieslist.size(); levelset++) {
        if (serieslist[levelset].mapfilename == currentseries) {
            return levelset;
        }
    }

    // none found
    return -1;
}


/* We no longer use actual .dac files but...  */
static std::string generatedacfilename()
{
    std::string dacfile = gs.series.mapfilename;

    if(gs.series.ruleset == Ruleset_Lynx) dacfile += "-lynx.dac";
    else dacfile += "-ms.dac";

    return dacfile;
}

/* Display the full selection of available series to the user as a
 * scrolling list, and permit one to be selected. When one is chosen,
 * pick one of levels to be the current level. All fields of the
 * gamespec structure are initialized. If autoplay is TRUE, then the
 * function will skip the display if there is only one series
 * available or if currentseries is not nullptr and matches the name of
 * one of the series in the array. Otherwise a scrolling list will be
 * initialized with that series selected. If defaultlevel is not zero,
 * and a level in the selected series that the user is permitted to
 * access matches it, then that level will be the initial current
 * level. The return value is false is the user closes the window,
 * otherwise true.
 */
static bool selectseriesandlevel(std::vector<gameseries> &serieslist, int &levelset)
{
    int preLevelSet = levelset;
again:
    TWTableSpec mftable(g_mainWindow);
    mftable.setCols(1);
    mftable.addCell("Levelset");
    for (const gameseries &series : serieslist) {
        mftable.addCell(series.name.c_str());
    }

    g_mainWindow->DisplayList(mftable, levelset, gs.series.ruleset);

    for (;;) {
        int f = g_mainWindow->Input(true);
        if (f == CmdProceed) {
            gs.series.ruleset = g_mainWindow->GetSelectedRuleSet();
            levelset = g_mainWindow->GetSelectedRow();
            return true;
        } else if (f == CmdReloadLevelsets) {
            createserieslist(serieslist);
            goto again;
        } else if (f == CmdQuit) {
            return false;
        } else if (f == CmdChooseLevelset) {
            if (preLevelSet != -1) {
                levelset = preLevelSet;
                return true;
            } else {
                TileWorldApp::Bell();
            }
        }
    }
}


static void selectlevelset(std::vector<gameseries> &serieslist, int levelset)
{
    // move the selected series to the gamespec
    gs.series.count = serieslist[levelset].count;
    gs.series.games = std::move(serieslist[levelset].games);
    gs.series.mapfilename = std::move(serieslist[levelset].mapfilename);
    gs.series.mapfiledir = serieslist[levelset].mapfiledir;
    gs.series.solheadersize = serieslist[levelset].solheadersize;
    gs.series.name = std::move(serieslist[levelset].name);
    memcpy(gs.series.solheader, serieslist[levelset].solheader, serieslist[levelset].solheadersize);

    // copy some dac info over to gameseries
    gs.series.dacfilename = generatedacfilename();
    gs.series.gsflags = 0;

    // change the selected series setting
    setstringsetting("selectedseries", gs.series.mapfilename.c_str());
    setintsetting("selectedruleset", gs.series.ruleset);

    if (!readseriesfile(gs.series))
        die("%s: cannot read data file", gs.series.dacfilename.c_str());

    if (gs.series.count < 1)
        die("%s: no levels found in data file", gs.series.dacfilename.c_str());

    gs.enddisplay = false;
    gs.playmode = Play_None;
    gs.currentgame = -1;
    gs.melindacount = 0;

    findlevelfromhistory();

    if (gs.currentgame < 0) {
        gs.currentgame = 0;
        for (int i = 0 ; i < gs.series.count ; ++i) {
            if (!issolved(i)) {
                gs.currentgame = i;
                break;
            }
        }
    }
}

static bool chooselevelset(bool alwaysshowlist) {
    std::vector<gameseries> serieslist;

    // create a list of all the available levelsets
    createserieslist(serieslist);

    // pick a levelset
    int levelset;
    if (serieslist.size() == 1)
        levelset = 0;
    else
        levelset = findseries(serieslist, gs.series.mapfilename);

    // display a scrolling list
    if (alwaysshowlist || levelset == -1)
        if(!selectseriesandlevel(serieslist, levelset))
            return false;

    // Pick the level to play. Defaults to last played if available.
    selectlevelset(serieslist, levelset);

    return true;
}


/*
 * The old main function.
 */

int tworld()
{
    atexit(shutdowngamestate);

    // get previously selected levelset and ruleset
    gs.series.mapfilename = getstringsetting("selectedseries");
    gs.series.ruleset = getintsetting("selectedruleset");
    if (gs.series.ruleset == -1)
        gs.series.ruleset = Ruleset_Lynx;

    if(!chooselevelset(false))
        return 0;

    g_mainWindow->PushSubtitle("");

    // plays the game
    while (true) {
        int cmd = runcurrentlevel();
        switch(cmd) {

        case CmdQuit:
            savesettings();
            savehistory();
            return EXIT_SUCCESS;

        case CmdChooseLevelset:
            savehistory();
            g_mainWindow->PopSubtitle();
            g_mainWindow->ClearDisplay();
            freeseriesdata(gs.series);
            if(!chooselevelset(true))
                return EXIT_SUCCESS;

            g_mainWindow->PushSubtitle("");
            break;

        default:
            break;
        }
    }
}
