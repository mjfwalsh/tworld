/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QtGui/QClipboard>
#include <SDL.h>
#include <cstdlib>

#include "TWApp.h"
#include "tworld.h"
#include "oshwbind.h"
#include "timer.h"
#include "sdlsfx.h"
#include "settings.h"
#include "TWMainWnd.h"
#include "fileio.h"
#include "messages.h"
#include "unslist.h"
#include "err.h"

TileWorldApp* g_app = nullptr;
TileWorldMainWnd* g_mainWindow = nullptr;

TileWorldApp::TileWorldApp(int& argc, char** argv)
    :
    QApplication(argc, argv)
{
    g_app = this;

    // set the application name - needed by initdirs
    setApplicationName("Tile World");

    // setup the directories - needed by settings and history
    initdirs();

    // load history
    loadhistory();

    // load history
    loadsettings();

    // set the window icon
    #if not defined __APPLE__
    setWindowIcon(QIcon("tworld.png"));
    #endif

    // start the main window
    g_mainWindow = new TileWorldMainWnd;
    g_mainWindow->setWindowTitle(applicationName());
    g_mainWindow->SetKeyboardRepeat(true);

    // initialise timer
    timerinitialize();

    // initialise tiles
    tileinitialize();

    // initialise sounds
    if (!sfxinitialize()) {
        warn("failed to load sounds");
    }

    // initial setup of resource system
    loadmessagesfromfile("messages.txt");
    loadunslistfromfile("unslist.txt");
}


TileWorldApp::~TileWorldApp()
{
    delete g_mainWindow;
    g_mainWindow = nullptr;
    g_app = nullptr;
}


/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
void eventupdate(bool wait)
{
    QApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);
}


/*
 * Copy text to clipboard
 */

void TileWorldApp::CopyToClipboard(const QString &text)
{
    QClipboard* pClipboard = QApplication::clipboard();
    if (pClipboard == 0) return;
    pClipboard->setText(text);
}

/* Ring the bell.
 */
void TileWorldApp::Bell(void)
{
    int v = getintsetting("volume");
    if(v > 0) QApplication::beep();
}

/* The real main().
 */
int main(int argc, char *argv[])
{
    TileWorldApp app(argc, argv);
    return tworld();
}
