#!/usr/bin/perl -w

# Copyright (C) 2019 by Michael J. Walsh. Licenced under the GNU GPL, v 3. See COPYING.

# pragmas and modules
use v5.10;
use strict;
use File::stat;
use POSIX qw(strftime);

# global vars
my $cmd;
my @qt_modules = qw|QtCore QtGui QtXml QtWidgets|;
my @c_base = qw|cc -std=gnu11|;
my @cpp_base = qw|c++ -std=gnu++11|;
my @common_params = qw|-Wall -pedantic -DNDEBUG -O2 -I. -Dstricmp=strcasecmp  -Werror|;
my @qt_opts;
my @sdl_opts;
my $need_updated_time_stamp = 1;
my $time_stamp_file = 'src/help.c';
my $sdl_config = 'sdl2-config';
my $qmake = 'qmake';

if($#ARGV == -1) {
	$cmd = 'compile';
} elsif($#ARGV == 0 && length $ARGV[0] > 0) {
	$cmd = $ARGV[0];
} else {
	usage();
}

if($cmd eq "install") {
	system 'mv', 'Tile World.app', '/Applications/';
} elsif($cmd eq "compile") {
	compile();
	linker();
} elsif($cmd eq "mkapp") {
	mkapp();
} elsif($cmd eq "clean") {
	clean();
} else {
	usage();
}


sub run {
	say join ' ', @_;
	system @_;
	return $?;
}


sub compile_file {
	my $input_file = shift;
	my $force = shift;

	my @args;
	my $action;

	my $output_file = $input_file;
	$output_file =~ s|^src/|obj/|;

	my @check_files = ($input_file);

	if($output_file =~ s/\.(c|cpp)$/.o/) {
		$action = "Compiling $output_file...";

		# basis compiler params
		if($1 eq 'c') {
			push @args, @c_base;
		} else {
			push @args, @cpp_base;
		}
		push @args, @common_params;

		# filter source code to find dependancies and sdl/qt requirements
		my $r = filter_file($input_file);

		# add dependancy for the generated ui file
		if(defined $r->{'obj/ui_TWMainWnd.h'}) {
			$r->{'src/TWMainWnd.ui'} = 1;
		}

		push @check_files, @{ $r->{'files'} };

		if($r->{'need_sdl'}) {
			push @args, @sdl_opts;
		}
		if($r->{'need_qt'}) {
			push @args, @qt_opts;
		}

		# add time stamp
		if($input_file eq $time_stamp_file) {
			my $time_stamp = strftime('%d %B %Y', localtime);
			$time_stamp =~ s/^0+//;

			push @args, qq|-DCOMPILE_TIME="$time_stamp"|;
			$need_updated_time_stamp = 0;
		}

		push @args, '-c', '-o';

	} elsif($output_file =~ s|([^/]*)\.h$|moc_$1.cpp|) {
		$action = "MOC-ing $input_file...";
		push @args, 'moc', '-o';
	} elsif($output_file =~ s|([^/]*)\.ui$|ui_$1.h|) {
		$action = "Compiling UI $input_file...";
		push @args, 'uic', '-o';
	} else {
		die "Failed to understand input file $input_file";
	}

	if(!$force && -e $output_file) {
		my @changed_files = run_time_check($output_file, @check_files);

		if($#changed_files == 0) {
			print "$changed_files[0] has changed\n";
		} elsif($#changed_files > 0) {
			my $last = pop @changed_files;
			print join ', ', @changed_files;
			print " and $last have changed\n";
		} else {
			say "Nothing to do for $input_file";
			return;
		}
	}

	# run compiler
	say $action;
	my $res;
	if($args[0] eq 'uic') {
		shift @args;
		$res = run_uic(@args, $output_file, $input_file);
	} else {
		$res = run(@args, $output_file, $input_file);
	}
	if ($res != 0) {
		say 'Failed...';
		exit 1;
	}
}

sub compile {
	# qt opts
	my $qt_install_headers = `$qmake -query QT_INSTALL_HEADERS`;
	chomp $qt_install_headers;
	@qt_opts = ('-I' . $qt_install_headers);
	push @qt_opts, map { "-I$qt_install_headers/$_" } @qt_modules;
	push @qt_opts, '-fPIC';

	# sdl opts
	my $sdl_cflags = `$sdl_config --cflags`;
	chomp $sdl_cflags;
	@sdl_opts = split(/ /, $sdl_cflags);

	if(!-d 'obj') { mkdir 'obj'; }

	# compile
	my @files_to_compile = qw|src/tworld.c src/series.c src/play.c src/encoding.c src/solution.c
	src/res.cpp src/lxlogic.c src/mslogic.c src/unslist.c src/messages.cpp src/help.c
	src/score.cpp src/random.c src/settings.cpp src/fileio.c src/err.c src/generic.c src/tile.c
	src/timer.c src/sdlsfx.c src/oshwbind.cpp src/CCMetaData.cpp src/TWDisplayWidget.cpp
	src/TWProgressBar.cpp src/TWMainWnd.ui src/TWMainWnd.cpp src/TWMainWnd.h
	obj/moc_TWMainWnd.cpp src/TWApp.cpp|;
	foreach my $f (@files_to_compile) {
		compile_file($f, 0);
	}
}

sub linker {
	my @object_files = <obj/*.o>;
	my $executable_name = 'tworld';

	if(-f $executable_name) {
		my @changed_files = run_time_check($executable_name, @object_files);

		if($#changed_files < 0) {
			say "Not linking as no changes have been made";
			return;
		}
	}

	my @link_command = ('c++', '-o', $executable_name, @object_files);

	# add qt's lib dir
	my $qt_install_libs = `$qmake -query QT_INSTALL_LIBS`;
	chomp $qt_install_libs;
	push @link_command, '-L' . $qt_install_libs;

	# add frameworks or libraries
	if($^O eq 'darwin') {
		my $qt_install_prefix = `$qmake -query QT_INSTALL_PREFIX`;
		chomp $qt_install_prefix;

		push @link_command, '-F' . $qt_install_prefix . '/Frameworks';
		push @link_command, map { ('-framework' , $_)  } @qt_modules;
	} else {
		my @largs = @qt_modules;
		foreach (@largs) {s/Qt/-lQt5/;}

		push @link_command, @largs;
	}

	# sdl options
	my $sdl_libs = `$sdl_config --libs`;
	chomp $sdl_libs;
	push @link_command, split(/ /, $sdl_libs);

	# now that we're linking, compile help for timestamp if we haven't already done so
	if($need_updated_time_stamp) {
		compile_file($time_stamp_file, 1);
	}

	# run link command
	say "Linking $executable_name...";
	my $r = run(@link_command);
	if ( $r == 0 ) {
		say 'Done...';
	} else {
		say 'Failed...';
	}
}

sub clean {
	system 'rm', '-fR', 'Tile World.app';
	unlink 'tworld';
	unlink <obj/*.*>;
}

sub mkapp {
	system 'mkdir', '-p', 'Tile World.app/Contents/MacOS';
	system 'mkdir', '-p', 'Tile World.app/Contents/Resources';
	system 'cp', 'Info.plist', 'Tile World.app/Contents/';
	system 'cp', 'tworld.icns', 'Tile World.app/Contents/Resources/Tile World.icns';
	system 'cp', 'tworld', 'Tile World.app/Contents/MacOS/Tile World';

	system 'cp', '-R', 'res', 'Tile World.app/Contents/Resources/';
	system 'cp', '-R', 'data', 'Tile World.app/Contents/Resources/';
	system 'cp', '-R', 'licences', 'Tile World.app/Contents/Resources/';
}

sub filter_file {
	my $input_file = shift;
	my %files_done;

	my $result = {
	'need_qt' => 0,
	'need_sdl' => 0
	};

	my $need_qt = 0;
	my @files = ($input_file);

	foreach my $file (@files) {
		# Get a kind-of-absolute path with source dir
		# and ignore files outside it.
		next unless $file = process_path($file);

		my $cwd = $file;
		$cwd =~ s|/[^/]*$||;

		next if defined $files_done{$file};

		if($file =~ /\.(c|cpp|h)$/) {
			my $fh;
			if(!open ($fh, '<', $file)) {
				print "Failed on include: $file\n";
				next;
			}
			while(my $l = <$fh>) {
				if($l =~ m/^[\t ]*#include[\t ]+"([^"]+)"/) {
					push @files, "$cwd/$1";
				}
				if($l =~ /^[\t ]*#include[\t ]+<SDL/) {
					$result->{'need_sdl'} = 1;
				}
				if($l =~ /^[\t ]*#include[\t ]+<Q/) {
					$result->{'need_qt'} = 1;
				}
			}
			close $fh;
		} else {
			next unless -f $file;
		}
		$files_done{$file} = 1;
	}

	# avoid adding twice
	delete $files_done{$input_file};

	my @fd = keys %files_done;

	$result->{'files'} = \@fd;

	return $result;
}

sub run_time_check {
	my $output_file = shift;
	my @check_files = @_;

	my @changed_files = ();
	my $output_time = stat($output_file)->mtime;

	foreach my $f (@check_files) {
		my $input_time = stat($f)->mtime;

		if($input_time > $output_time) {
			push @changed_files, $f;
		}
	}

	return @changed_files;
}

sub process_path {
	my $path = shift;

	return '' if $path =~ m|^/|;

	my @f_parts = split m|/+|, $path;
	my @n_parts;

	foreach my $e (@f_parts) {
		if($e eq '.') {
			next;
		} elsif($e eq '..') {
			return '' if $#n_parts < 0;
			pop @n_parts;
		} else {
			push @n_parts, $e;
		}
	}

	return join '/', @n_parts;
}

sub run_uic {
	my $output_file = '';
	my $res = 0;
	my @args;

	while($#_ > 0) { # ignore last element
		my $arg = shift @_;

		if($arg eq '-o') {
			$output_file = shift @_;
			last;
		} else {
			push @args, $arg;
		}
	}
	push @args, @_;

	open(my $PIPE, '-|', 'uic', @args) || return 1;
	my $code = join '', <$PIPE>;
	close $PIPE;

	# Pass the scale attribute directly to the game and objects
	# widgets so the initial size corresponds to the users zoom preference.
	$res += $code =~ s|(void setupUi\(.*?)\)|$1, double scale)|;
	$res += $code =~ s|(m_pGameWidget->setMinimumSize\(.*?)(\);)|$1 * scale$2|;
	$res += $code =~ s|(m_pObjectsWidget->setMinimumSize\(.*?)(\);)|$1 * scale$2|;

	# Counterintuitive, using points makes the app less portable rather than more as
	# operating systems assume a notional dpi instead of a real one (on Mac it's 72,
	# on Windows and Linux it's 96). So switching to pixels ensures fonts will render
	# the same on all three operating systems.
	if($code =~ s|setPointSize|setPixelSize|g) {
		$res++;
	}

	# fix macro include
	$code =~ s| TWMAINWND_H| UI_TWMAINWND_H|g;

	if(length $output_file == 0) {
		print $code;
		return $res == 4 ? 0 : 1;
	} elsif($res == 4) {
		open(my $OUT, '>', $output_file) || return 1;
		print $OUT $code;
		close $OUT;
		return 0;
	} else {
		return 1;
	}
}

sub usage {
	say 'make.pl (compile|clean|mkapp|install)';
	exit;
}
