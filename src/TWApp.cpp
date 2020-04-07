/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include <QClipboard>

#include <cstdlib>

#include "TWApp.h"
#include "TWMainWnd.h"
#include "tworld.h"
#include "oshwbind.h"
#include "timer.h"
#include "sdlsfx.h"
#include "settings.h"
#include "defs.h"
#include "TWMainWnd.h"
#include "res.h"
#include "fileio.h"
#include "messages.h"
#include "unslist.h"
#include "err.h"

TileWorldApp* g_pApp = 0;
TileWorldMainWnd* g_pMainWnd = 0;

TileWorldApp::TileWorldApp(int& argc, char** argv)
	:
	QApplication(argc, argv)
{
	g_pApp = this;
}


TileWorldApp::~TileWorldApp()
{
	delete g_pMainWnd;
	g_pMainWnd = 0;

	g_pApp = 0;
}


/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
void eventupdate(bool wait)
{
	QApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);
}


/* Main initialisation function
 */
bool TileWorldApp::Initialize()
{
	// set the application name - needed by initdirs
	setApplicationName("Tile World");

	// setup the directories - needed by settings and history
	initdirs();

	// load history
	loadhistory();

	// load history
	loadsettings();

	// set the window icon
	#if not defined Q_OS_OSX
	setWindowIcon(QIcon("tworld.png"));
	#endif

	// start the main window
	g_pMainWnd = new TileWorldMainWnd;
	g_pMainWnd->setWindowTitle(applicationName());
	g_pMainWnd->SetKeyboardRepeat(true);

	// initialise timer
	if (!timerinitialize()) {
		warn("failed to initialise timer");
		return false;
	}

	// initialise tiles
	tileinitialize();

	// initialise sounds
	if (!sfxinitialize()) {
		warn("failed to load sounds");
	}

	// initial setup of resource system
	loadmessagesfromfile("messages.txt");
	loadunslistfromfile("unslist.txt");

	return true;
}


/*
 * Copy text to clipboard
 */

void TileWorldApp::CopyToClipboard(QString text)
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

/* Exit gracefully. Called when the user closes the window.
 */
void TileWorldApp::ExitTWorld()
{
	// These functions should only be run when exiting gracefully
	savesettings();
	savehistory();

	// Attempt to gracefully destroy application objects

	// throw 1;
	// Can't throw C++ exceptions through C code

	// longjmp(m_jmpBuf, 1);
	// Works, but needs to be cleaner
	::exit(0);
	// Live with this for now...
}


/* The real main().
 */
int main(int argc, char *argv[])
{
	TileWorldApp app(argc, argv);
	if(!app.Initialize()) return 1;
	return tworld();
}
