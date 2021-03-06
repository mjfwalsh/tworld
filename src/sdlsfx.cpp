/* sdlsfx.cpp: Creating the program's sound effects.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Madhav Shanbhag and Michael Walsh.
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#include	<SDL.h>

#include	<cstdlib>
#include	<cstring>

#include	"sdlsfx.h"
#include	"settings.h"
#include	"defs.h"
#include	"err.h"

/* Some generic default settings for the audio output.
 */
#define DEFAULT_SND_FMT		AUDIO_S16SYS
#define DEFAULT_SND_FREQ	22050
#define	DEFAULT_SND_CHAN	1

/* The data needed for each sound effect's wave.
 */
typedef	struct sfxinfo {
	Uint8	    *wave;		/* the actual wave data */
	Uint32		len;		/* size of the wave data */
	int			pos;		/* how much has been played already */
	bool		playing;	/* is the wave currently playing? */
} sfxinfo;

/* The data needed to talk to the sound output device.
 */
static SDL_AudioSpec	spec;

/* All of the sound effects.
 */
static sfxinfo		sounds[SND_COUNT];

/* TRUE if the program is currently talking to a sound device.
 */
static bool		hasaudio = false;

/* The volume level.
 */
static int		volume = SDL_MIX_MAXVOLUME;

/* The sound buffer size scaling factor.
 */
static const int		soundbufsize = 0;

/* Release all memory for the given sound effect.
 */
static void freesfx(int index)
{
	if (sounds[index].wave) {
		SDL_LockAudio();
		free(sounds[index].wave);
		sounds[index].wave = NULL;
		sounds[index].pos = 0;
		sounds[index].playing = false;
		SDL_UnlockAudio();
	}
}

/* The callback function that is called by the sound driver to supply
 * the latest sound effects. All the sound effects are checked, and
 * the ones that are being played get another chunk of their sound
 * data mixed into the output buffer. When the end of a sound effect's
 * wave data is reached, the one-shot sounds are changed to be marked
 * as not playing, and the continuous sounds are looped.
 */
static void sfxcallback(void *data, Uint8 *wave, int len)
{
	int	i, n;

	(void)data;
	memset(wave, spec.silence, len);
	for (i = 0 ; i < SND_COUNT ; ++i) {
		if (!sounds[i].wave)
			continue;
		if (!sounds[i].playing)
			if (!sounds[i].pos || i >= SND_ONESHOT_COUNT)
				continue;
		n = sounds[i].len - sounds[i].pos;
		if (n > len) {
			SDL_MixAudio(wave, sounds[i].wave + sounds[i].pos, len, volume);
			sounds[i].pos += len;
		} else {
			SDL_MixAudio(wave, sounds[i].wave + sounds[i].pos, n, volume);
			sounds[i].pos = 0;
			if (i < SND_ONESHOT_COUNT) {
				sounds[i].playing = false;
			} else if (sounds[i].playing) {
				while (len - n >= (int)sounds[i].len) {
					SDL_MixAudio(wave + n, sounds[i].wave, sounds[i].len, volume);
					n += sounds[i].len;
				}
				sounds[i].pos = len - n;
				SDL_MixAudio(wave + n, sounds[i].wave, sounds[i].pos, volume);
			}
		}
	}
}

/*
 * The exported functions.
 */

/* Activate or deactivate the sound system. When activating for the
 * first time, the connection to the sound device is established. When
 * deactivating, the connection is closed.
 */
bool setaudiosystem(bool active)
{
	SDL_AudioSpec	des;
	int			n;

	if (!active) {
		if (hasaudio) {
			SDL_PauseAudio(true);
			SDL_CloseAudio();
			hasaudio = false;
		}
		return true;
	}

	if (!SDL_WasInit(SDL_INIT_AUDIO)) {
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
			warn("Cannot initialize audio output: %s", SDL_GetError());
			return false;
		}
	}

	if (hasaudio)
		return true;

	des.freq = DEFAULT_SND_FREQ;
	des.format = DEFAULT_SND_FMT;
	des.channels = DEFAULT_SND_CHAN;
	des.callback = sfxcallback;
	des.userdata = NULL;
	for (n = 1 ; n <= des.freq / TICKS_PER_SECOND ; n <<= 1) ;
	des.samples = (n << soundbufsize) >> 2;
	if (SDL_OpenAudio(&des, &spec) < 0) {
		warn("can't access audio output: %s", SDL_GetError());
		return false;
	}
	hasaudio = true;
	SDL_PauseAudio(false);

	return true;
}

/* Load a single wave file into memory. The wave data is converted to
 * the format expected by the sound device.
 */
bool loadsfxfromfile(int index, char const *filename)
{
	SDL_AudioSpec	specin;
	SDL_AudioCVT	convert;
	Uint8	       *wavein;
	Uint8	       *wavecvt;
	Uint32		lengthin;

	if (!filename || filename[0] == '\0') {
		freesfx(index);
		return true;
	}

	if (!hasaudio)
		if (!setaudiosystem(true))
			return false;

	if (!SDL_LoadWAV(filename, &specin, &wavein, &lengthin)) {
		freesfx(index);
		warn("can't load %s: %s", filename, SDL_GetError());
		return false;
	}

	if (SDL_BuildAudioCVT(&convert,
			specin.format, specin.channels, specin.freq,
			spec.format, spec.channels, spec.freq) < 0) {
		warn("can't create converter for %s: %s", filename, SDL_GetError());
		return false;
	}
	if (!(wavecvt = (Uint8 *)malloc(lengthin * convert.len_mult)))
		memerrexit();
	memcpy(wavecvt, wavein, lengthin);
	SDL_FreeWAV(wavein);
	convert.buf = wavecvt;
	convert.len = lengthin;
	if (SDL_ConvertAudio(&convert) < 0) {
		warn("can't convert %s: %s", filename, SDL_GetError());
		return false;
	}

	freesfx(index);
	SDL_LockAudio();
	sounds[index].wave = convert.buf;
	sounds[index].len = convert.len * convert.len_ratio;
	sounds[index].pos = 0;
	sounds[index].playing = false;
	SDL_UnlockAudio();

	return true;
}

/* Select the sounds effects to be played. sfx is a bitmask of sound
 * effect indexes. Any continuous sounds that are not included in sfx
 * are stopped. One-shot sounds that are included in sfx are
 * restarted.
 */
void playsoundeffects(unsigned long sfx)
{
	unsigned long	flag;
	int			i;

	if (!hasaudio || !volume) {
		return;
	}

	SDL_LockAudio();
	for (i = 0, flag = 1 ; i < SND_COUNT ; ++i, flag <<= 1) {
		if (sfx & flag) {
			sounds[i].playing = true;
			if (i < SND_ONESHOT_COUNT && sounds[i].pos)
				sounds[i].pos = 0;
		} else {
			if (i >= SND_ONESHOT_COUNT)
				sounds[i].playing = false;
		}
	}
	SDL_UnlockAudio();
}

/* If action is negative, stop playing all sounds immediately.
 * Otherwise, just temporarily pause or unpause sound-playing.
 */
void setsoundeffects(int action)
{
	if (!hasaudio || !volume)
		return;

	if (action < 0) {
		SDL_LockAudio();
		for (int i = 0 ; i < SND_COUNT ; ++i) {
			sounds[i].playing = false;
			sounds[i].pos = 0;
		}
		SDL_UnlockAudio();
	} else {
		SDL_PauseAudio(!action);
	}
}

/* Set the current volume level to v.
 */
static int _setvolume(int v)
{
	if (!hasaudio)
		return 0;
	if (v < 0)
		v = 0;
	else if (v > 10)
		v = 10;
	volume = (SDL_MIX_MAXVOLUME * v + 9) / 10;
	return v;
}
int setvolume(int v)
{
	v = _setvolume(v);
	setintsetting("volume", v);
	return v;
}

/* Change the current volume level by delta. Returns the resulting volume level
 */
int changevolume(int delta)
{
	return setvolume(((10 * volume) / SDL_MIX_MAXVOLUME) + delta);
}

/* Shut down the sound system.
 */
static void shutdown(void)
{
	setaudiosystem(false);
	if (SDL_WasInit(SDL_INIT_AUDIO))
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	hasaudio = false;

	for (int n = 0; n < SND_COUNT; ++n)
		freesfx(n);
}

/* Initialize the module.
 */
bool sfxinitialize()
{
	atexit(shutdown);

	// turn on sound system
	setaudiosystem(true);

	// set volume
	_setvolume(getintsetting("volume"));

	return true;
}
