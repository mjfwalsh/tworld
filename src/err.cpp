/* err.cpp: Error handling and reporting.
 *
 * Copyright (C) 2001-2006 by Brian Raiter, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include	<stdlib.h>
#include	<stdarg.h>
#include	<stdio.h>

#include	"err.h"

/* "Hidden" arguments to warn_, errmsg_, and die_.
 */
char const      *err_cfile_ = NULL;
unsigned long	err_lineno_ = 0;

/* Values used for the first argument of usermessage().
 */
enum { NOTIFY_DIE, NOTIFY_ERR, NOTIFY_LOG };

/* Display a message to the user. cfile and lineno can be NULL and 0
 * respectively; otherwise, they identify the source code location
 * where this function was called from. prefix is an optional string
 * that is displayed before and/or apart from the body of the message.
 * fmt and args define the formatted text of the message body. action
 * indicates how the message should be presented. NOTIFY_LOG causes
 * the message to be displayed in a way that does not interfere with
 * the program's other activities. NOTIFY_ERR presents the message as
 * an error condition. NOTIFY_DIE should indicate to the user that the
 * program is about to shut down.
 */
static void usermessage(int action, char const *prefix, char const *cfile,
	unsigned long lineno, char const *fmt, va_list args)
{
	fprintf(stderr, "%s: ", action == NOTIFY_DIE ? "FATAL" :
							action == NOTIFY_ERR ? "error" : "warning");
	if (prefix)
		fprintf(stderr, "%s: ", prefix);
	if (fmt)
		vfprintf(stderr, fmt, args);
	if (cfile)
		fprintf(stderr, " [%s:%lu] ", cfile, lineno);
	fputc('\n', stderr);
	fflush(stderr);
}

/* Log a warning message.
 */
void warn_(char const *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	usermessage(NOTIFY_LOG, NULL, err_cfile_, err_lineno_, fmt, args);
	va_end(args);
	err_cfile_ = NULL;
	err_lineno_ = 0;
}

/* Display an error message to the user.
 */
void errmsg_(char const *prefix, char const *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	usermessage(NOTIFY_ERR, prefix, err_cfile_, err_lineno_, fmt, args);
	va_end(args);
	err_cfile_ = NULL;
	err_lineno_ = 0;
}

/* Display an error message to the user and exit.
 */
void die_(char const *fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	usermessage(NOTIFY_DIE, NULL, err_cfile_, err_lineno_, fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}
