/* unslist.h: Functions to manage the list of unsolvable levels.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_unslist_h_
#define	HEADER_unslist_h_

#include	"defs.h"

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* Read the list of unsolvable levels from the given filename. If the
 * filename does not contain a path, then the function looks for the
 * file in the resource directory and the user's save directory.
 */
OSHW_EXTERN int loadunslistfromfile(char const *filename);

/* Look up all the levels in the given series, and mark the ones that
 * appear in the list of unsolvable levels by initializing the
 * unsolvable field. Levels that do not appear in the list will have
 * their unsolvable fields explicitly set to NULL. The number of
 * unsolvable levels is returned.
 */
extern int markunsolvablelevels(gameseries *series);

#undef OSHW_EXTERN

#endif
