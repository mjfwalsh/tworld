/* res.c: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2014 by Brian Raiter and Eric Schmidt, under the GNU
 * General Public License. No warranty. See COPYING for details.
 */

#include	<QObject>
#include	<QSettings>
#include	<QStringList>
#include	<QFile>
#include	<QString>
#include	<QRegularExpression>
#include	<QHash>
#include	<QDebug>

#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	"defs.h"
#include	"fileio.h"
#include	"err.h"
#include	"oshw.h"
#include	"messages.h"
#include	"unslist.h"
#include	"res.h"

/* The directory containing all the resource files.
 */
char		       *resdir = NULL;

// The complete list of rulesets.
QStringList rulesets = {"", "Lynx", "MS"};

// The active ruleset.
QString         currentRuleset = "";

// 2D Array for resources
QHash<QString, QHash<QString, QString> > allResources;

// to pass functions as params
typedef int (*txtloader)(char const * fname);

// res keys
QStringList resKeys = {
"tileimages",
"unsolvablelist",
"endmessages",
"chipdeathsound",
"levelcompletesound",
"chipdeathbytimesound",
"ticksound",
"derezzsound",
"blockedmovesound",
"pickupchipsound",
"pickuptoolsound",
"thiefsound",
"teleportsound",
"opendoorsound",
"socketsound",
"switchsound",
"tileemptiedsound",
"wallcreatedsound",
"trapenteredsound",
"bombsound",
"splashsound",
"blockmovingsound",
"skatingforwardsound",
"skatingturnsound",
"slidingsound",
"slidewalkingsound",
"icewalkingsound",
"waterwalkingsound",
"firewalkingsound"};


/* Iterate through the lines of the rc file, storing the values in the
 * allresources array. Lines consisting only of whitespace, or with an
 * octothorpe as the first non-whitespace character, are skipped over.
 * Lines containing a ruleset in brackets introduce ruleset-specific
 * resource values. Ruleset-independent values are copied into each of
 * the ruleset-specific entries.
 */
bool ReadRCFile()
{
	QString rcFile(resdir);
	rcFile += "/rc";

	QSettings rc(rcFile, QSettings::IniFormat);

	QStringList rs = rc.childGroups();
	for (int i = -1; i < rs.size(); ++i) {
		QString g;
		if(i == -1) {
			g = "all";
		} else {
			rc.beginGroup(rs[i]); // case sensitive
			g = rs[i].toLower();
		}

		QStringList ruleset = rc.childKeys();
		for (int j = 0; j < ruleset.size(); ++j) {
			QString k = ruleset[j].toLower();
			QString v = rc.value(ruleset[j]).toString().toLower(); // case sensitive

			allResources[g][k] = v;
		}
		if(i > -1) rc.endGroup();
	}

    if(allResources.contains("all") && allResources.contains("ms")
    	&& allResources.contains("lynx")) return TRUE;
    else return FALSE;
}

/*
 * Resource-loading functions
 */

QString GetResource(QString resid)
{
	if(allResources[currentRuleset].contains(resid)) {
		return allResources[currentRuleset][resid];
	} else {
		return allResources["all"][resid];
	}
}


QString GetResourcePath(QString resid)
{
	QString rd(resdir);
	QString f = GetResource(resid);

	if(f == "") return "";

	QFile file(rd + "/" + f);
	return file.fileName();
}


/* Attempt to load the tile images.
 */
bool LoadImages()
{
	QString fp = GetResourcePath("tileimages");
	bool f;

	f = loadtileset(fp.toStdString().c_str(), TRUE);

	if (!f) errmsg(fp.toStdString().c_str(), "no valid tilesets found");

	return f;
}


/* Load the list of unsolvable levels.
 */
int LoadTextResource(QString resid, txtloader loadfunc)
{
	return loadfunc(GetResource(resid).toStdString().c_str());
}

/* Load all of the sound resources.
 */
int LoadSounds()
{
    QRegularExpression re("sound$");
    int		count = 0;

    QStringList sounds = resKeys.filter(re);
        for (int i = 0; i < sounds.size(); ++i) {
        QString fp = GetResourcePath(sounds.at(i));

		if(loadsfxfromfile(i, fp.toStdString().c_str())) ++count;
	}

    return count;
}

/* Load all resources that are available. FALSE is returned if the
 * tile images could not be loaded. (Sounds are not required in order
 * to run, and by this point we should already have a valid font and
 * color scheme set.)
 */
int loadgameresources(int ruleset)
{
        currentRuleset = rulesets.at(ruleset).toLower();
        if (!LoadImages()) return FALSE;
        if (LoadSounds() == 0) setaudiosystem(FALSE);
        return TRUE;
}

/* Parse the rc file and load the font and color scheme. FALSE is returned
 * if an error occurs.
 */
int initresources()
{
    if (!ReadRCFile()) return FALSE;
    LoadTextResource("unsolvablelist", loadunslistfromfile);
    LoadTextResource("endmessages", loadmessagesfromfile);
    return TRUE;
}

/* Free all resources.
 */
void freeallresources(void)
{
    int	n;
    freetileset();
    clearunslist();
    for (n = 0 ; n < SND_COUNT ; ++n)
	 freesfx(n);
}
