/* err.cpp: Error handling and reporting.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include    <cstdlib>
#include    <cstdarg>
#include    <cstdio>

#include    "err.h"

/* Log a warning message.
 */
void warn_(char const *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

/* Display an error message to the user and exit.
 */
void die_(char const *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
    exit(EXIT_FAILURE);
}
