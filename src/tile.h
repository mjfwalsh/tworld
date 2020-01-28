/* Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_tile_h_
#define	HEADER_tile_h_

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* Extract the tile images stored in the given file and use them as
 * the current tile set. FALSE is returned if the attempt was
 * unsuccessful. If complain is FALSE, no error messages will be
 * displayed.
 */
OSHW_EXTERN int loadtileset(char const *filename, int complain);

/* Free all memory associated with the current tile images.
 */
OSHW_EXTERN void freetileset(void);

#undef OSHW_EXTERN

#endif
