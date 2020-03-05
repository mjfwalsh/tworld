/* Copyright (C) 2001-2019 by Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef TWAPP_H
#define TWAPP_H

class TileWorldMainWnd;

#include <QApplication>

class TileWorldApp : public QApplication
{
public:
	TileWorldApp(int& argc, char** argv);
	~TileWorldApp();

	void ExitTWorld();
	void InitDirs();
	const char *GetDir(int t);
	bool Initialize();

private:
	char *appResDir;
	char *userSetsDir;
	char *userDataDir;
	char *appDataDir;
	char *userSolDir;
	char *userDir;
};


extern TileWorldApp* g_pApp;
extern TileWorldMainWnd* g_pMainWnd;


#endif
