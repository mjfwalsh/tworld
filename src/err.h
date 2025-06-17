/* err.h: Error handling and reporting.
 *
 * Copyright (C) 2001-2010 by Brian Raiter and Madhav Shanbhag,
 * under the GNU General Public License. No warranty. See COPYING for details.
 */

#ifndef HEADER_err_h_
#define HEADER_err_h_

/* Log an error message and continue.
 */
extern void warn_(char const *fmt, ...);

/* Display an error message and abort.
 */
extern void die_(char const *fmt, ...) __attribute__((noreturn));

/* A really ugly hack used to smuggle extra arguments into variadic
 * functions.
 */
#define warn(fmt, ...) warn_("error: " fmt " [%s:%lu]\n", ##__VA_ARGS__, __FILE__, __LINE__)
#define  die(fmt, ...)  die_("FATAL: " fmt " [%s:%lu]\n", ##__VA_ARGS__, __FILE__, __LINE__)

/* Simple functions for dealing with memory allocation simply.
 */

void inline memerrexit()
{
    die("out of memory");
}

template <typename T>
inline void safe_realloc(T** p, size_t s)
{
    T *tmp = (T *)realloc(*p, s);
    if(tmp == nullptr) memerrexit();
    *p = tmp;
}

#endif
