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


void initdirs()
{
	resdir = (char *) malloc(g_pApp->appResDir.length() + 1);
	seriesdir = (char *) malloc(g_pApp->userSetsDir.length() + 1);
	user_seriesdatdir = (char *) malloc(g_pApp->userDataDir.length() + 1);
	global_seriesdatdir = (char *) malloc(g_pApp->appDataDir.length() + 1);
	solutiondir = (char *) malloc(g_pApp->userSolDir.length() + 1);
	settingsdir = (char *) malloc(g_pApp->userDir.length() + 1);

	strcpy(resdir, g_pApp->appResDir.toUtf8().constData());
	strcpy(seriesdir, g_pApp->userSetsDir.toUtf8().constData());
	strcpy(user_seriesdatdir, g_pApp->userDataDir.toUtf8().constData());
	strcpy(global_seriesdatdir, g_pApp->appDataDir.toUtf8().constData());
	strcpy(solutiondir, g_pApp->userSolDir.toUtf8().constData());
	strcpy(settingsdir, g_pApp->userDir.toUtf8().constData());
}

void TileWorldApp::InitDirs()
{
	auto checkDir = [](QString d)
    {
		QDir dir(d);
		if (!dir.exists()) dir.mkpath(".");
    };

	// Get the app resources
	QString appRootDir = QApplication::applicationDirPath();
	#if defined Q_OS_OSX || Q_OS_LINUX
	{
		#if defined Q_OS_OSX
		QString appResDir = appRootDir + "/../Resources";
		#elif defined Q_OS_LINUX
		QString appResDir = appRootDir + "/../share/tworld";
		#endif

		QDir dir(appResDir);
		if (dir.exists()) appRootDir = appResDir;
	}
	#endif

	// change pwd to appRootDir
	QDir::setCurrent(appRootDir);

	// these should already exist
	appResDir =  QString(appRootDir + "/res");
	appDataDir =  QString(appRootDir + "/data");

	// get user directory
	userDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	checkDir(userDir);

	// ~/Library/Application Support/Tile World/sets
	userSetsDir = QString(userDir + "/sets");
	checkDir(userSetsDir);

	// ~/Library/Application Support/Tile World/data
	userDataDir = QString(userDir + "/data");
	checkDir(userDataDir);

	// ~/Library/Application Support/Tile World/solutions
	userSolDir = QString(userDir + "/solutions");
	checkDir(userSolDir);
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
	return app.RunTWorld();
}
