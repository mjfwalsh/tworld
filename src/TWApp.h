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
	bool Initialize();
};


extern TileWorldApp* g_pApp;
extern TileWorldMainWnd* g_pMainWnd;


#endif
