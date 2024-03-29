/* solution.cpp: Functions for reading and writing the solution files.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<QStringList>

#include	"TWTableSpec.h"
#include	"defs.h"
#include	"fileio.h"
#include	"series.h"
#include	"solution.h"
#include	"err.h"

/*
 * The following is a description of the solution file format. Note that
 * numeric values are always stored in little-endian order, consistent
 * with the data file.
 *
 * The header is at least eight bytes long, and contains the following
 * values:
 *
 * HEADER
 *  0-3   signature bytes (35 33 9B 99)
 *   4    ruleset (1=Lynx, 2=MS)
 *   5    other options (currently always zero)
 *   6    other options (currently always zero)
 *   7    count of bytes in remainder of header (currently always zero)
 *
 * After the header are level solutions, usually but not necessarily
 * in numerical order. Each solution begins with the following values:
 *
 * PER LEVEL
 *  0-3   offset to next solution (from the end of this field)
 *  4-5   level number
 *  6-9   level password (four ASCII characters in "cleartext")
 *  10    other flags (currently always zero)
 *  11    initial random slide direction and stepping value
 * 12-15  initial random number generator value
 * 16-19  time of solution in ticks
 * 20-xx  solution bytes
 *
 * If the offset field is 0, then none of the other fields are
 * present. (This permits the file to contain padding.) If the offset
 * field is 6, then the level's number and password are present but
 * without a saved game. Otherwise, the offset should never be less
 * than 16.
 *
 * Note that byte 11 contains the initial random slide direction in
 * the bottom three bits, and the initial stepping value in the next
 * three bits. The top two bits are unused. (The initial random slide
 * direction is always zero under the MS ruleset.)
 *
 * The solution bytes consist of a stream of values indicating the
 * moves of the solution. The values vary in size from one to five
 * bytes in length. The size of each value is always specified in the
 * first byte. There are five different formats for the values. (Note
 * that in the following byte diagrams, the bits are laid out
 * little-endian instead of the usual MSB-first.)
 *
 * The first format can be either one or two bytes long. The two-byte
 * form is shown here:
 *
 * #1: 01234567 89012345
 *     NNDDDTTT TTTTTTTT
 *
 * The two lowest bits, marked with Ns, contain either one (01) or two
 * (10), and indicate how many bytes are used. The next three bits,
 * marked with Ds, contain the direction of the move. The remaining
 * bits are marked with Ts, and these indicate the amount of time, in
 * ticks, between this move and the prior move, less one. (Thus, a
 * value of T=0 indicates a move that occurs on the tick immediately
 * following the previous move.) The very first move of a solution is
 * an exception: it is not decremented, as that would sometimes
 * require a negative value to be stored. If the one-byte version is
 * used, then T is only three bits in size; otherwise T is 11 bits
 * long.
 *
 * The second format is four bytes in length:
 *
 * #2: 01234567 89012345 67890123 45678901
 *     11DD0TTT TTTTTTTT TTTTTTTT TTTTTTTT
 *
 * This format allocates 27 bits for the time value. (The top four
 * bits will always be zero, however, as the game's timer is currently
 * limited to 23 bits.) Since there are only two bits available for
 * storing the direction, this format can only be used to store
 * orthogonal moves (i.e. it cannot be used to store a Lynx diagonal
 * move).
 *
 * The third format has the form:
 *
 * #3: 01234567
 *     00DDEEFF
 *
 * This value encodes three separate moves (DD, EE, and FF) packed
 * into a single byte. Each move has an implicit time value of four
 * ticks separating it from the prior move (i.e. T=3). Like the second
 * format, only two bits are used to store each move.
 *
 * The fourth and final format, like the first format, can vary in
 * size. It can be two, three, four, or five bytes long, depending on
 * how many bits are needed to store the time interval. It is shown
 * here in its largest form:
 *
 * #4: 01234567 89012345 67890123 45678901 23456789
 *     11NN1DDD DDDDDDTT TTTTTTTT TTTTTTTT TTTTTTTT
 *
 * The two bits marked NN indicate the size of the field in bytes,
 * less two (i.e., 00 for a two-byte value, 11 for a five-byte value).
 * Seven bits are used to indicate the move's direction, which allows
 * this field to store MS mouse moves. The time value is encoded
 * normally, and can be 2, 10, 18, or 26 bits long.
 */

/* The signature bytes of the solution files.
 */
#define	CSSIG		0x999B3335UL

/* The signature bytes for each ruleset.
 */
#define SIG_SOLFILE_LYNX 1
#define SIG_SOLFILE_MS 2

/* Translate move directions between three-bit and four-bit
 * representations.
 *
 * 0 = NORTH	    = 0001 = 1
 * 1 = WEST	    = 0010 = 2
 * 2 = SOUTH	    = 0100 = 4
 * 3 = EAST	    = 1000 = 8
 * 4 = NORTH | WEST = 0011 = 3
 * 5 = SOUTH | WEST = 0110 = 6
 * 6 = NORTH | EAST = 1001 = 9
 * 7 = SOUTH | EAST = 1100 = 12
 */
static int const diridx8[16] = {
	-1,  0,  1,  4,  2, -1,  5, -1,  3,  6, -1, -1,  7, -1, -1, -1
};
static int const idxdir8[8] = {
	NORTH, WEST, SOUTH, EAST,
	NORTH | WEST, SOUTH | WEST, NORTH | EAST, SOUTH | EAST
};

#define	isdirectmove(dir)	(directionalcmd(dir))
#define	ismousemove(dir)	(!isdirectmove(dir))
#define	isdiagonal(dir)		(isdirectmove(dir) && diridx8[dir] > 3)
#define isorthogonal(dir)	(isdirectmove(dir) && diridx8[dir] <= 3)
#define	dirtoindex(dir)		(diridx8[dir])
#define	indextodir(dir)		(idxdir8[dir])

/* TRUE if file modification is prohibited.
 */
bool		readonly = false;

/*
 * Functions for manipulating move lists.
 */

/* Initialize or reinitialize list as empty.
 */
void initmovelist(actlist *list)
{
	if (!list->allocated || !list->list) {
		list->allocated = 16;
		x_type_alloc(action, list->list, list->allocated * sizeof *list->list);
	}
	list->count = 0;
}

/* Append move to the end of list.
 */
void addtomovelist(actlist *list, action move)
{
	if (list->count >= list->allocated) {
		list->allocated *= 2;
		x_type_alloc(action, list->list, list->allocated * sizeof *list->list);
	}
	list->list[list->count++] = move;
}

/* Deallocate list.
 */
void destroymovelist(actlist *list)
{
	if (list->list)
		free(list->list);
	list->allocated = 0;
	list->list = NULL;
}

/*
 * Functions for handling the solution file header.
 */

/* Read the header bytes of the given solution file. extra
 * receives any bytes in the header that this code doesn't recognize.
 */
static bool readsolutionheader(fileinfo &file, int ruleset,
	int *extrasize, unsigned char *extra)
{
	unsigned long	sig;
	unsigned short	f;
	unsigned char	n;

	if (!file.readint32(&sig, "not a valid solution file"))
		return false;
	if (sig != CSSIG)
		return fileerr(&file, "not a valid solution file");
	if (!file.readint8(&n, "not a valid solution file"))
		return false;

	switch (n) {
		case SIG_SOLFILE_MS:	n = Ruleset_MS;		break;
		case SIG_SOLFILE_LYNX:	n = Ruleset_Lynx;	break;
		default: fileerr(&file, "solution file is for an unrecognised ruleset");
	}

	if (n != ruleset)
		return fileerr(&file, "solution file is for a different ruleset"
			" than the level set file");
	if (!file.readint16(&f, "not a valid solution file"))
		return false;
	(void)f; // ignored at moment

	if (!file.readint8(&n, "not a valid solution file"))
		return false;
	*extrasize = n;
	if (n)
		if (!file.read(extra, *extrasize, "not a valid solution file"))
			return false;

	return true;
}

/* Write the header bytes to the given solution file.
 */
static bool writesolutionheader(fileinfo &file, int ruleset,
	int extrasize, unsigned char const *extra)
{
	switch (ruleset) {
		case Ruleset_MS:	ruleset = SIG_SOLFILE_MS;		break;
		case Ruleset_Lynx:	ruleset = SIG_SOLFILE_LYNX;		break;
		default: return false;
	}

	return file.writeint32(CSSIG)
		&& file.writeint8(ruleset)
		&& file.writeint16(0) // ignored at moment
		&& file.writeint8(extrasize)
		&& file.write(extra, extrasize);
}

/* Write the name of the level set to the given solution file.
 */
static int writesolutionsetname(fileinfo &file, char const *setname)
{
	const char zeroes[16] = "";
	const int n = strlen(setname) + 1;
	return file.writeint32(n + 16)
		&& file.write(zeroes, 16)
		&& file.write(setname, n);
}

/*
 * Solution translation.
 */

/* Expand a level's solution data into an actual list of moves.
 */
bool expandsolution(solutioninfo *solution, gamesetup const *game)
{
	unsigned char const	       *dataend;
	unsigned char const	       *p;
	action			act;
	int				n;

	if (game->solutionsize <= 16)
		return false;

	solution->flags = game->solutiondata[6];
	solution->rndslidedir = indextodir(game->solutiondata[7] & 7);
	solution->stepping = (game->solutiondata[7] >> 3) & 7;
	solution->rndseed = game->solutiondata[8] | (game->solutiondata[9] << 8)
		| (game->solutiondata[10] << 16)
		| (game->solutiondata[11] << 24);

	initmovelist(&solution->moves);
	act.when = -1;
	p = game->solutiondata + 16;
	dataend = game->solutiondata + game->solutionsize;
	while (p < dataend) {
		switch (*p & 0x03) {
		case 0:
			act.dir = indextodir((*p >> 2) & 0x03);
			act.when += 4;
			addtomovelist(&solution->moves, act);
			act.dir = indextodir((*p >> 4) & 0x03);
			act.when += 4;
			addtomovelist(&solution->moves, act);
			act.dir = indextodir((*p >> 6) & 0x03);
			act.when += 4;
			addtomovelist(&solution->moves, act);
			++p;
			break;
		case 1:
			act.dir = indextodir((*p >> 2) & 0x07);
			act.when += ((*p >> 5) & 0x07) + 1;
			addtomovelist(&solution->moves, act);
			++p;
			break;
		case 2:
			if (p + 2 > dataend)
				goto truncated;
			act.dir = indextodir((*p >> 2) & 0x07);
			act.when += ((p[0] >> 5) & 0x07) + ((unsigned long)p[1] << 3) + 1;
			addtomovelist(&solution->moves, act);
			p += 2;
			break;
		case 3:
			if (*p & 0x10) {
				n = (*p >> 2) & 0x03;
				if (p + 2 + n > dataend)
					goto truncated;
				act.dir = ((p[0] >> 5) & 0x07) | ((p[1] & 0x3F) << 3);
				act.when += (p[1] >> 6) & 0x03;
				while (n--)
					act.when += (unsigned long)p[2 + n] << (2 + n * 8);
				++act.when;
				p += 2 + ((*p >> 2) & 0x03);
			} else {
				if (p + 4 > dataend)
					goto truncated;
				act.dir = indextodir((*p >> 2) & 0x03);
				act.when += ((p[0] >> 5) & 0x07) | ((unsigned long)p[1] << 3)
					| ((unsigned long)p[2] << 11)
					| ((unsigned long)p[3] << 19);
				++act.when;
				p += 4;
			}
			addtomovelist(&solution->moves, act);
			break;
		}
	}
	return true;

truncated:
	warn("level %d: truncated solution data", game->number);
	initmovelist(&solution->moves);
	return false;
}

/* Take the given solution and compress it, storing the compressed
 * data as part of the level's setup.
 */
bool contractsolution(solutioninfo const *solution, gamesetup *game)
{
	action const       *move;
	unsigned char      *data;
	int			size, when;

	free(game->solutiondata);
	game->solutionsize = 0;
	game->solutiondata = NULL;
	if (!solution->moves.count)
		return true;

	size = 21;
	move = solution->moves.list + 1;
	for (int i = 1 ; i < solution->moves.count ; ++i, ++move)
		size += !isorthogonal(move->dir) ? 5
			: move[0].when - move[-1].when <= (1 << 3) ? 1
			: move[0].when - move[-1].when <= (1 << 11) ? 2 : 4;
	data = (unsigned char *)malloc(size);
	if (!data) {
		warn("failed to record level %d solution:"
			" out of memory", game->number);
		return false;
	}

	data[0] = game->number & 0xFF;
	data[1] = (game->number >> 8) & 0xFF;
	data[2] = game->passwd[0];
	data[3] = game->passwd[1];
	data[4] = game->passwd[2];
	data[5] = game->passwd[3];
	data[6] = solution->flags;
	data[7] = dirtoindex(solution->rndslidedir) | (solution->stepping << 3);
	data[8] = solution->rndseed & 0xFF;
	data[9] = (solution->rndseed >> 8) & 0xFF;
	data[10] = (solution->rndseed >> 16) & 0xFF;
	data[11] = (solution->rndseed >> 24) & 0xFF;
	data[12] = game->besttime & 0xFF;
	data[13] = (game->besttime >> 8) & 0xFF;
	data[14] = (game->besttime >> 16) & 0xFF;
	data[15] = (game->besttime >> 24) & 0xFF;

	when = -1;
	size = 16;
	move = solution->moves.list;
	for (int i = 0 ; i < solution->moves.count ; ++i, ++move) {
		int delta = -when - 1;
		when = move->when;
		delta += when;
		if (ismousemove(move->dir)
			|| (isdiagonal(move->dir) && delta >= (1 << 11))) {
			data[size] = 0x13 | ((move->dir << 5) & 0xE0);
			data[size + 1] = ((move->dir >> 3) & 0x3F) | ((delta & 0x03) << 6);
			if (delta < (1 << 2)) {
				size += 2;
			} else {
				data[size + 2] = (delta >> 2) & 0xFF;
				if (delta < (1 << 10)) {
					data[size] |= 1 << 2;
					size += 3;
				} else {
					data[size + 3] = (delta >> 10) & 0xFF;
					if (delta < (1 << 18)) {
						data[size] |= 2 << 2;
						size += 4;
					} else {
						data[size + 4] = (delta >> 18) & 0xFF;
						data[size] |= 3 << 2;
						size += 5;
					}
				}
			}
		} else if (delta == 3 && i + 2 < solution->moves.count
			&& isorthogonal(move[0].dir)
				&& move[1].when - move[0].when == 4
				&& isorthogonal(move[1].dir)
				&& move[2].when - move[1].when == 4
				&& isorthogonal(move[2].dir)) {
			data[size++] = (dirtoindex(move[0].dir) << 2)
				| (dirtoindex(move[1].dir) << 4)
				| (dirtoindex(move[2].dir) << 6);
			move += 2;
			i += 2;
			when = move->when;
		} else if (delta < (1 << 3)) {
			data[size++] = 0x01 | (dirtoindex(move->dir) << 2)
				| ((delta << 5) & 0xE0);
		} else if (delta < (1 << 11)) {
			data[size++] = 0x02 | (dirtoindex(move->dir) << 2)
				| ((delta << 5) & 0xE0);
			data[size++] = (delta >> 3) & 0xFF;
		} else {
			data[size++] = 0x03 | (dirtoindex(move->dir) << 2)
				| ((delta << 5) & 0xE0);
			data[size++] = (delta >> 3) & 0xFF;
			data[size++] = (delta >> 11) & 0xFF;
			data[size++] = (delta >> 19) & 0xFF;
		}
	}

	game->solutionsize = size;
	game->solutiondata = (unsigned char *)realloc(data, size);
	if (!game->solutiondata)
	{
		warn("failed to record level %d solution:"
			" out of memory", game->number);
		return false;
	}
	return true;
}

/*
 * File I/O for level solutions.
 */

/* Read the data of a one complete solution from the given file into
 * the appropriate fields of game.
 */
static bool readsolution(fileinfo &file, gamesetup *game)
{
	unsigned long	size;

	game->number = 0;
	game->sgflags = 0;
	game->besttime = TIME_NIL;
	game->solutionsize = 0;
	game->solutiondata = NULL;
	if (!file.isopen())
		return true;

	if (!file.readint32(&size) || size == 0xFFFFFFFF)
		return false;
	if (!size)
		return true;
	game->solutionsize = size;
	game->solutiondata = file.readbuf(size, "unexpected EOF");
	if (!game->solutiondata || (size <= 16 && size != 6))
		return fileerr(&file, "invalid data in solution file");
	game->number = (game->solutiondata[1] << 8) | game->solutiondata[0];
	memcpy(game->passwd, game->solutiondata + 2, 4);
	game->passwd[4] = '\0';
	game->sgflags |= SGF_HASPASSWD;
	if (size == 6)
		return true;

	game->besttime = game->solutiondata[12] | (game->solutiondata[13] << 8)
		| (game->solutiondata[14] << 16)
		| (game->solutiondata[15] << 24);
	size -= 16;
	if (!game->number && !*game->passwd) {
		game->sgflags |= SGF_SETNAME;
		if (size > 255)
			size = 255;
		memcpy(game->name, game->solutiondata + 16, size);
		game->name[size] = '\0';
		free(game->solutiondata);
		game->solutionsize = 0;
		game->solutiondata = NULL;
	}

	return true;
}

/* Write the data of one complete solution from the appropriate fields
 * of game to the given file.
 */
static bool writesolution(fileinfo &file, gamesetup const *game)
{
	if (game->solutionsize && (game->sgflags & SGF_REPLACEABLE) == 0) {
		if (!file.writeint32(game->solutionsize, "write error")
			|| !file.write(game->solutiondata,
				game->solutionsize, "write error"))
			return false;
	} else if (game->sgflags & SGF_HASPASSWD) {
		if (!file.writeint32(6, "write error")
			|| !file.writeint16(game->number, "write error")
			|| !file.write(game->passwd, 4, "write error"))
			return false;
	}

	return true;
}

/*
 * File I/O for solution files.
 */

/* The solution file is a based on the corresponding dac file
 */
static void setsolutionfilename(gameseries *series)
{
	if (!series->savefilename) {
		int n = strlen(series->name);
		x_cmalloc(series->savefilename, n + 5);
		memcpy(series->savefilename, series->name, n);
		memcpy(series->savefilename + n, ".tws", 5);
	}
}

/* Open the solution file.
 */
static bool opensolutionfile(gameseries *series, fileinfo &file, bool writable)
{
	if (writable && readonly)
		return false;

	return file.open(writable ? "wb" : "rb", writable ? "can't access file" : NULL);
}

/* Read the saved solution data for the given series into memory.
 */
bool readsolutions(gameseries *series)
{
	gamesetup	gametmp = {0};

	if (series->gsflags & GSF_NODEFAULTSAVE) {
		series->solheadersize = 0;
		return true;
	}

	setsolutionfilename(series);
	fileinfo file(SOLUTIONDIR, series->savefilename);

	if (!opensolutionfile(series, file, false)) {
		series->solheadersize = 0;
		return true;
	}

	if (!readsolutionheader(file, series->ruleset, &series->solheadersize, series->solheader))
		return false;

	while (readsolution(file, &gametmp)) {
		if (gametmp.sgflags & SGF_SETNAME) {
			if (strcmp(gametmp.name, series->name)) {
				warn("%s: ignoring solution file %s as it was"
					" recorded for a different level set: %s", series->name,
					series->savefilename, gametmp.name);
				series->gsflags |= GSF_NOSAVING;
				return false;
			}
			continue;
		}

		int n = findlevelinseries(series, gametmp.number, gametmp.passwd);
		if (n < 0) {
			n = findlevelinseries(series, 0, gametmp.passwd);
			if (n < 0) {
				fileerr(&file,
					"unmatched password in solution file");
				continue;
			}
			warn("level %d has been moved to level %d",
				gametmp.number, series->games[n].number);
		}
		series->games[n].besttime = gametmp.besttime;
		series->games[n].sgflags = gametmp.sgflags;
		series->games[n].solutionsize = gametmp.solutionsize;
		series->games[n].solutiondata = gametmp.solutiondata;
	}

	file.close();
	return true;
}

/* Write out all the solutions for the given series.
 */
bool savesolutions(gameseries *series)
{
	gamesetup  *game;
	int		i;

	if (readonly || (series->gsflags & GSF_NOSAVING))
		return true;

	if (series->gsflags & GSF_NODEFAULTSAVE)
		return true;

	setsolutionfilename(series);

	fileinfo file(SOLUTIONDIR, series->savefilename);

	if (!opensolutionfile(series, file, true))
		return false;

	if (!writesolutionheader(file, series->ruleset,
			series->solheadersize, series->solheader))
		return fileerr(&file,
			"saved-game file has become corrupted!");
	if (!writesolutionsetname(file, series->name))
		return fileerr(&file,
			"saved-game file has become corrupted!");
	for (i = 0, game = series->games ; i < series->count ; ++i, ++game) {
		if (!writesolution(file, game))
			return fileerr(&file,
				"saved-game file has become corrupted!");
	}

	file.close();
	return true;
}

/* Free all memory allocated for storing the game's solutions, and mark
 * the levels as being unsolved.
 */
void clearsolutions(gameseries *series)
{
	gamesetup  *game;
	int		n;

	for (n = 0, game = series->games ; n < series->count ; ++n, ++game) {
		free(game->solutiondata);
		game->besttime = TIME_NIL;
		game->sgflags = 0;
		game->solutionsize = 0;
		game->solutiondata = NULL;
	}
	series->solheadersize = 0;

	if(series->savefilename)
		free(series->savefilename);
}

/*
 * Creating the list of solution files.
 */

/* Mini-structure for passing data in and out of findfiles().
 */
typedef	struct solutiondata {
	QStringList filelist;
	char const *prefix;		/* the filename prefix to seek */
	int		prefixlen;	/* length of the filename prefix */
} solutiondata;

/* If the given file starts with the prefix stored in the solutiondata
 * structure, then add it to the list of filenames. This function is
 * a callback for findfiles().
 */
static bool getsolutionfile(char const *filename, int curdir, void *data)
{
	solutiondata       *sdata = (solutiondata *)data;

	if (!memcmp(filename, sdata->prefix, sdata->prefixlen)) {
		sdata->filelist.push_back(filename);
	}
	return true;
}

/* Produce a list of available solution files associated with the
 * given series (i.e. that have the name of the series as their
 * prefix). An array of filenames is returned through pfilelist, the
 * array's size is returned through pcount, and the table of the
 * filenames is returned through table. FALSE is returned if no table
 * was returned.
 */
bool createsolutionfilelist(gameseries const *series,
	QStringList *filelist, TWTableSpec *table)
{
	solutiondata	s;
	int			n;

	s.prefix = series->name;
	s.prefixlen = n = strlen(series->name);
	if (n > 4 && s.prefix[n - 4] == '.' && tolower(s.prefix[n - 3]) == 'd'
			&& tolower(s.prefix[n - 2]) == 'a'
			&& tolower(s.prefix[n - 1]) == 't')
		s.prefixlen -= 4;

	if (!findfiles(SOLUTIONDIR, &s, getsolutionfile))
		return false;

	if (s.filelist.size() == 0) {
		return false;
	}

	table->setCols(1);
	table->addCell("Select a solution file");
	for (int i = 0; i < s.filelist.size(); i++) {
		table->addCell(s.filelist[i]);
	}

	*filelist = s.filelist;

	return true;
}
