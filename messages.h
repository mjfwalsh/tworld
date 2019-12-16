/* messages.h: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#ifndef HEADER_messages_h_
#define HEADER_messages_h_

enum
{
    MessageWin,
    MessageDie,
    MessageTime,
    MessageTypeCount
};

int loadmessagesfromfile(char const *filename);
char const *getmessage(int type, char const *alt);

#endif
