/* score.h: Calculating and formatting the scores.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_score_h_
#define	HEADER_score_h_

/* Return the user's scores for a given level. The last three arguments
 * receive the base score for the level, the time bonus for the level,
 * and the total score for the series.
 */
extern bool getscoresforlevel(gameseries const *series, int level,
				 int *base, int *bonus, long *total);

/* Produce a table showing the player's scores for the given series,
 * formatted in columns. Each level in the series is listed in a
 * separate row, with a header row and an extra row at the end giving
 * a grand total. The pointer pointed to by plevellist receives an
 * array of level indexes to match the rows of the table (less one for
 * the header row), or -1 if no level is displayed in that row. If
 * usepasswds is TRUE, levels for which the user has not learned the
 * password will either not be included or will show no title. FALSE
 * is returned if an error occurs.
 */
extern void createscorelist(gameseries const *series, bool usepasswds,
			   int **plevellist, int *pcount, TWTableSpec *table);


/* Free all memory allocated by the above functions.
 */
extern void freescorelist(int *plevellist);

/* Create a string representing the level time achieved for a level. */
QString timestring(int lvlnum,  QString lvltitle, int besttime,
	int timed, int bad);

/* Create a list of timestrings for all levels with solutions in the
 * gameseries. */
QString leveltimes(gameseries const *series);


#endif
