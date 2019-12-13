/* sdlsfx.h: The internal shared definitions for sound effects using SDL.
 * Used in the SDL as well as the Qt OS/hardware layer.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_sdlsfx_h_
#define	HEADER_sdlsfx_h_

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* Activate or deactivate the sound system. The return value is TRUE
 * if the sound system is (or already was) active.
 */
OSHW_EXTERN int setaudiosystem(int active);

/* Load a wave file into memory. index indicates which sound effect to
 * associate the sound with. FALSE is returned if an error occurs.
 */
OSHW_EXTERN int loadsfxfromfile(int index, char const *filename);

/* Specify the sounds effects to be played at this time. sfx is the
 * bitwise-or of any number of sound effects. If a non-continuous
 * sound effect in sfx is already playing, it will be restarted. Any
 * continuous sound effects that are currently playing that are not
 * set in sfx will stop playing.
 */
OSHW_EXTERN void playsoundeffects(unsigned long sfx);

/* Control sound-effect production depending on the value of action.
 * A negative value turns off all sound effects that are playing. A
 * zero value temporarily suspends the playing of sound effects. A
 * positive value continues the sound effects at the point at which
 * they were suspended.
 */
OSHW_EXTERN void setsoundeffects(int action);

/* Set the current volume level. Volume ranges from 0 (silence) to 10
 * (the default). Setting the sound to zero causes sound effects to be
 * displayed as textual onomatopoeia. If display is TRUE, the new
 * volume level will be displayed to the user. FALSE is returned if
 * the sound system is not currently active.
 */
OSHW_EXTERN int setvolume(int volume);

/* Alters the current volume level by delta.
 */
OSHW_EXTERN int changevolume(int delta);

/* Release all memory used for the given sound effect's wave data.
 */
OSHW_EXTERN void freesfx(int index);

/* The initialization function for the sound module.
 */
OSHW_EXTERN int sdlsfxinitialize(int silence, int soundbufsize);

#undef OSHW_EXTERN

#endif
