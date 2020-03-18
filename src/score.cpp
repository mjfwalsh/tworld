/* score.cpp: Calculating scores and formatting the display of same.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include	<stdlib.h>
#include	<string.h>

#include	<sstream>
#include	<vector>

#include	"defs.h"
#include	"play.h"
#include	"score.h"
#include	"err.h"

using std::ostringstream;
using std::vector;
using std::string;

/* Translate a number into a string, complete with commas.
 */
static std::string cdecimal(long number)
{
	string numWithCommas = std::to_string(number);
	int insertPosition = numWithCommas.length() - 3;
	while (insertPosition > 0) {
		numWithCommas.insert(insertPosition, ",");
		insertPosition-=3;
	}

	return numWithCommas;
}

/* Return the user's scores for a given level.
 */
int getscoresforlevel(gameseries const *series, int level,
	int *base, int *bonus, long *total)
{
	gamesetup   *game;
	long	totalscore;
	int		n;

	*base = 0;
	*bonus = 0;
	totalscore = 0;
	for (n = 0, game = series->games ; n < series->count ; ++n, ++game) {
		if (n >= series->allocated)
			break;
		int levelscore = 0;
		int timescore = 0;
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
void createscorelist(gameseries const *series, int usepasswds, int **plevellist,
	int *pcount, tablespec *rtable)
{
	gamesetup  *game;
	tableCell blank;
	std::vector<tableCell> table;
	int	       *levellist = NULL;
	int		levelscore, timescore;
	long	totalscore;
	int		count;

	levellist = (int*)malloc((series->count + 2) * sizeof *levellist);
	if (!levellist)
		memerrexit();

	totalscore = 0;

	table.push_back({1, RightAlign, "Level"});
	table.push_back({1, LeftAlign, "Name"});
	table.push_back({1, RightAlign, "Base"});
	table.push_back({1, RightAlign, "Bonus"});
	table.push_back({1, RightAlign, "Score"});

	blank = {4, LeftAlign, " "};

	int j;
	count = 0;
	for (j = 0, game = series->games ; j < series->count ; ++j, ++game) {
		if (j >= series->allocated)
			break;

		table.push_back({1, RightAlign, std::to_string(game->number)});

		if (hassolution(game)) {
			table.push_back({1, LeftAlign, game->name});

			if (game->sgflags & SGF_REPLACEABLE) {
				table.push_back({3, CenterAlign, " (Deleted) "});
			} else {
				levelscore = 500 * game->number;
				table.push_back({1, RightAlign, cdecimal(levelscore)});

				if (game->time) {
					timescore = 10 * (game->time - game->besttime / TICKS_PER_SECOND);
					table.push_back({1, RightAlign, cdecimal(timescore)});
				} else {
					timescore = 0;
					table.push_back({1, RightAlign, "---"});
				}
				table.push_back({1, RightAlign, cdecimal(levelscore + timescore)});
				totalscore += levelscore + timescore;
			}
			levellist[count] = j;
			++count;
		} else {
			if (!usepasswds || (game->sgflags & SGF_HASPASSWD)) {
				table.push_back({4, LeftAlign, game->name});

				levellist[count] = j;
			} else {
				table.push_back(blank);
				levellist[count] = -1;
			}
			++count;
		}
	}

	while (table[table.size()-1].text == blank.text
			&& table[table.size()-1].colspan == blank.colspan) {
		table.pop_back();
		table.pop_back();
		--count;
	}

	table.push_back({2, LeftAlign, "Total Score"});
	table.push_back({3, RightAlign, cdecimal(totalscore)});

	levellist[count] = -1;
	++count;

	*plevellist = levellist;
	*pcount = count;

	rtable->rows = count + 1;
	rtable->cols = 5;
	rtable->items = table;
}

/* Free the memory allocated by createscorelist() or createtimelist().
 */
void freescorelist(int *levellist)
{
	free(levellist);
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
