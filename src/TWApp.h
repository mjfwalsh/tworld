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

	// Copy text to clipboard.
	static void CopyToClipboard(QString text);

	// Beep
	static void Bell();
};

extern TileWorldApp* g_pApp;
extern TileWorldMainWnd* g_pMainWnd;

/* Process all pending events. If wait is TRUE and no events are
 * currently pending, the function blocks until an event arrives.
 */
extern void eventupdate(bool wait);


#endif
