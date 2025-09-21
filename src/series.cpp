/* series.c: Functions for finding and reading the data files.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include    <QtCore/QDir>
#include    <QtCore/QString>

#include    <algorithm>
#include    <vector>
#include    <cerrno>
#include    <cstdlib>
#include    <cstring>
#include    <cctype>

#include    "defs.h"
#include    "fileio.h"
#include    "solution.h"
#include    "unslist.h"
#include    "series.h"
#include    "TWMainWnd.h"
#include    "utils.h"
#include    "err.h"

/* The signature bytes of the data files.
 */
#define SIG_DATFILE     0xAAAC

#define SIG_DATFILE_MS      0x0002
#define SIG_DATFILE_LYNX    0x0102

/*
 * Reading the data file.
 */

/* Examine the top of a data file and identify its type. FALSE is
 * returned if any header bytes appear to be invalid.
 */
static bool readseriesheader(gameseries &series, fileinfo &file)
{
    // Skip forward when we've already read the header
    if(series.count) {
        file.seek(6);
        return true;
    }

    uint16_t    val16;
    int         ruleset;

    if (!file.readint16(val16, "not a valid data file"))
        return false;
    if (val16 != SIG_DATFILE)
        return fileerr(&file, "not a valid data file");
    if (!file.readint16(val16, "not a valid data file"))
        return false;
    switch (val16) {
        case SIG_DATFILE_MS:    ruleset = Ruleset_MS;       break;
        case SIG_DATFILE_LYNX:  ruleset = Ruleset_Lynx;     break;
        default:
            fileerr(&file, "data file uses an unrecognized ruleset");
            return false;
    }
    if (series.ruleset == Ruleset_None)
        series.ruleset = ruleset;
    if (!file.readint16(val16, "not a valid data file"))
        return false;
    series.count = val16;
    if (!series.count) {
        fileerr(&file, "file contains no maps");
        return false;
    }

    return true;
}

/* Read a single level out of the given data file. The level's name,
 * password, and time limit are extracted from the data.
 */
static bool readleveldata(fileinfo &file, gamesetup &game)
{
    unsigned char          *data;
    unsigned char const        *dataend;
    uint16_t        size;
    int             n;

    if (!file.readint16(size))
        return false;
    data = file.readbuf(size, "missing or invalid level data");
    if (!data)
        return false;
    if (size < 2) {
        fileerr(&file, "invalid level data");
        free(data);
        return false;
    }
    game.levelsize = size;
    game.leveldata = data;
    dataend = game.leveldata + game.levelsize;

    game.number = data[0] | (data[1] << 8);
    if (size < 10)
        goto badlevel;
    game.time = data[2] | (data[3] << 8);
    game.besttime = TIME_NIL;
    game.passwd[0] = '\0';
    data += data[8] | (data[9] << 8);
    data += 10;
    if (data + 2 >= dataend)
        goto badlevel;
    data += data[0] | (data[1] << 8);
    data += 2;
    size = data[0] | (data[1] << 8);
    data += 2;
    if (data + size != dataend)
        warn("level %d: inconsistent size data (%d vs %d)",
            game.number, dataend - data, size);

    while (data + 2 < dataend) {
        size = data[1];
        data += 2;
        if (size > dataend - data)
            size = dataend - data;
        switch (data[-2]) {
        case 1:
            if (size > 1)
                game.time = data[0] | (data[1] << 8);
            break;
        case 3:
            assignmax(game.name, data, size);
            break;
        case 6:
            for (n = 0 ; n < size && n < 4 && data[n] ; ++n)
                game.passwd[n] = data[n] ^ 0x99;
            game.passwd[n] = '\0';
            break;
        case 8:
            warn("level %d: ignoring field 8 password", game.number);
            break;
        }
        data += size;
    }
    if (!game.passwd[0] || strlen(game.passwd) != 4)
        goto badlevel;

    return true;

badlevel:
    free(game.leveldata);
    game.levelsize = 0;
    game.leveldata = nullptr;
    warn("%s: level %d: invalid level data", file.name(), game.number);
    return false;
}

/* Assuming that the series passed in is in fact the original
 * chips.dat file, this function undoes the changes that MS introduced
 * to the original Lynx levels. A rather "ad hack" way to accomplish
 * this, but it permits this fixup to occur without requiring the user
 * to perform a special one-time task. Four passwords are repaired, a
 * possibly missing wall in level 88 is restored, the beartrap wirings
 * of levels 99 and 111 are fixed, the layout changes to 121 and 127
 * are undone, and level 145 is removed.
 */
static bool undomschanges(gameseries &series)
{
    struct { int num, pos, val; } *fixup, fixups[] = {
        {   5,  0x011D,  'P' ^ 0x99 },  {  95,  0x035F,  'W' ^ 0x99 },
        {   9,  0x032D,  'V' ^ 0x99 },  {  95,  0x0360,  'V' ^ 0x99 },
        {   9,  0x032E,  'U' ^ 0x99 },  {  95,  0x0361,  'H' ^ 0x99 },
        {  27,  0x01E7,  'D' ^ 0x99 },  {  95,  0x0362,  'Y' ^ 0x99 },
        {  87,  0x0148,  0x09 },    { 120,  0x0195,  0x00 },
        {  98,  0x0340,  8 },       {  98,  0x0342,  14 },
        {  98,  0x034A,  23 },      {  98,  0x034C,  14 },
        {  98,  0x0354,  8 },       {  98,  0x0356,  16 },
        {  98,  0x035E,  23 },      {  98,  0x0360,  16 },
        {  98,  0x0368,  16 },      {  98,  0x036A,  18 },
        {  98,  0x0372,  6 },       {  98,  0x0374,  20 },
        {  98,  0x037C,  16 },      {  98,  0x037E,  20 },
        {  98,  0x0386,  23 },      {  98,  0x0388,  23 },
        {  98,  0x0390,  23 },      {  98,  0x0392,  25 },
        { 110,  0x02B6,  22 },      { 110,  0x02B8,  11 },
        { 110,  0x02C0,  15 },      { 110,  0x02C2,  6 },
        { 126,  0x00B6,  0x00 },    { 126,  0x00C2,  0x01 },
        { 126,  0x01B6,  0x01 },    { 126,  0x01C2,  0x00 },
        { -1, -1, -1 }
    };

    if (series.count != 149)
        return false;
    for (fixup = fixups ; fixup->num >= 0 ; ++fixup)
        if (series.games[fixup->num].levelsize <= fixup->pos)
            return false;

    series.games.erase(series.games.begin() + 144);
    --series.count;

    for(int n = 144; n < 148; n++)
        series.games[n].number = n+1;

    for (fixup = fixups ; fixup->num >= 0 ; ++fixup)
        series.games[fixup->num].leveldata[fixup->pos] = fixup->val;

    series.games[5].passwd[3] = 'P';
    series.games[9].passwd[0] = 'V';
    series.games[9].passwd[1] = 'U';
    series.games[27].passwd[3] = 'D';
    series.games[95].passwd[0] = 'W';
    series.games[95].passwd[1] = 'V';
    series.games[95].passwd[2] = 'H';
    series.games[95].passwd[3] = 'Y';

    return true;
}

/*
 * Functions to read the data files.
 */

/* Load all levels from the given data file, and all of the user's
 * saved solutions.
 */
bool readseriesfile(gameseries &series)
{
    if (series.gsflags & GSF_ALLMAPSREAD)
        return true;
    if (series.count <= 0) {
        warn("%s: cannot read from empty level set", series.name.c_str());
        return false;
    }

    fileinfo file(series.mapfiledir, series.mapfilename);
    if (!file.open("rb", "unknown error"))
        return false;
    if (!readseriesheader(series, file))
        return false;

    series.games.resize(series.count);

    for (int n = 0; n < series.count && !file.testend();) {
        if (readleveldata(file, series.games[n]))
            ++n;
        else
            --series.count;
    }

    series.games.resize(series.count);

    file.close();
    series.gsflags |= GSF_ALLMAPSREAD;
    if (series.ruleset == Ruleset_Lynx
            && strcasecmp(series.mapfilename.c_str(), "CHIPS.DAT") == 0)
        undomschanges(series);
    markunsolvablelevels(series);
    readsolutions(series);
    g_mainWindow->ReadExtensions(series);
    return true;
}

/* Free all memory allocated for the given gameseries.
 */
void freeseriesdata(gameseries &series)
{
    series.games.clear();
}

/*
 * Functions to locate the series files.
 */

/* Determine whether the file is a map file. If so, add it to the list of
 * available map files. This function is used as a findfiles() callback.
 */
static bool getmapfile(const std::string &filename, int curdir, std::vector<gameseries> &serieslist)
{
    uint32_t magic;

    // return false on io errors but true otherwise
    fileinfo file(curdir, filename);
    if (!file.open("rb", "unknown error")) {
        return false;
    }
    if (!file.readint32(magic, "unexpected EOF")) {
        file.close();
        return false;
    }
    file.rewind();
    if ((magic & 0xFFFF) != SIG_DATFILE) {
        file.close();
        return true;
    }

    // init an (almost) blank gameseries struct
    gameseries &s = serieslist.emplace_back();
    s.mapfilename = filename;
    s.mapfiledir = curdir;
    s.gsflags = 0;
    s.count = 0;
    s.ruleset = Ruleset_None;

    // set the title
    s.name = filename;
    const int namelength = s.name.length();
    if (namelength > 4) {
        char *suffix = &s.name[namelength - 4];
        if (!strcasecmp(suffix, ".dat") || !strcasecmp(suffix, ".ccl"))
            s.name.resize(namelength - 4);
    }

    if (!readseriesheader(s, file)) {
        fileerr(&file, "Failed to understand series header");
        serieslist.pop_back();
        file.close();
        return false;
    }

    file.close();
    return true;
}

/* Run a glob over the dat files in a directory
 */
static void getmapfiles(int dir, std::vector<gameseries> &serieslist)
{
    for (const QString &file : QDir(getdir(dir)).entryList()) {
        if (file[0] != '.') {
            const std::string filename = file.toStdString();
            getmapfile(filename, dir, serieslist);
        }
    }
}

/* A callback function to compare two gameseries structs.
 */
static bool compare_gameseries(gameseries &a, gameseries &b)
{
    return strcasecmp(a.mapfilename.c_str(), b.mapfilename.c_str()) < 0;
}

/* Remove duplicates from a previously sorted gameseries list.
 */
static void removeduplicateserieses(std::vector<gameseries> &game_list)
{
    for (auto n = game_list.begin(); n < game_list.end() - 1; ++n) {
        if(strcasecmp(n->mapfilename.c_str(), (n+1)->mapfilename.c_str()) == 0) {
            game_list.erase(n);
            --n;
        }
    }
}

/* Produce a list of the series that are available for play.
 * Search the series directory and generate an array of gameseries
 * structures corresponding to the data files found there. The array
 * is returned through list, and the size of the array is returned
 * through count. The program will be aborted if a serious error
 * occurs or if no series can be found.
 */
void createserieslist(std::vector<gameseries> &serieslist)
{    
    serieslist.clear();

    getmapfiles(GLOBAL_SERIESDATDIR, serieslist);
    getmapfiles(USER_SERIESDATDIR, serieslist);

    std::sort(serieslist.begin(), serieslist.end(), compare_gameseries);
    removeduplicateserieses(serieslist);
}

/*
 * Miscellaneous functions
 */

/* A function for looking up a specific level in a series by number
 * and/or password.
 */
int findlevelinseries(gameseries const &series, int number, char const *passwd)
{
    int i, n;

    n = -1;
    if (number) {
        for (i = 0; i < series.count; ++i) {
            if (series.games[i].number == number) {
                if (!passwd || !strcmp(series.games[i].passwd, passwd)) {
                    if (n >= 0)
                        return -1;
                    n = i;
                }
            }
        }
    } else if (passwd) {
        for (i = 0; i < series.count; ++i) {
            if (!strcmp(series.games[i].passwd, passwd)) {
                if (n >= 0)
                    return -1;
                n = i;
            }
        }
    } else {
        return -1;
    }
    return n;
}
