#!/usr/bin/env perl

use v5.20;
use autodie;

local $/; # enable slurp mode

my $code = <STDIN>;

# Pass the scale attribute directly to the game and objects
# widgets so the initial size corresponds to the users zoom preference.
if($code !~ s|(void setupUi\(.*?)\)|$1, double scale)|) {
    error("Uic replace error 1");
}

if($code !~ s|m_gameWidget->setMinimumSize\(.*?\);|m_gameWidget->setFixedSize(scale * DEFAULTTILE * NXTILES, scale * DEFAULTTILE * NYTILES);|) {
    error("Uic replace error 2");
}

if($code !~ s|m_objectsWidget->setMinimumSize\(.*?\);|m_objectsWidget->setFixedSize(scale * DEFAULTTILE * 4, scale * DEFAULTTILE * 2);|) {
    error("Uic replace error 3");
}

if($code !~ s|m_messagesFrame->setMinimumWidth\(.*?\);|m_messagesFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);|) {
    error("Uic replace error 4");
}

if($code !~ s|m_infoFrame->setMinimumWidth\(.*?\);|m_infoFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);|) {
    error("Uic replace error 5");
}

# Counterintuitively, using points makes the app less portable rather than more as
# operating systems assume a notional dpi instead of a real one (on Mac it's 72,
# on Windows and Linux it's 96). So switching to pixels ensures fonts will render
# the same on all three operating systems.
if($code !~ s|setPointSize|setPixelSize|g) {
    error("Uic replace error 6");
}

# fix macro include
if($code !~ s| TWMAINWND_H| UI_TWMAINWND_H|g) {
    error("Uic replace error 7");
}

# add the tile header for macros
print qq|#include "../src/tile.h"\n|;
print $code;

exit 0;

sub error {
    say STDERR @_;
    exit 1;
}
