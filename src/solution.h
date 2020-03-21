/* solution.c: Functions for reading and writing the solution files.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_solution_h_
#define	HEADER_solution_h_

#include	"TWTableSpec.h"
#include	"defs.h"

/* A structure holding all the data needed to reconstruct a solution.
 */
typedef	struct solutioninfo {
	actlist		moves;		/* the actual moves of the solution */
	unsigned long	rndseed;	/* the PRNG's initial seed */
	unsigned long	flags;		/* other flags (currently unused) */
	unsigned char	rndslidedir;	/* random slide's initial direction */
	signed char		stepping;	/* the timer offset */
} solutioninfo;

/* Initialize or reinitialize list as empty.
 */
extern void initmovelist(actlist *list);

/* Append move to the end of list.
 */
extern void addtomovelist(actlist *list, action move);

/* Make to an independent copy of from.
 */
extern void copymovelist(actlist *to, actlist const *from);

/* Deallocate list.
 */
extern void destroymovelist(actlist *list);

/* Expand a level's solution data into the actual solution, including
 * the full list of moves. FALSE is returned if the solution is
 * invalid or absent.
 */
extern bool expandsolution(solutioninfo *solution, gamesetup const *game);

/* Take the given solution and compress it, storing the compressed
 * data as part of the level's setup. FALSE is returned if an error
 * occurs. (It is not an error to compress the null solution.)
 */
extern bool contractsolution(solutioninfo const *solution, gamesetup *game);

/* Read all the solutions for the given series into memory. FALSE is
 * returned if an error occurs. Note that it is not an error for the
 * solution file to not exist.
 */
extern bool readsolutions(gameseries *series);

/* Write out all the solutions for the given series. The solution file
 * is created if it does not currently exist. The solution file's
 * directory is also created if it does not currently exist. (Nothing
 * is done if the directory's name has been unset, however.) FALSE is
 * returned if an error occurs.
 */
extern bool savesolutions(gameseries *series);

/* Free all memory allocated for storing the game's solutions, and mark
 * the levels as being unsolved.
 */
extern void clearsolutions(gameseries *series);

/* Produce a list of available solution files associated with the
 * given series (i.e. that have the name of the series as their
 * prefix). An array of filenames is returned through pfilelist, the
 * array's size is returned through pcount, and the table of the
 * filenames is returned through table. FALSE is returned if no table
 * was returned.
 */
extern bool createsolutionfilelist(gameseries const *series,
	std::vector<std::string> *filelist, int *pcount, TWTableSpec *table);

#endif
