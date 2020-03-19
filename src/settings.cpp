/* settings.cpp: Functions for managing settings.
 *
 * Copyright (C) 2014-2017 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "settings.h"
#include "fileio.h"
#include "defs.h"
#include "err.h"

using namespace std;

map<string, string> settings;
char const *sfname = "settings";

void loadsettings()
{
	if(!settings.empty()) {
		warn("Settings already loaded");
		return;
	}

	char *fpath = getpathforfileindir(SETTINGSDIR, sfname);
	ifstream in(fpath);
	free(fpath);

	if (!in)
		return;

	string line;
	while (getline(in, line)) {
		size_t pos(line.find('='));
		if (pos != string::npos)
		settings.insert({line.substr(0, pos), line.substr(pos+1)});
	}

	if (!in.eof())
		warn("Error reading settings file");
}

void savesettings()
{
	char *fpath = getpathforfileindir(SETTINGSDIR, sfname);
	ofstream out(fpath);
	free(fpath);

	if (!out) {
		warn("Could not open settings file");
		return;
	}
	for (map<string,string>::const_iterator i(settings.begin());
			i != settings.end(); ++i) {
		out << i->first << '=' << i->second << '\n';
	}

	if (!out) {
		warn("Could not write settings");
	}
}

int getintsetting(char const * name)
{
	map<string, string>::const_iterator loc(settings.find(name));
	if (loc == settings.end())
		return -1;
	istringstream in(loc->second);
	int i;
	if (!(in >> i))
		return -1;
	return i;
}

void setintsetting(char const * name, int val)
{
	ostringstream out;
	out << val;
	settings[name] = out.str();
}

char const * getstringsetting(char const * name)
{
	map<string, string>::const_iterator loc(settings.find(name));
	if (loc == settings.end())
		return nullptr;
	return loc->second.c_str();
}

void setstringsetting(char const * name, char const * val)
{
	settings[name] = val;
}
