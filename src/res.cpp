/* res.cpp: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2020 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include	<cstdlib>
#include	<cstring>

#include	"defs.h"
#include	"res.h"
#include	"tile.h"
#include	"fileio.h"
#include	"sdlsfx.h"
#include	"err.h"

// the length of the longest res filename + a bit extra
#define FILENAME_LEN 14

// res dir
static int resPathLen = 0;

//fpstring
static char *fpstring;

/* Attempt to load the tile images.
 */
static void LoadImages(int ruleset)
{
	if(ruleset == Ruleset_Lynx) {
		strcpy(fpstring + resPathLen, "atiles.bmp");
	} else {
		strcpy(fpstring + resPathLen, "tiles.bmp");
	}

	if(!loadtileset(fpstring, true)) {
		die("no valid tilesets found: %s", fpstring);
	}
}

/* Load all of the sound resources.
 */
static int addSound(int i, const char *file)
{
	strcpy(fpstring + resPathLen, file);
	return loadsfxfromfile(i, fpstring);
}

static void LoadSounds(int ruleset)
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
	if(ruleset == Ruleset_MS) {
		count += addSound(SND_TIME_OUT,        "ding.wav");
		count += addSound(SND_TIME_LOW,        "tick.wav");
		count += addSound(SND_CHIP_LOSES,      "death.wav");
		count += addSound(SND_CANT_MOVE,       "oof.wav");
		count += addSound(SND_IC_COLLECTED,    "chack.wav");
		count += addSound(SND_SOCKET_OPENED,   "socket.wav");
	}

	// lynx only
	else {
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

	if (count == 0) setaudiosystem(false);
}


/* Load all resources that are available. The app dies if it can't load any tiles
 * but ignores a failure to find sounds.
 */
void loadgameresources(int ruleset)
{
	const char *resPath = getdir(RESDIR);
	resPathLen = strlen(resPath);

	x_cmalloc(fpstring, resPathLen + FILENAME_LEN);
	memcpy(fpstring, resPath, resPathLen);

	fpstring[resPathLen] = '/';
	resPathLen++;

	LoadImages(ruleset);
	LoadSounds(ruleset);

	free(fpstring);
}
