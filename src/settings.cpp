/* settings.cpp: Functions for managing settings.
 *
 * Copyright (C) 2014-2017 by Eric Schmidt, under the GNU General Public
 * License. No warranty. See COPYING for details.
 */

#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QMapIterator>

#include "settings.h"
#include "fileio.h"
#include "err.h"

QMap<QString, QByteArray> settings_string;
QMap<QString, int> settings_int;

char const *sfname = "settings";

void loadsettings()
{
    if(!settings_string.empty() || !settings_int.empty()) {
        warn("Settings already loaded");
        return;
    }

    std::string fname = getpathforfileindir(SETTINGSDIR, sfname);
    QFile infile(fname.c_str());

    if (!infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        warn("Failed to load settings file: %s", fname.c_str());
        return;
    }

    while (!infile.atEnd()) {
        QByteArray line = infile.readLine().trimmed();

        int pos = line.indexOf('=');

        if (pos == -1) continue;

        QString k = line.left(pos);
        QByteArray sv = line.mid(pos+1);

        bool isInt;
        int iv = sv.toInt(&isInt);

        if(isInt)
            settings_int.insert(k, iv);
        else
            settings_string.insert(k, sv);
    }
}

void savesettings()
{
    std::string fname = getpathforfileindir(SETTINGSDIR, sfname);
    QFile outfile(fname.c_str());

    if (!outfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        warn("Failed to save settings file: %s", fname.c_str());
        return;
    }

    QTextStream s(&outfile);

    QMapIterator<QString, QByteArray> i(settings_string);
    while (i.hasNext()) {
        i.next();
        s << i.key() << "=" << i.value() << "\n";
    }

    QMapIterator<QString, int> j(settings_int);
    while (j.hasNext()) {
        j.next();
        s << j.key() << "=" << j.value() << "\n";
    }
}

int getintsetting(char const *name)
{
    return settings_int.value(name, -1);
}

void setintsetting(char const *name, int val)
{
    settings_int.insert(name, val);
}

char const *getstringsetting(char const *name)
{
    if(settings_string.contains(name)) {
        return settings_string[name].data();
    } else {
        return NULL;
    }
}

void setstringsetting(char const *name, char const *val)
{
    settings_string.insert(name, QByteArray(val));
}
