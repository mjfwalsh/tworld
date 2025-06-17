/* utils.h: Some useful utilities
 *
 * Copyright (C) 2020 by Michael Walsh.
 * Licensed under the GNU General Public License.
 * No warranty. See COPYING for details.
 */

#ifndef HEADER_utils_h_
#define HEADER_utils_h_

#include <string>

void stringcopy(char *dest, const char *source, int capacity);
void assignmax(std::string &dst, const unsigned char *src, int maxchars);

#endif
