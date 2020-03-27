/* utils.cpp: Some useful utilities
 *
 * Copyright (C) 2020 by Michael Walsh.
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#include <cstring>

#include "utils.h"

void stringcopy(char *dest, const char *source, short s)
{
	short l = strlen(source);
	if(l < s) {
		strcpy(dest, source);
	} else {
		s--;
		memcpy(dest, source, s);
		dest[s] = '\0';
	}
}
