/* messages.h: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_messages_h_
#define HEADER_messages_h_

#ifdef __cplusplus
	#include <QString>

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

OSHW_EXTERN void loadmessagesfromfile(char const *filename);

#ifdef __cplusplus
extern QString getmessage(int type, QString alt);
#endif


#undef OSHW_EXTERN

#endif
