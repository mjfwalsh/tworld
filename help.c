/* help.c: Displaying online help.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	"defs.h"
#include	"ver.h"
#include	"comptime.h"
#include	"help.h"


/* Version and license information.
 */
static char const *vourzhon_items[] = {
"1-Tile World: version " VERSION,

"1-Copyright (c) 2001-2017 by Brian Raiter, Madhav Shanbhag, and"
" Eric Schmidt",

"1-compiled " COMPILE_TIME,

"1!This program is free software; you can redistribute it and/or"
" modify it under the terms of the GNU General Public License as"
" published by the Free Software Foundation; either version 2 of"
" the License, or (at your option) any later version.",

"1!This program is distributed in the hope that it will be"
" useful, but WITHOUT ANY WARRANTY; without even the implied"
" warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR"
" PURPOSE. See the GNU General Public License for more details.",

"1!Bug reports are appreciated, and can be sent to"
" eric41293@comcast.net or CrapulentCretin@Yahoo.com."
};
static tablespec const vourzhon_table = { 6, 1, vourzhon_items };
tablespec const *vourzhon = &vourzhon_table;
