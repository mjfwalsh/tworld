/* Copyright (C) 2001-2019 by Brian Raiter, Eric Schmidt and Michael Walsh.
 * Licensed under the GNU General Public License. No warranty.
 * See COPYING for details.
 */

#ifndef	HEADER_timer_h_
#define	HEADER_timer_h_

/* Control the timer depending on the value of action. A negative
 * value turns off the timer if it is running and resets the counter
 * to zero. A zero value turns off the timer but does not reset the
 * counter. A positive value starts (or resumes) the timer.
 */
extern void settimer(int action);

/* Set the length (in real time) of a second of game time. A value of
 * zero selects the default of 1000 milliseconds.
 */
extern void settimersecond(int ms);

/* Return the number of ticks since the timer was last reset.
 */
extern int gettickcount(void);

/* Put the program to sleep until the next timer tick.
 */
extern int waitfortick(void);

/* Force the timer to advance to the next tick.
 */
extern int advancetick(void);

/* Initialisation function
 */

extern bool timerinitialize();

#endif
