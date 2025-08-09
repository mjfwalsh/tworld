/* score.cpp: Calculating scores and formatting the display of same.
 *
 * Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#include    <QtCore/QString>
#include    <QtCore/QLocale>
#include    <QtCore/QTextStream>

#include    "TWApp.h"
#include    "TWTableSpec.h"
#include    "defs.h"
#include    "play.h"
#include    "score.h"
#include    "err.h"

QLocale locale;

/* Return the user's scores for a given level.
 */
bool getscoresforlevel(gameseries const *series, int level,
    int *base, int *bonus, long *total)
{
    long totalscore = 0L;
    int n = 0;

    *base = 0;
    *bonus = 0;

    for (const gamesetup &game : series->games) {
        int levelscore = 0;
        int timescore = 0;
        if (hassolution(&game)) {
            levelscore = game.number * 500;
            if (game.time)
                timescore = 10 * (game.time
                    - game.besttime / TICKS_PER_SECOND);
        }
        if (n == level) {
            *base = levelscore;
            *bonus = timescore;
        }
        totalscore += levelscore + timescore;
        n++;
    }
    *total = totalscore;
    return true;
}

/* Produce a table that displays the user's score, broken down by
 * levels with a grand total at the end. If usepasswds is FALSE, all
 * levels are displayed. Otherwise, levels after the last level for
 * which the user knows the password are left out. Other levels for
 * which the user doesn't know the password are in the table, but
 * without any information besides the level's number.
 */
void createscorelist(const gameseries *series, bool usepasswds, std::vector<int> &levellist,
    TWTableSpec *table)
{
    qlonglong   totalscore;

    levellist.reserve(series->games.size() + 2);

    totalscore = 0;

    table->setCols(6);

    table->addCell("Level", RightAlign);
    table->addCell("Name",  LeftAlign);
    table->addCell("Base",  RightAlign);
    table->addCell("Best Time", RightAlign);
    table->addCell("Time Bonus", RightAlign);
    table->addCell("Score", RightAlign);

    int j = 0;
    int blankLines = 0;
    for (const gamesetup &game : series->games) {
        table->addCell(locale.toString(game.number), RightAlign);

        if (hassolution(&game)) {
            table->addCell(game.name.c_str());

            if (game.sgflags & SGF_REPLACEABLE) {
                table->addCell("", LeftAlign, 4);
            } else {
                int levelscore = 500 * game.number;
                int timescore = 0;

                table->addCell(locale.toString(levelscore), RightAlign);

                if (game.time) {
                    int bestime = game.time - game.besttime / TICKS_PER_SECOND;
                    table->addCell(locale.toString(bestime), RightAlign);

                    timescore = 10 * bestime;
                    table->addCell(locale.toString(timescore), RightAlign);
                } else {
                    table->addCell("", RightAlign, 2);
                }
                table->addCell(locale.toString(levelscore + timescore), RightAlign);
                totalscore += levelscore + timescore;
            }
            levellist.push_back(j);
            blankLines = 0;
        } else {
            if (!usepasswds || (game.sgflags & SGF_HASPASSWD)) {
                table->addCell(game.name.c_str(), LeftAlign, 5);

                levellist.push_back(j);
                blankLines = 0;
            } else {
                table->addCell("", LeftAlign, 5); // blank line
                blankLines++;
                levellist.push_back(-1);
            }
        }
        j++;
    }

    // trim empty rows
    table->trimRows(blankLines);

    // add trailing row
    table->addCell("Total Score", RightAlign, 2);
    table->addCell(locale.toString(totalscore), RightAlign, 4);

    levellist.push_back(-1);
}

/* Free the memory allocated by createscorelist() or createtimelist().
 */
void freescorelist(int *levellist)
{
    free(levellist);
}

QString timestring(int lvlnum, const QString &lvltitle, int besttime, int timed, int bad)
{
    QString result;

    QTextStream oss(&result);
    oss.setLocale(locale);

    oss << '#' << lvlnum << " (" << lvltitle << "): ";
    if (!timed) oss << '[';
    oss << besttime;
    if (!timed) oss << ']';
    if (bad) oss << " *bad*";
    oss << '\n';

    return result;
}

void copyleveltimestoclipboard(gameseries const *series)
{
    QString result;

    QTextStream oss(&result);
    oss.setLocale(locale);

    for (const gamesetup &game : series->games) {
        if (hassolution(&game)) {
            int const starttime = (game.time ? game.time : 999);
            int const besttime = starttime - game.besttime / TICKS_PER_SECOND;
            bool const timed = (game.time > 0);
            bool const bad = (game.sgflags & SGF_REPLACEABLE);
            oss << timestring(game.number, game.name.c_str(), besttime, timed, bad);
        }
    }

    TileWorldApp::CopyToClipboard(result);
}
