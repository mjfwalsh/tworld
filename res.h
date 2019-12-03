/* res.h: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_res_h_
#define HEADER_res_h_

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

/* The directory containing all the resource files.
 */
//OSHW_EXTERN char	       *resdir;

/* Parse the rc file and initialize the resources that are needed at
 * the start of the program (i.e., the font and color settings).
 * FALSE is returned if the rc file contained errors or if a resource
 * could not be loaded.
 */
OSHW_EXTERN int initresources();

/* Load all resources, using the settings for the given ruleset. FALSE
 * is returned if any critical resources could not be loaded.
 */
OSHW_EXTERN int loadgameresources(int ruleset);

/* Release all memory allocated for the resources.
 */
OSHW_EXTERN void freeallresources();

#undef OSHW_EXTERN

#endif
