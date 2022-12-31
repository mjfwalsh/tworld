/* help.cpp: Displaying online help.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag,
 * Eric Schmidt and Michael Walsh.
 * Licenced under the GNU General Public Licence.
 * No warranty. See COPYING for details.
 */

#include	"help.h"
#include	"../obj/comptime.h"

/* Version and Licence information.
 */
char const *aboutText =
"<html><body>"

"<p align='center'><b>Tile World</b></p>"

"<p align='justify'>Copyright &copy; 2001-2022 by Brian Raiter, Madhav Shanbhag,"
" Eric Schmidt and Michael Walsh"

"<p align='justify'>The sound effects were created by Brian Raiter, using "
"<a href='https://sox.sourceforge.net/'>SoX</a>, and tile images were created by Anders Kaseorg, "
"using <a href='https://www.povray.org/'>POV-Ray</a>. Both the sound effects and the images have "
"been released into the public domain.</p>"

"<p align='justify'>This program is free software; you can redistribute it and/or"
" modify it under the terms of the <a href='file:licences/gpl2.html'>GNU General Public Licence "
"(version 2)</a> as published by the Free Software Foundation, or (at your option) any later "
"version of that licence.</p>"

"<p align='justify'>This program is distributed in the hope that it will be"
" useful, but WITHOUT ANY WARRANTY; without even the implied"
" warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR"
" PURPOSE. See the GNU General Public Licence for more details.</p>"

"<p align='justify'>Tile World is build using <a href='https://www.qt.io/'>Qt</a> and "
"<a href='https://www.libsdl.org/'>SDL</a> libraries. Qt is available under the "
"<a href='file:licences/lgpl.html'>GNU Lesser General Public License</a>, and SDL is "
"available under the <a href='file:licences/zlib.html'>zlib licence</a>.</p>"

"<p align='left'>You can obtain this program's source code at<br>"
"<a href='https://github.com/mjfwalsh/tworld'>github.com/mjfwalsh/tworld</a>.</p>"

"<p align='left'>Bug reports may be filed at<br>"
"<a href='https://github.com/mjfwalsh/tworld/issues'>github.com/mjfwalsh/tworld/issues</a>.</p>"

"<p align='center'>Compiled on " COMPILE_TIME ".</p>"

"</body></html>";
