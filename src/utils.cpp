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

// std::string::assign(const char*) may copy too many characters
// std::string::assign("\0\0\0\0", 4) sets the size to 4
void assignmax(std::string &dst, const unsigned char *src, int maxchars)
{
    const char *s = (const char *)src;
    dst.assign(s, strnlen(s, maxchars));
}

