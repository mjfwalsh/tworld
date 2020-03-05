/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include "TWApp.h"
#include "TWMainWnd.h"
#include "tworld.h"
#include "oshwbind.h"
#include "timer.h"
#include "sdlsfx.h"
#include "settings.h"
#include "defs.h"
#include "oshw.h"
#include "res.h"
#include "err.h"

#include <QClipboard>
#include <QDir>
#include <QStandardPaths>

#include <string.h>
#include <stdlib.h>

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
void eventupdate(int wait)
{
	QApplication::processEvents(wait ? QEventLoop::WaitForMoreEvents : QEventLoop::AllEvents);
}


/* Main initialisation function
 */
bool TileWorldApp::Initialize()
{
	// set the application name - needed by InitDirs
	setApplicationName("Tile World");

	// setup the directories - needed by settings and history
	InitDirs();

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
	g_pMainWnd->SetKeyboardRepeat(TRUE);

	// initialise timer
	if (!timerinitialize()) {
		errmsg(NULL, "failed to initialise timer");
		return false;
	}

	// initialise tiles
	if (!tileinitialize()) {
		errmsg(NULL, "failed to initialise tiles");
		return false;
	}

	// initialise sounds
	if (!sfxinitialize()) {
		errmsg(NULL, "failed to load sounds");
	}

	// initial setup of resource system
	if (!initresources()) {
		errmsg(NULL, "failed to initialise resources");
		return false;
	}

	return true;
}


/*
 * Resource-loading functions.
 */

void copytoclipboard(char const *text)
{
	QClipboard* pClipboard = QApplication::clipboard();
	if (pClipboard == 0) return;
	pClipboard->setText(text);
}

const char *getdir(int t)
{
	return g_pApp->GetDir(t);
}

const char *TileWorldApp::GetDir(int t)
{
	switch(t) {
	case RESDIR:
		return appResDir;
	case SERIESDIR:
		return userSetsDir;
	case USER_SERIESDATDIR:
		return userDataDir;
	case GLOBAL_SERIESDATDIR:
		return appDataDir;
	case SOLUTIONDIR:
		return userSolDir;
	case SETTINGSDIR:
		return userDir;
	case NULLDIR:
	default:
		return NULL;
	}
}


void TileWorldApp::InitDirs()
{
	auto checkDir = [](QString d)
	{
		QDir dir(d);
		if (!dir.exists() && !dir.mkpath(".")) {
			die("Unable to create folder %s", d.toUtf8().constData());
		}
	};

	// Get the app resources
	QString appRootDir = QApplication::applicationDirPath();
	#if defined Q_OS_OSX
	QDir appShareDir(appRootDir + "/../Resources");
	#elif defined Q_OS_UNIX
	QDir appShareDir(appRootDir + "/../share/tworld");
	#endif

	#if defined Q_OS_UNIX
	if (appShareDir.exists()) appRootDir = appShareDir.path();
	#endif

	// change pwd to appRootDir
	QDir::setCurrent(appRootDir);

	// these folders should already exist
	QString appResDirQ =  QString(appRootDir + "/res");
	QString appDataDirQ =  QString(appRootDir + "/data");

	// set user directory
	QString userDirQ = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	checkDir(userDirQ);

	// ~/Library/Application Support/Tile World/sets
	QString userSetsDirQ = QString(userDirQ + "/sets");
	checkDir(userSetsDirQ);

	// ~/Library/Application Support/Tile World/data
	QString userDataDirQ = QString(userDirQ + "/data");
	checkDir(userDataDirQ);

	// ~/Library/Application Support/Tile World/solutions
	QString userSolDirQ = QString(userDirQ + "/solutions");
	checkDir(userSolDirQ);

	// translate QString to const char*
	x_cmalloc(appResDir, appResDirQ.length() + 1);
	x_cmalloc(userSetsDir, userSetsDirQ.length() + 1);
	x_cmalloc(userDataDir, userDataDirQ.length() + 1);
	x_cmalloc(appDataDir, appDataDirQ.length() + 1);
	x_cmalloc(userSolDir, userSolDirQ.length() + 1);
	x_cmalloc(userDir, userDirQ.length() + 1);

	strcpy(appResDir, appResDirQ.toUtf8().constData());
	strcpy(userSetsDir, userSetsDirQ.toUtf8().constData());
	strcpy(userDataDir, userDataDirQ.toUtf8().constData());
	strcpy(appDataDir, appDataDirQ.toUtf8().constData());
	strcpy(userSolDir, userSolDirQ.toUtf8().constData());
	strcpy(userDir, userDirQ.toUtf8().constData());
}

void TileWorldApp::ExitTWorld()
{
	// These function need to called from here as they
	// need to access TileWorldApp::GetDir
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
