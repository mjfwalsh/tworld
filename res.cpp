/* res.c: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2014 by Brian Raiter and Eric Schmidt, under the GNU
 * General Public License. No warranty. See COPYING for details.
 */

#include	<QFile>
#include	<QString>
#include	<QHash>
#include	<stdlib.h>
#include	"defs.h"
#include	"err.h"
#include	"messages.h"
#include	"unslist.h"
#include	"res.h"
#include	"tile.h"
#include	"sdlsfx.h"

/* The directory containing all the resource files.
 */
char		 *resdir = NULL;

// The active ruleset.
int         currentRuleset;

// 2D Array for resources
QHash<int, QHash<QString, QString>> allResources;

// sounds
QString sounds[SND_COUNT];

// to pass functions as params
typedef int (*txtloader)(char const * fname);

void addResource(int soundIndex, QString key, QString all, QString ms, QString lynx)
{
	key = key.toLower();

	if(!all.isEmpty()) allResources[Ruleset_None][key] = all;
	if(!ms.isEmpty()) allResources[Ruleset_MS][key] = ms;
	if(!lynx.isEmpty()) allResources[Ruleset_Lynx][key] = lynx;

	if(soundIndex > -1) sounds[soundIndex] = key;
}

void initres()
{
	addResource(-1,                  "TileImages",           "",               "tiles.bmp",   "atiles.bmp");
	addResource(-1,                  "UnsolvableList",       "unslist.txt",    "",            "");
	addResource(-1,                  "EndMessages",          "messages.txt",   "",            "");
	addResource(SND_CHIP_LOSES,      "ChipDeathSound",       "",               "death.wav",   "derezz.wav");
	addResource(SND_CHIP_WINS,       "LevelCompleteSound",   "tada.wav",       "",            "");
	addResource(SND_TIME_OUT,        "ChipDeathByTimeSound", "",               "ding.wav",    "");
	addResource(SND_TIME_LOW,        "TickSound",            "",               "tick.wav",    "");
	addResource(SND_CANT_MOVE,       "BlockedMoveSound",     "",               "oof.wav",     "bump.wav");
	addResource(SND_IC_COLLECTED,    "PickupChipSound",      "",               "chack.wav",   "ting.wav");
	addResource(SND_ITEM_COLLECTED,  "PickupToolSound",      "ting.wav",       "",            "");
	addResource(SND_BOOTS_STOLEN,    "ThiefSound",           "thief.wav",      "",            "");
	addResource(SND_TELEPORTING,     "TeleportSound",        "teleport.wav",   "",            "");
	addResource(SND_DOOR_OPENED,     "OpenDoorSound",        "door.wav",       "",            "");
	addResource(SND_SOCKET_OPENED,   "SocketSound",          "",               "socket.wav",  "door.wav");
	addResource(SND_BUTTON_PUSHED,   "SwitchSound",          "click.wav",      "",            "");
	addResource(SND_TILE_EMPTIED,    "TileEmptiedSound",     "",               "",            "whisk.wav");
	addResource(SND_WALL_CREATED,    "WallCreatedSound",     "",               "",            "popup.wav");
	addResource(SND_TRAP_ENTERED,    "TrapEnteredSound",     "",               "",            "bump.wav");
	addResource(SND_BOMB_EXPLODES,   "BombSound",            "bomb.wav",       "",            "");
	addResource(SND_WATER_SPLASH,    "SplashSound",          "splash.wav",     "",            "");
	addResource(SND_BLOCK_MOVING,    "BlockMovingSound",     "",               "",            "block.wav");
	addResource(SND_SKATING_FORWARD, "SkatingForwardSound",  "",               "",            "skate.wav");
	addResource(SND_SKATING_TURN,    "SkatingTurnSound",     "",               "",            "skaturn.wav");
	addResource(SND_SLIDING,         "SlidingSound",         "",               "",            "force.wav");
	addResource(SND_SLIDEWALKING,    "SlideWalkingSound",    "",               "",            "slurp.wav");
	addResource(SND_ICEWALKING,      "IceWalkingSound",      "",               "",            "snick.wav");
	addResource(SND_WATERWALKING,    "WaterWalkingSound",    "",               "",            "plip.wav");
	addResource(SND_FIREWALKING,     "FireWalkingSound",     "",               "",            "crackle.wav");
}

/*
 * Resource-loading functions
 */

QString GetResource(QString resid)
{
	if(allResources[currentRuleset].contains(resid)) {
		return allResources[currentRuleset][resid];
	} else {
		return allResources[Ruleset_None][resid];
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

	f = loadtileset(fp.toUtf8().constData(), TRUE);

	if (!f) errmsg(fp.toUtf8().constData(), "no valid tilesets found");

	return f;
}


/* Load the list of unsolvable levels.
 */
int LoadTextResource(QString resid, txtloader loadfunc)
{
	return loadfunc(GetResource(resid).toUtf8().constData());
}

/* Load all of the sound resources.
 */
int LoadSounds()
{
    int	count = 0;
    for(int i = 0; i < SND_COUNT; i++) {
    	QString file = GetResourcePath(sounds[i]);
    	if(loadsfxfromfile(i, file.toUtf8().constData())) ++count;
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
	    currentRuleset = ruleset;
        if (!LoadImages()) return FALSE;
        if (LoadSounds() == 0) setaudiosystem(FALSE);
        return TRUE;
}

/* Parse the rc file and load the font and color scheme. FALSE is returned
 * if an error occurs.
 */
int initresources()
{
    initres();
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
