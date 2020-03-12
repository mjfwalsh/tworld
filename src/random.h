/* random.h: The game's random-number generator.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_random_h_
#define	HEADER_random_h_

#include	"defs.h"

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* Mark an existing PRNG as beginning a new sequence.
 */
OSHW_EXTERN void resetprng(prng *gen);

/* Restart an existing PRNG upon a predetermined sequence.
 */
OSHW_EXTERN void restartprng(prng *gen, unsigned long initial);

/* Retrieve the original seed value of the current sequence.
 */
#define	getinitialseed(gen)	((gen)->initial)

/* Return a random integer between zero and three, inclusive.
 */
OSHW_EXTERN int random4(prng *gen);

/* Randomly permute an array of three integers.
 */
OSHW_EXTERN void randomp3(prng *gen, int *array);

/* Randomly permute an array of four integers.
 */
OSHW_EXTERN void randomp4(prng *gen, int *array);

#endif
