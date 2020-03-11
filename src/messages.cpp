/* messages.cpp: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include "messages.h"
#include "fileio.h"
#include "err.h"

#include <QStringList>
#include <QFile>
#include <QBitArray>
#include <QVector>

QStringList messages;
QVector<size_t> typeindex[MessageTypeCount];
size_t current[MessageTypeCount];
#define maxMessageSize 511
QStringList messageTypeNames = { "win", "die", "time" };

void loadmessagesfromfile(char const *filename)
{
	// Dont load twice
	if(!messages.isEmpty()) {
		errmsg(filename, "Attempt to load message files a second time");
		return;
	}

	// get filename and open
	char *fname = getpathforfileindir(RESDIR, filename);
	QFile infile(fname);
	free(fname);
	if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		errmsg(filename, "Failed to load messages file");
		return;
	}

	// default values
	QBitArray isactive(MessageTypeCount);
	isactive.setBit(MessageDie);
	for (size_t i = 0; i < MessageTypeCount; ++i) {
		current[i] = 0;
	}

	while (!infile.atEnd()) {
		QString line = infile.readLine().trimmed(); // removes any extra CRs or LFs

		if (line.isEmpty()) continue;
		if (line.front() == ':') {
			isactive.fill(false);

			line.remove(0, 1); // Discard ':'

			QStringList types = line.split(' ');
			for(int i=0; i<types.size(); i++) {
				int typenum = messageTypeNames.indexOf(types[i]);
				if (typenum > -1)
					isactive.setBit(typenum);
			}
		} else {
			for (size_t i = 0; i < isactive.size(); ++i) {
				if (isactive.testBit(i))
					typeindex[i].push_back(messages.size());
			}
			line.truncate(maxMessageSize);

			messages.push_back(line);
		}
	}
}

QString getmessage(int type, QString alt)
{
	if (type < 0 || type >= MessageTypeCount || typeindex[type].size() == 0)
		return alt;

	size_t const mnum = typeindex[type][current[type]];

	current[type] = (current[type] + 1) % typeindex[type].size();

	return messages[mnum];
}
