#!/usr/bin/perl -w

# Copyright (C) 2019 by Michael J. Walsh. Licenced under the GNU GPL, v 3.

# This script modifies the output of the uic tool.

# It allows passing the scale attribute directly to the game and objects
# widgets so the initial size corresponds to the users zoom preference.

# This a bit hacky but avoids having to abandon using the .ui file and uic tool
# just to make a small change.

use strict;

my $output_file = '';
my @args;

while($#ARGV > 0) { # ignore last element
	my $arg = shift @ARGV;

	if($arg eq '-o') {
		$output_file = shift @ARGV;
		last;
	} else {
		push @args, $arg;
	}
}

push @args, @ARGV;

open(my $PIPE, '-|', 'uic', @args) || exit 1;
my $code = join '', <$PIPE>;
close $PIPE;

my $res = 0;
$res += $code =~ s|(void setupUi\(.*?)\)|$1, double scale)|;
$res += $code =~ s|(m_pGameWidget->setMinimumSize\(.*?)(\);)|$1 * scale$2|;
$res += $code =~ s|(m_pObjectsWidget->setMinimumSize\(.*?)(\);)|$1 * scale$2|;

if(length $output_file == 0) {
	print $code;
	exit $res == 3 ? 0 : 1;
} elsif($res == 3) {
	open(my $OUT, '>', $output_file) || exit 1;
	print $OUT $code;
	close $OUT;
	exit 0;
} else {
	exit 1;
}
