/* messages.cpp: Functions for end-of-game messages.
 *
 * Copyright (C) 2014 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include "messages.h"

#include "fileio.h"
#include "defs.h"
#include "err.h"

#include <algorithm>
#include <bitset>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

using std::bitset;
using std::ifstream;
using std::find;
using std::getline;
using std::istringstream;
using std::string;
using std::vector;

namespace
{
	vector<string> messages;
	vector<size_t> typeindex[MessageTypeCount];
	size_t current[MessageTypeCount];
}

int const maxMessageSize = 511;
char const * messageTypeNames[MessageTypeCount] = { "win", "die", "time" };

int loadmessagesfromfile(char const *filename)
{
	char *fname = getpathforfileindir(RESDIR, filename);
	ifstream infile(fname);
	free(fname);

	if (!infile)
		return FALSE;

	vector<string> newmessages;
	vector<size_t> newtypeindex[MessageTypeCount];
	bitset<MessageTypeCount> isactive;
	isactive.set(MessageDie);
	string line;
	while (getline(infile, line)) {
		// Just in case DOS line endings on Linux. Not sure if needed.
		string::iterator rpos(find(line.begin(), line.end(), '\r'));
		if (rpos != line.end())
			line.erase(rpos);

		if (line.empty()) continue;
		if (line[0] == ':') {
			isactive.reset();

			istringstream instr(line);
			instr.get(); // Discard ':'
			string type;
			while (instr >> type) {
				int typenum = find(messageTypeNames,
					messageTypeNames + MessageTypeCount, type)
					- messageTypeNames;
				if (typenum < MessageTypeCount)
					isactive.set(typenum);
			}
		} else {
			for (size_t i = 0; i < isactive.size(); ++i) {
				if (isactive[i])
					newtypeindex[i].push_back(newmessages.size());
			}
			line = line.substr(0, maxMessageSize+1);
			newmessages.push_back(line);
		}
	}

	messages.swap(newmessages);
	for (size_t i = 0; i < MessageTypeCount; ++i) {
		typeindex[i].swap(newtypeindex[i]);
		current[i] = 0;
	}

	return TRUE;
}

std::string getmessage(int type, std::string alt)
{
	if (type < 0 || type >= MessageTypeCount || typeindex[type].size() == 0)
		return alt;

	size_t const mnum = typeindex[type][current[type]];

	current[type] = (current[type] + 1) % typeindex[type].size();

	if(messages[mnum].empty()){
		return alt;
	} else {
		return messages[mnum];
	}
}
