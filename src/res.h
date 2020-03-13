/* res.h: Functions for loading resources from external files.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef	HEADER_res_h_
#define HEADER_res_h_


/* Initialize the resources that are needed at the start of the program
 * FALSE is returned if a resource could not be loaded.
 */
extern void initresources();

/* Load all resources, using the settings for the given ruleset. FALSE
 * is returned if any critical resources could not be loaded.
 */
extern int loadgameresources(int ruleset);

#endif
