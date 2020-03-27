/* messages.h: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_messages_h_
#define HEADER_messages_h_

class QString;

enum
{
	MessageWin,
	MessageDie,
	MessageTime,
	MessageTypeCount
};

extern void loadmessagesfromfile(char const *filename);

extern QString getmessage(int type, QString alt);


#endif
