/* score.cpp: Calculating scores and formatting the display of same.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	<sstream>

#include	"defs.h"
#include	"err.h"
#include	"play.h"
#include	"score.h"

using std::ostringstream;
using std::string;

/* Translate a number into a string.
 */
static char const *decimal(long number)
{
	static char		buf[32];
	char	       *dest = buf + sizeof buf;
	unsigned long	n;

	n = number >= 0 ? (unsigned long)number : (unsigned long)-(number + 1) + 1;
	*--dest = '\0';
	do {
		*--dest = '0' + n % 10;
		n /= 10;
	} while (n);
	if (number < 0)
		*--dest = '-';
	return dest;
}

/* Translate a number into a string, complete with commas.
 */
static char const *cdecimal(long number)
{
	static char		buf[32];
	char	       *dest = buf + sizeof buf;
	unsigned long	n;
	int			i = 0;

	n = number >= 0 ? (unsigned long)number : (unsigned long)-(number + 1) + 1;
	*--dest = '\0';
	do {
		++i;
		if (i % 4 == 0) {
			*--dest = ',';
			++i;
		}
		*--dest = '0' + n % 10;
		n /= 10;
	} while (n);
	if (number < 0)
		*--dest = '-';
	return dest;
}

/* Return the user's scores for a given level.
 */
int getscoresforlevel(gameseries const *series, int level,
	int *base, int *bonus, long *total)
{
	gamesetup   *game;
	int		levelscore, timescore;
	long	totalscore;
	int		n;

	*base = 0;
	*bonus = 0;
	totalscore = 0;
	for (n = 0, game = series->games ; n < series->count ; ++n, ++game) {
		if (n >= series->allocated)
			break;
		levelscore = 0;
		timescore = 0;
		if (hassolution(game)) {
			levelscore = game->number * 500;
			if (game->time)
				timescore = 10 * (game->time
					- game->besttime / TICKS_PER_SECOND);
		}
		if (n == level) {
			*base = levelscore;
			*bonus = timescore;
		}
		totalscore += levelscore + timescore;
	}
	*total = totalscore;
	return TRUE;
}

/* Produce a table that displays the user's score, broken down by
 * levels with a grand total at the end. If usepasswds is FALSE, all
 * levels are displayed. Otherwise, levels after the last level for
 * which the user knows the password are left out. Other levels for
 * which the user doesn't know the password are in the table, but
 * without any information besides the level's number.
 */
int createscorelist(gameseries const *series, int usepasswds, int **plevellist,
	int *pcount, tablespec *table)
{
	gamesetup  *game;
	char const **ptrs;
	char       *textheap;
	char       *blank;
	int	       *levellist = NULL;
	int		levelscore, timescore;
	long	totalscore;
	int		count;
	int		used, j, n;

	if (plevellist) {
		levellist = (int*)malloc((series->count + 2) * sizeof *levellist);
		if (!levellist)
			memerrexit();
	}
	ptrs = (char const **)malloc((series->count + 2) * 5 * sizeof *ptrs);
	textheap = (char*)malloc((series->count + 2) * 128);
	if (!ptrs || !textheap)
		memerrexit();
	totalscore = 0;

	n = 0;
	used = 0;
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "1+Level");
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "1-Name");
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "1+Base");
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "1+Bonus");
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "1+Score");

	blank = textheap + used;
	used += 1 + sprintf(textheap + used, "4- ");

	count = 0;
	for (j = 0, game = series->games ; j < series->count ; ++j, ++game) {
		if (j >= series->allocated)
			break;

		ptrs[n++] = textheap + used;
		used += 1 + sprintf(textheap + used, "1+%s", decimal(game->number));
		if (hassolution(game)) {
			ptrs[n++] = textheap + used;
			used += 1 + sprintf(textheap + used, "1-%.64s", game->name);
			if (game->sgflags & SGF_REPLACEABLE) {
				ptrs[n++] = textheap + used;
				used += 1 + sprintf(textheap + used, "3.*BAD*");
			} else {
				levelscore = 500 * game->number;
				ptrs[n++] = textheap + used;
				used += 1 + sprintf(textheap + used, "1+%s",
					cdecimal(levelscore));
				ptrs[n++] = textheap + used;
				if (game->time) {
					timescore = 10 * (game->time
						- game->besttime / TICKS_PER_SECOND);
					used += 1 + sprintf(textheap + used, "1+%s",
						cdecimal(timescore));
				} else {
					timescore = 0;
					strcpy(textheap + used, "1+---");
					used += 6;
				}
				ptrs[n++] = textheap + used;
				used += 1 + sprintf(textheap + used, "1+%s",
					cdecimal(levelscore + timescore));
				totalscore += levelscore + timescore;
			}
			if (plevellist)
				levellist[count] = j;
			++count;
		} else {
			if (!usepasswds || (game->sgflags & SGF_HASPASSWD)) {
				ptrs[n++] = textheap + used;
				used += 1 + sprintf(textheap + used, "4-%s", game->name);
				if (plevellist)
					levellist[count] = j;
			} else {
				ptrs[n++] = blank;
				if (plevellist)
					levellist[count] = -1;
			}
			++count;
		}
	}

	while (ptrs[n - 1] == blank) {
		n -= 2;
		--count;
	}

	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "2-Total Score");
	ptrs[n++] = textheap + used;
	used += 1 + sprintf(textheap + used, "3+%s", cdecimal(totalscore));
	if (plevellist)
		levellist[count] = -1;
	++count;

	if (plevellist)
		*plevellist = levellist;
	if (pcount)
		*pcount = count;

	table->rows = count + 1;
	table->cols = 5;
	table->items = ptrs;

	return TRUE;
}

/* Free the memory allocated by createscorelist() or createtimelist().
 */
void freescorelist(int *levellist, tablespec *table)
{
	free(levellist);
	if (table) {
		free((void*)table->items[0]);
		free(table->items);
	}
}

char const* timestring(int lvlnum, char const *lvltitle, int besttime,
	int timed, int bad)
{
	static string result;

	ostringstream oss;
	oss << '#' << lvlnum << " (" << lvltitle << "): ";
	if (!timed) oss << '[';
	oss << besttime;
	if (!timed) oss << ']';
	if (bad) oss << " *bad*";
	oss << '\n';

	result = oss.str();
	return result.c_str();
}

char const* leveltimes(gameseries const *series)
{
	static string result;
	result.clear();

	for (int i = 0; i < series->count; ++i) {
		gamesetup const & game = series->games[i];
		if (hassolution(&game)) {
			int const starttime = (game.time ? game.time : 999);
			int const besttime = starttime - game.besttime / TICKS_PER_SECOND;
			bool const timed = (game.time > 0);
			bool const bad = (game.sgflags & SGF_REPLACEABLE);
			result += timestring(game.number, game.name, besttime, timed, bad);
		}
	}

	return result.c_str();
}
