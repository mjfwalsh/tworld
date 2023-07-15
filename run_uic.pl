#!/usr/bin/env perl

use v5.20;
use autodie;

if(@ARGV != 2) {
	error("Usage: $0 <input file> <output file>");
}

my ($input_file, $output_file) = @ARGV;

open(my $PIPE, '-|', 'uic', $input_file) || error("Uic failed to run");
my $code = join '', <$PIPE>;
close $PIPE;

# Pass the scale attribute directly to the game and objects
# widgets so the initial size corresponds to the users zoom preference.
if($code !~ s|(void setupUi\(.*?)\)|$1, double scale)|) {
	error("Uic replace error 1");
}

if($code !~ s|m_pGameWidget->setMinimumSize\(.*?\);|m_pGameWidget->setFixedSize(scale * DEFAULTTILE * NXTILES, scale * DEFAULTTILE * NYTILES);|) {
	error("Uic replace error 2");
}

if($code !~ s|m_pObjectsWidget->setMinimumSize\(.*?\);|m_pObjectsWidget->setFixedSize(scale * DEFAULTTILE * 4, scale * DEFAULTTILE * 2);|) {
	error("Uic replace error 3");
}

if($code !~ s|m_pMessagesFrame->setMinimumWidth\(.*?\);|m_pMessagesFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);|) {
	error("Uic replace error 4");
}

if($code !~ s|m_pInfoFrame->setMinimumWidth\(.*?\);|m_pInfoFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);|) {
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

# create output dir if necessary
my $output_dir = $output_file;
$output_dir =~ s|/?[^/]+$|| || die;
if(!-d $output_dir) {
	mkdir $output_dir;
}

# add the tile header for macros
open(my $OUT, '>', $output_file) || error("Uic failed to open output file");
print $OUT qq|#include "../src/tile.h"\n|;
print $OUT $code;
close $OUT;

exit 0;

sub error {
	say STDERR @_;
	exit 1;
}
