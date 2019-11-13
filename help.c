/* help.c: Displaying online help.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#include	"defs.h"
#include	"ver.h"
#include	"comptime.h"
#include	"help.h"

/* Help for command-line options.
 */
static char const *yowzitch_items[] = {
    "1-Usage:", "1!tworld [-hvVdlsbtpqrPFa] [-n N] [-DLRS DIR] "
		"[NAME] [SNAME] [LEVEL]",
    "1-   -p", "1!Disable password checking.",
    "1-   -F", "1!Run in fullscreen mode.",
    "1-   -q", "1!Run quietly.",
    "1-   -r", "1!Run in read-only mode; solutions will not be saved.",
    "1-   -P", "1!Put Lynx ruleset emulation in pedantic mode.",
    "1-   -n", "1!Set initial volume level to N.",
    "1-   -a", "1!Double the size of the sound buffer (can be repeated).",
    "1-   -l", "1!Display the list of available data files and exit.",
    "1-   -s", "1!Display scores for the selected data file and exit.",
    "1-   -t", "1!Display times for the selected data file and exit.",
    "1-   -b", "1!Batch-verify solutions for the selected data file and exit.",
    "1-   -h", "1!Display this help and exit.",
    "1-   -d", "1!Display default directories and exit.",
    "1-   -v", "1!Display version number and exit.",
    "1-   -V", "1!Display version and license information and exit.",
    "2!NAME specifies which data file to use.",
    "2!LEVEL specifies which level to start at.",
    "2!SNAME specifies an alternate solution file."
};
static tablespec const yowzitch_table = { 19, 2, 2, -1, yowzitch_items };
tablespec const *yowzitch = &yowzitch_table;

/* Version and license information.
 */
static char const *vourzhon_items[] = {
    "1+*", "1-Tile World: version " VERSION,
    "1+",  "1-Copyright (c) 2001-2017 by Brian Raiter, Madhav Shanbhag, and"
    	   " Eric Schmidt",
    "1+",  "1-compiled " COMPILE_TIME,
    "1+*", "1!This program is free software; you can redistribute it and/or"
	   " modify it under the terms of the GNU General Public License as"
	   " published by the Free Software Foundation; either version 2 of"
	   " the License, or (at your option) any later version.",
    "1+*", "1!This program is distributed in the hope that it will be"
	   " useful, but WITHOUT ANY WARRANTY; without even the implied"
	   " warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR"
	   " PURPOSE. See the GNU General Public License for more details.",
    "1+*", "1!Bug reports are appreciated, and can be sent to"
           " eric41293@comcast.net or CrapulentCretin@Yahoo.com."
};
static tablespec const vourzhon_table = { 6, 2, 1, -1, vourzhon_items };
tablespec const *vourzhon = &vourzhon_table;
