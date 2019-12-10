/* Copyright (C) 2001-2010 by Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef TWAPP_H
#define TWAPP_H

class TileWorldMainWnd;

#include <QApplication>
#include <QString>

class TileWorldApp : public QApplication
{
public:
	TileWorldApp(int& argc, char** argv);
	~TileWorldApp();

	int RunTWorld();
	void ExitTWorld();
	void InitDirs();

	bool Initialize(bool bSilence, int nSoundBufSize, bool bShowHistogram);

	QString appResDir;
	QString userSetsDir;
	QString userDataDir;
	QString appDataDir;
	QString userDir;

private:
	bool m_bSilence, m_bShowHistogram;
};


extern TileWorldApp* g_pApp;
extern TileWorldMainWnd* g_pMainWnd;


#endif
