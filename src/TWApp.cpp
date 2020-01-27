/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include "TWApp.h"
#include "TWMainWnd.h"
#include "generic.h"
#include "sdlsfx.h"
#include "defs.h"
#include "oshw.h"
#include "res.h"
#include "err.h"
#include "solution.h"
#include "settings.h"

#include <QClipboard>
#include <QDir>
#include <QStandardPaths>

#include <string.h>
#include <stdlib.h>

TileWorldApp* g_pApp = 0;
TileWorldMainWnd* g_pMainWnd = 0;

TileWorldApp::TileWorldApp(int& argc, char** argv)
	:
	QApplication(argc, argv),
	m_bSilence(false),
	m_bShowHistogram(false)
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
int oshwinitialize(int silence, int soundbufsize, int showhistogram)
{
	return g_pApp->Initialize(silence, soundbufsize, showhistogram);
}

bool TileWorldApp::Initialize(bool bSilence, int nSoundBufSize,
                              bool bShowHistogram)
{
	m_bSilence = bSilence;
	m_bShowHistogram = bShowHistogram;

	g_pMainWnd = new TileWorldMainWnd;
	g_pMainWnd->setWindowTitle(applicationName());

	if ( ! (
		generictimerinitialize(bShowHistogram) &&
		generictileinitialize() &&
		sdlsfxinitialize(bSilence, nSoundBufSize)
	   ) )
		return false;

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

int TileWorldApp::RunTWorld()
{
    return tworld();
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
	app.setApplicationName("Tile World");
	app.InitDirs();
	#if not defined Q_OS_OSX
		app.setWindowIcon(QIcon("tworld.png"));
	#endif
	return app.RunTWorld();
}
