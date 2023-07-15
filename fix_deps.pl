#!/usr/bin/env perl

use v5.20;
use autodie;

local $/; # enable slurp mode

die if @ARGV != 1;

my $output_file = $ARGV[0];

my $deps = <STDIN>;

# add the dep file as a target
$deps =~ s/^([^:]+):/$1 $output_file:/ || die;

# clean up bad paths
# changes "../obj/ui_TWMainWnd.h" to "obj/ui_TWMainWnd.h"
$deps =~ s![^ ]+(src|obj)/([^ ]+\.h)!$1/$2!g;

# remove obj/comptime.h dep
$deps =~ s|obj/comptime.h||;

# create output dir if necessary
my $output_dir = $output_file;
$output_dir =~ s|/?[^/]+$|| || die;
if(!-d $output_dir) {
	mkdir $output_dir;
}

open(my $out, '>', $output_file);
print $out $deps;
close $out;

exit 0;
