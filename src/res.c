/* res.c: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2020 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include	<stdlib.h>
#include	<string.h>
#include	"defs.h"
#include	"err.h"
#include	"messages.h"
#include	"unslist.h"
#include	"res.h"
#include	"tile.h"
#include	"sdlsfx.h"

// the length of the longest res filename
#define FILENAME_LEN 12

// res dir
const char *resPath;
int resPathLen = 0;

// The active ruleset.
int         currentRuleset;

// to pass functions as params
typedef int (*txtloader)(char const * fname);

/* Attempt to load the tile images.
 */
static void LoadImages()
{
    char *fp = (char *)malloc(resPathLen);
    strcpy(fp, resPath);

	if(currentRuleset == Ruleset_Lynx) {
		strcat(fp, "/atiles.bmp");
	} else {
		strcat(fp, "/tiles.bmp");
	}

	if(!loadtileset(fp, TRUE)) {
		die(fp, "no valid tilesets found");
	}
}


/* Load the list of unsolvable levels.
 */
static int LoadTextResource(const char* file, txtloader loadfunc)
{
    char *fp = (char *)malloc(resPathLen);
    strcpy(fp, resPath);
	strcat(fp, "/");
	strcat(fp, file);

	return loadfunc(fp);
}

/* Load all of the sound resources.
 */
static int addSound(int i, const char *file)
{
    char *fp = (char *)malloc(resPathLen);
    strcpy(fp, resPath);
	strcat(fp, "/");
	strcat(fp, file);

    return loadsfxfromfile(i, fp);
}

static int LoadSounds()
{
	int count = 0;

	// all
	count += addSound(SND_CHIP_WINS,       "tada.wav");
	count += addSound(SND_ITEM_COLLECTED,  "ting.wav");
	count += addSound(SND_BOOTS_STOLEN,    "thief.wav");
	count += addSound(SND_TELEPORTING,     "teleport.wav");
	count += addSound(SND_DOOR_OPENED,     "door.wav");
	count += addSound(SND_BUTTON_PUSHED,   "click.wav");
	count += addSound(SND_BOMB_EXPLODES,   "bomb.wav");
	count += addSound(SND_WATER_SPLASH,    "splash.wav");

	// ms only
	if(currentRuleset == Ruleset_MS) {
		count += addSound(SND_TIME_OUT,        "ding.wav");
		count += addSound(SND_TIME_LOW,        "tick.wav");
		count += addSound(SND_CHIP_LOSES,      "death.wav");
		count += addSound(SND_CANT_MOVE,       "oof.wav");
		count += addSound(SND_IC_COLLECTED,    "chack.wav");
		count += addSound(SND_SOCKET_OPENED,   "socket.wav");
	}

	// lynx only
	if(currentRuleset == Ruleset_Lynx) {
		count += addSound(SND_CHIP_LOSES,      "derezz.wav");
		count += addSound(SND_CANT_MOVE,       "bump.wav");
		count += addSound(SND_IC_COLLECTED,    "ting.wav");
		count += addSound(SND_SOCKET_OPENED,   "door.wav");
		count += addSound(SND_TILE_EMPTIED,    "whisk.wav");
		count += addSound(SND_WALL_CREATED,    "popup.wav");
		count += addSound(SND_TRAP_ENTERED,    "bump.wav");
		count += addSound(SND_BLOCK_MOVING,    "block.wav");
		count += addSound(SND_SKATING_FORWARD, "skate.wav");
		count += addSound(SND_SKATING_TURN,    "skaturn.wav");
		count += addSound(SND_SLIDING,         "force.wav");
		count += addSound(SND_SLIDEWALKING,    "slurp.wav");
		count += addSound(SND_ICEWALKING,      "snick.wav");
		count += addSound(SND_WATERWALKING,    "plip.wav");
		count += addSound(SND_FIREWALKING,     "crackle.wav");
	}

	return count;
}

// set resPath and resPathLen
void initVars()
{
	if(resPathLen == 0) {
		resPath = getdir(RESDIR);
		resPathLen = strlen(resPath) + FILENAME_LEN;
		resPathLen *= sizeof(char *);
		resPathLen++;
	}
}

/* Load all resources that are available. FALSE is returned if the
 * tile images could not be loaded. (Sounds are not required in order
 * to run, and by this point we should already have a valid font and
 * color scheme set.)
 */
int loadgameresources(int ruleset)
{
	initVars();

	currentRuleset = ruleset;
	LoadImages();
	if (LoadSounds() == 0) setaudiosystem(FALSE);
	return TRUE;
}

/* Parse the rc file and load the font and color scheme. FALSE is returned
 * if an error occurs.
 */
int initresources()
{
	initVars();

    return LoadTextResource("unslist.txt", loadunslistfromfile) &&
    LoadTextResource("messages.txt", loadmessagesfromfile);
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
