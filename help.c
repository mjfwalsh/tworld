/* help.c: Displaying online help.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag,
 * Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#ifndef COMPILE_TIME
#include    "comptime.h"
#endif
#include	"help.h"


/* Version and license information.
 */
char const *aboutText =
"<html><body>"

"<p align='center'><b>Tile World</b></p>"

"<p align='justify'>Copyright &copy; 2001-2020 by Brian Raiter, Madhav Shanbhag,"
" Eric Schmidt and Michael Walsh"

"<p align='justify'>This program is free software; you can redistribute it and/or"
" modify it under the terms of the GNU General Public License as"
" published by the Free Software Foundation; either version 2 of"
" the License, or (at your option) any later version.</p>"

"<p align='justify'>This program is distributed in the hope that it will be"
" useful, but WITHOUT ANY WARRANTY; without even the implied"
" warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR"
" PURPOSE. See the GNU General Public License for more details.</p>"

"<p align='center'>Bug reports are appreciated and can be filed at<br>"
"<a href='https://github.com/mjfwalsh/tworld/issues'>https://github.com/mjfwalsh/tworld/issues</a></p>"

"<p align='center'>Compiled on " COMPILE_TIME ".</p>"

"</body></html>";
