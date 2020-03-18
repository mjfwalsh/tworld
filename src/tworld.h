/* tworld.c: Header for the original top-level module.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag,
 * Eric Schmidt and Michael Walsh
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#ifndef	HEADER_tworld_h_
#define	HEADER_tworld_h_

#include	"TWTableSpec.h"
#include	"defs.h"

enum { Play_None, Play_Normal, Play_Back, Play_Verify };

/* The data needed to identify what level is being played.
 */
typedef	struct gamespec {
	gameseries	series;		/* the complete set of levels */
	int		currentgame;	/* which level is currently selected */
	int		playmode;	/* which mode to play */
	int		usepasswds;	/* FALSE if passwords are to be ignored */
	int		status;		/* final status of last game played */
	int		enddisplay;	/* TRUE if the final level was completed */
	int		melindacount;	/* count for Melinda's free pass */
} gamespec;

/* Structure used to hold the complete list of available series.
 */
typedef	struct seriesdata {
	gameseries *list;		/* the array of available series */
	int		count;		/* size of array */
	mapfileinfo *mflist;	/* List of all levelset files */
	int		mfcount;	/* Number of levelset files */
	TWTableSpec	*table;		/* table for displaying the array */
} seriesdata;

/* Load the levelset history.
 */
extern int loadhistory(void);

#endif
