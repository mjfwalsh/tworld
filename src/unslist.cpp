/* unslist.cpp: Functions to manage the list of unsolvable levels.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include    <cstdlib>
#include    <cstring>
#include    <cctype>
#include    <cstdio>
#include    <vector>

#include    "defs.h"
#include    "fileio.h"
#include    "unslist.h"
#include    "err.h"

/* The information comprising one entry in the list of unsolvable
 * levels.
 */
struct unslistentry {
    unslistentry(int setid, int levelnum, int size, uint32_t hashval, int note) :
      setid(setid),
      levelnum(levelnum),
      size(size),
      hashval(hashval),
      note(note)
    {}

    int         setid;      /* the ID of the level set's name */
    int         levelnum;   /* the level's number */
    int         size;       /* the levels data's compressed size */
    uint32_t    hashval;    /* the levels data's hash value */
    int         note;       /* the entry's annotation ID, if any */
};

/* The pool of strings. In here are stored the level set names and the
 * annotations. The string IDs are simple offsets from the strings
 * pointer.
 */
static std::vector<std::string> strings;

/* The list of level set names for which unsolvable levels appear on
 * the list. This list allows the program to quickly find the level
 * set name's string ID.
 */
static std::vector<int> names;

/* The list of unsolvable levels proper.
 */
static std::vector<unslistentry> unslist;

/*
 * Managing the pool of strings.
 */

/* Turn a string ID back into a string pointer. (Note that a string ID
 * of zero will always return a null string, assuming that
 * storestring() has been called at least once.)
 */
static const std::string &getstring(int id)
{
    return strings[id - 1];
}

/* Make a copy of a string and add it to the string pool. The new
 * string's ID is returned.
 */
static int storestring(const std::string &str)
{
    strings.push_back(str);
    return strings.size();
}

/*
 * Managing the list of set names.
 */

/* Return the string ID of the given set name. If the set name is not
 * already in the list, then if add is TRUE the set name is added to
 * the list; otherwise zero is returned.
 */
static int lookupsetname(const std::string &needle, bool add)
{
    for (const int &name : names)
        if (getstring(name) == needle)
            return name;
    if (!add)
        return 0;

    int id = storestring(needle);
    names.push_back(id);
    return id;
}

/*
 * Managing the list of unsolvable levels.
 */

/* Add a new entry with the given data to the list.
 */
static void addtounslist(int setid, int levelnum,
    int size, uint32_t hashval, int note)
{
    unslist.emplace_back(setid, levelnum, size, hashval, note);
}

/* Remove all entries for the given level from the list. FALSE is
 * returned if the level was not on the list to begin with.
 */
static bool removefromunslist(int setid, int levelnum)
{
    bool f = false;

    for(auto p = unslist.begin(); p < unslist.end();)
    {
        if (p->setid == setid && p->levelnum == levelnum) {
            unslist.erase(p);
            f = true;
        } else {
            p++;
        }
    }
    return f;
}

/* Add the information in the given file to the list of unsolvable
 * levels. Errors in the file are flagged but do not prevent the
 * function from reading the rest of the file.
 */
static bool readunslist(fileinfo *file)
{
    char        buf[256], token[256];
    char const         *p;
    uint32_t    hashval;
    int         setid;
    unsigned int    size;
    int         lineno, levelnum, n;

    setid = 0;
    for (lineno = 1 ; ; ++lineno) {
        n = sizeof buf - 1;
        if (!file->getline(buf, &n, NULL))
            break;
        for (p = buf ; isspace(*p) ; ++p) ;
        if (!*p || *p == '#')
            continue;
        if (sscanf(p, "[%255[^]]]", token) == 1) {
            setid = lookupsetname(token, true);
            continue;
        }
        n = sscanf(p, "%3d: %04X %08X: %200[^\n\r]",
            &levelnum, &size, &hashval, token);
        if (n > 0 && levelnum > 0 && levelnum < 65536 && setid) {
            if (n == 1) {
                n = sscanf(p, "%*d: %2s", token);
                if (n > 0 && !strcmp(token, "ok")) {
                    removefromunslist(setid, levelnum);
                    continue;
                }
            } else if (n >= 3) {
                addtounslist(setid, levelnum, size, hashval,
                    n == 4 ? storestring(token) : 0);
                continue;
            }
        }
        warn("%s:%d: syntax error", file->name(), lineno);
    }
    return true;
}

/*
 * Exported functions.
 */

/* Look up the levels that constitute the given series and find which
 * levels appear in the list. Those that do will have the unsolvable
 * field in the gamesetup structure initialized.
 */
int markunsolvablelevels(gameseries &series)
{
    int     count = 0;

    for (int j = 0 ; j < series.count ; ++j)
        series.games[j].unsolvable = false;

    int setid = lookupsetname(series.mapfilename, false);
    if (!setid)
        return 0;
    
    for (const auto &unslevel : unslist) {
        if (unslevel.setid != setid)
            continue;
        
        for (int j = 0 ; j < series.count ; ++j) {
            if (series.games[j].number == unslevel.levelnum
                    && series.games[j].levelsize == unslevel.size
                    && series.games[j].levelhash == unslevel.hashval) {
                series.games[j].unsolvable = true;
                series.games[j].unsolvablereason = getstring(unslevel.note);
                ++count;
                break;
            }
        }
    }
    return count;
}

/* Read the list of unsolvable levels from the given filename. If the
 * filename does not contain a path, then the function looks for the
 * file in the resource directory and the user's save directory.
 */
void loadunslistfromfile(char const *filename)
{
    fileinfo file(RESDIR, filename);
    if (!file.open("r", NULL)) {
        warn("%s: Failed to load list of unsolvable levels", filename);
        return;
    }

    readunslist(&file);
    file.close();
}
