/* messages.h: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_messages_h_
#define HEADER_messages_h_

#ifdef __cplusplus
	#define OSHW_EXTERN extern "C"
#else
	#define OSHW_EXTERN extern
#endif

enum
{
	MessageWin,
	MessageDie,
	MessageTime,
	MessageTypeCount
};

OSHW_EXTERN int loadmessagesfromfile(char const *filename);
OSHW_EXTERN char const *getmessage(int type, char const *alt);

#undef OSHW_EXTERN

#endif
