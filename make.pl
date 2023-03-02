#!/usr/bin/perl -w

# Copyright (C) 2020 by Michael J. Walsh. Licenced under the GNU GPL, v 3. See COPYING.

# pragmas and modules
use v5.10;
use strict;
use File::stat;
use POSIX qw(strftime);

BEGIN {
	if($^O eq 'MSWin32') { # we have cp and which on unix
		require File::Which;
		require File::Copy::Recursive;
		require File::Path;
	}
}

# what we're doing
my $cmd;
my $need_updated_time_stamp = 1;

# windows vars
my $executable_name = $^O eq 'MSWin32' ? 'tworld.exe' : 'tworld';

# on mac add unlinked qt Homebrew keg to path
if($^O eq 'darwin') {
	$ENV{PATH} = '/usr/local/opt/qt/bin:' . $ENV{PATH};
}

# compiler options
my @qt_modules = qw|QtCore QtGui QtXml QtWidgets|;
my @params = qw|c++ -std=gnu++17 -Wall -pedantic -DNDEBUG -O2 -I. -Werror|;

# set these later if we need them
my @qt_opts;
my @sdl_opts;

# some command line tools
my $sdl_config;
my $qmake = 'qmake';
my $uic = 'uic';
my $moc = 'moc';

# there's no sdl2-config in windows
my $sdl_include_path;
my $sdl_lib_path;

# now process command
if($#ARGV == -1) {
	$cmd = 'compile';
} elsif($#ARGV == 0 && length $ARGV[0] > 0) {
	$cmd = $ARGV[0];
} else {
	usage();
}

if($cmd eq "install") {
	install();
} elsif($cmd eq "compile") {
	check_path();
	compile();
	linker();
} elsif($cmd eq "mkdist") {
	mkdist();
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

sub run_capture {
   say join ' ', @_;
   open my $fh, '-|', @_;
   my $out = join '', <$fh>;
   close $fh;
   return $out;
}



sub compile_file {
	my $input_file = shift;
	my $force = shift;

	my @args;
	my @check_files;

	my $output_file = $input_file;
	$output_file =~ s|^src/|obj/| || $output_file =~ s|^([^/]*)$|obj/$1|;

	if($output_file =~ s/\.cpp$/.o/) {
        my $dep_file = $output_file;
        $dep_file =~ s|^obj/|deps/| || die $output_file;
        $dep_file =~ s|\.o$|.d|;

		# basic compiler params
		push @args, @params, @sdl_opts, @qt_opts;

        my @check_files;
        if(-e $dep_file) {
            open(my $fh, '<', $dep_file);
            for(my $line = <$fh>) {
                chomp $line;
                push @check_files, $line;
            }
            close $fh;
        } else {
            my @filter_args = (@args, qw|-MM -MG -MF - -MT|, $input_file, $input_file);
            my $out = run_capture(@filter_args);

            $out =~ s/\015//gs;
            $out =~ s/\\\n/ /gs;
            $out =~ s/^.*?://gs;
            $out =~ s/^[ \n\t]+//gs;
            $out =~ s/[ \n\t]+$//gs;
            $out =~ s/[ \n\t]+/ /gs;

            push @check_files, grep { $_ ne '' } map { process_path($_) } split(/ +/, $out);

            open(my $fh, '>', $dep_file);
            print $fh join("\n", @check_files);
            print $fh "\n";
            close $fh;
        }

		push @args, '-c', '-o';
	} elsif($output_file =~ s|([^/]*)\.h$|moc_$1.cpp|) {
		push @check_files, $input_file;
		push @args, $moc, '-o';
	} elsif($output_file =~ s|([^/]*)\.ui$|ui_$1.h|) {
		push @check_files, $input_file;
		push @args, 'uic';
	} elsif($output_file =~ s|([^/]*)\.ico$|icon_$1.o|) {
		push @check_files, $input_file;
		push @args, 'windres', '-o';
	} else {
		die "Failed to understand input file $input_file";
	}

	if(!$force && -e $output_file) {
		my @changed_files = run_time_check($output_file, @check_files);

		if($#changed_files == 0) {
			say "$changed_files[0] has changed";
		} elsif($#changed_files > 0) {
			my $last = pop @changed_files;
			print join ', ', @changed_files;
			say " and $last have changed";
		} else {
			say "Nothing to do for $input_file";
			return;
		}
	}

	# compile time stamp
	if($input_file eq 'src/help.cpp') {
		my $time_stamp = strftime('%d %B %Y', localtime);
		$time_stamp =~ s/^0+//;

		open(my $fh, '>', 'obj/comptime.h');
		print $fh qq|#define COMPILE_TIME "$time_stamp"\n|;
		close $fh;

		$need_updated_time_stamp = 0;
	}

	# run compiler
	my $res;
	if($args[0] eq 'uic') {
		$res = run_uic($input_file, $output_file);
	} elsif($args[0] eq 'windres') {
		say qq|echo 1 ICON "$input_file" \| windres -o $output_file|;
		open (my $pipe, '|-', 'windres', '-o', $output_file);
		print $pipe qq|1 ICON "$input_file"|;
		close $pipe;
		$res = $?;
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
	my $qt_install_headers = get_command("$qmake -query QT_INSTALL_HEADERS");
	@qt_opts = ('-isystem' . $qt_install_headers);
	push @qt_opts, map { "-isystem$qt_install_headers/$_" } @qt_modules;
	push @qt_opts, '-fPIC';

	# sdl opts
	@sdl_opts = sdl_cflags();

	if(!-d 'obj') { mkdir 'obj'; }

	# compile
	my @files_to_compile = qw|src/TWMainWnd.ui src/tworld.cpp src/series.cpp src/play.cpp src/encoding.cpp src/solution.cpp
	src/res.cpp src/lxlogic.cpp src/mslogic.cpp src/unslist.cpp src/messages.cpp src/help.cpp
	src/score.cpp src/random.cpp src/settings.cpp src/fileio.cpp src/err.cpp src/tile.cpp src/utils.cpp
	src/timer.cpp src/sdlsfx.cpp src/oshwbind.cpp src/TWTableSpec.cpp src/CCMetaData.cpp src/TWDisplayWidget.cpp
	src/TWProgressBar.cpp src/TWMainWnd.cpp src/TWMainWnd.h
	obj/moc_TWMainWnd.cpp src/TWApp.cpp|;

	if($^O eq 'msys' || $^O eq 'MSWin32') {
		push @files_to_compile, 'tworld.ico';
	}

	foreach my $f (@files_to_compile) {
		compile_file($f, 0);
	}
}

sub linker {
	my @object_files = <obj/*.o>;

	if(-f $executable_name) {
		my @changed_files = run_time_check($executable_name, @object_files);

		if($#changed_files < 0) {
			say "Not linking as no changes have been made";
			return;
		}
	}

	my @link_command = ('c++', '-o', $executable_name, @object_files);

	# add qt's lib dir
	my $qt_install_libs = get_command("$qmake -query QT_INSTALL_LIBS");
	push @link_command, '-L' . $qt_install_libs;

	# add frameworks or libraries
	if($^O eq 'darwin') {
		my $qt_install_prefix = get_command("$qmake -query QT_INSTALL_PREFIX");

		push @link_command, '-F' . $qt_install_prefix . '/Frameworks';
		push @link_command, map { ('-framework' , $_)  } @qt_modules;
	} else {
        my $qt_major_version = get_command("$qmake -query QT_VERSION");
        $qt_major_version =~ s/\..*$//;

		my @largs = @qt_modules;
		foreach (@largs) {s/Qt/-lQt$qt_major_version/;}

		push @link_command, @largs;
	}

	# sdl options
	push @link_command, sdl_libs();

	# now that we're linking, compile help for timestamp if we haven't already done so
	if($need_updated_time_stamp) {
		compile_file('src/help.cpp', 1);
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
	if($^O eq 'darwin') {
		system 'rm', '-fR', 'Tile World.app';
	}

	if($^O eq 'MSWin32') {
		File::Path::rmtree('dist');
	} else {
		system 'rm', '-fR', 'dist';
	}

	unlink $executable_name;
	unlink <obj/*.*>;
}

sub mkdist {
	if($^O eq 'darwin') {
		system 'mkdir', '-p', 'Tile World.app/Contents/MacOS';
		system 'mkdir', '-p', 'Tile World.app/Contents/Resources';
		system 'cp', 'Info.plist', 'Tile World.app/Contents/';
		system 'cp', 'tworld.icns', 'Tile World.app/Contents/Resources/Tile World.icns';
		system 'cp', $executable_name, 'Tile World.app/Contents/MacOS/Tile World';

		system 'cp', '-R', 'res', 'Tile World.app/Contents/Resources/';
		system 'cp', '-R', 'data', 'Tile World.app/Contents/Resources/';
		system 'cp', '-R', 'licences', 'Tile World.app/Contents/Resources/';
	} elsif($^O eq 'msys') {
		system 'mkdir', '-p', 'dist';
		system 'cp', 'tworld.exe', 'dist/';
		system 'cp', 'tworld.png', 'dist/';

		system 'cp', '-R', 'res', 'dist/';
		system 'cp', '-R', 'data', 'dist/';
		system 'cp', '-R', 'licences', 'dist/';
	} elsif($^O eq 'MSWin32') {
		File::Copy::Recursive::fcopy('tworld.exe', 'dist/tworld.exe');
		File::Copy::Recursive::fcopy('tworld.png', 'dist/tworld.png');
		File::Copy::Recursive::dircopy('res', 'dist/res');
		File::Copy::Recursive::dircopy('data', 'dist/data');
		File::Copy::Recursive::dircopy('licences', 'dist/licences');
	} else {
		say "Failed: No distribution recipe for $^O";
		exit 1;
	}
}

sub install {
	if($^O eq 'linux') {
		system 'cp', $executable_name, '/usr/local/games/';

		system 'mkdir', '-p', '/usr/local/share/tworld';
		system 'cp', '-R', 'res', '/usr/local/share/tworld/';
		system 'cp', '-R', 'data', '/usr/local/share/tworld/';
		system 'cp', '-R', 'licences', '/usr/local/share/tworld/';
		system 'cp', 'tworld.png', '/usr/local/share/tworld/';

		if(-d '/usr/share/applications') {
			system 'cp', 'tworld.desktop', '/usr/share/applications/';
		}
	} elsif($^O eq 'darwin') {
		if(!-d 'Tile World.app') {
			mkdist();
		}

		system 'mv', 'Tile World.app', '/Applications/';
	} else {
		say "Sorry no install script for $^O";
		exit 1;
	}
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
    my $input_file = shift;
	my $output_file = shift;
	my $res = 0;

	open(my $PIPE, '-|', $uic, $input_file) || return 1;
	my $code = join '', <$PIPE>;
	close $PIPE;

	say "uic ran sucessfully";

	# Pass the scale attribute directly to the game and objects
	# widgets so the initial size corresponds to the users zoom preference.
	$res += $code =~ s|(void setupUi\(.*?)\)|$1, double scale)|;

	$res += $code =~ s|m_pGameWidget->setMinimumSize\(.*?\);|m_pGameWidget->setFixedSize(scale * DEFAULTTILE * NXTILES, scale * DEFAULTTILE * NYTILES);|;
	$res += $code =~ s|m_pObjectsWidget->setMinimumSize\(.*?\);|m_pObjectsWidget->setFixedSize(scale * DEFAULTTILE * 4, scale * DEFAULTTILE * 2);|;

	$res += $code =~ s|m_pMessagesFrame->setMinimumWidth\(.*?\);|m_pMessagesFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);|;
	$res += $code =~ s|m_pInfoFrame->setMinimumWidth\(.*?\);|m_pInfoFrame->setFixedWidth((4 * DEFAULTTILE * scale) + 10);|;

	$res += $code =~ s|^(#include.*)\n\n|$1\n#include "../src/tile.h"\n\n|m;

	# Counterintuitive, using points makes the app less portable rather than more as
	# operating systems assume a notional dpi instead of a real one (on Mac it's 72,
	# on Windows and Linux it's 96). So switching to pixels ensures fonts will render
	# the same on all three operating systems.
	$res += $code =~ s|setPointSize|setPixelSize|g;

	# fix macro include
	$code =~ s| TWMAINWND_H| UI_TWMAINWND_H|g;

	if($res == 10) {
		open(my $OUT, '>', $output_file) || return 1;
		print $OUT $code;
		close $OUT;
		return 0;
	} else {
		say "Failed: script make $res replacements instead of 10";
		return 1;
	}
}

sub check_path {
	if(which('sdl2-config')) {
		$sdl_config = 'sdl2-config';
	} elsif(which('sdl-config')) {
		say "Using SDL1 as can't find sdl2-config";
		$sdl_config = 'sdl-config';
	} elsif($^O eq 'MSWin32') {
		if(-f 'paths.ini') {
			open(my $fh, '<', 'paths.ini');
			my %paths;
			while(my $line = <$fh>) {
				if($line =~ m/^(.*?)=(.*)$/) {
					$paths{$1} = $2;
				}
			}
			close $fh;

			$sdl_include_path = $paths{sdl_include_path};
			$sdl_lib_path = $paths{sdl_lib_path};

			if(!defined $sdl_include_path || !defined $sdl_lib_path) {
				say "paths.ini is corrupted.";
				exit 1;
			}
		} else {
			say "Please enter your sdl lib directory (ie where libSDL2.a or libSDL.a is):";
			$sdl_lib_path = <STDIN>;
			exit 1 unless defined $sdl_lib_path;
			chomp $sdl_lib_path;
			if(!-d $sdl_lib_path) {
				say "Failed to find directory";
				exit 1;
			}

			say "Please enter your sdl include directory (ie where SDL.h or SDL.h is):";
			$sdl_include_path = <STDIN>;
			exit 1 unless defined $sdl_include_path;
			chomp $sdl_include_path;
			if(!-d $sdl_include_path) {
				say "Failed to find directory";
				exit 1;
			}

			# save these directories for next time
			open (my $fh, '>', 'paths.ini');
			print $fh "sdl_lib_path=$sdl_lib_path\n";
			print $fh "sdl_include_path=$sdl_include_path\n";
			close $fh;
			say "These directories have been saved in paths.ini.";
		}
	} else {
		die "Found neither sdl2-config nor sdl-config.\n";
	}

	foreach my $command ($qmake, $uic, $moc) {
		if(!which($command)) {
			die "Can't find $command\n";
		}
	}
}

sub which {
	my $exe = shift;

	if($^O eq 'MSWin32') {
		return File::Which::which($exe) ? 1 : 0;
	} else {
		`which $exe 2> /dev/null > /dev/null`;
		return $? == 0 ? 1 : 0;
	}
}

sub get_command {
	my $cmd = shift;
	my $out = `$cmd`;
	$out =~ s/[\015\012]+$//; # better chomp

	if($^O eq 'msys2') { $out =~ s|^([A-Z]):/|/$1/|; }

	if(wantarray) {
		return split(/ +/, $out);
	} else {
		return $out;
	}
}

sub usage {
	say 'make.pl (compile|clean|mkdist|install)';
	exit;
}


sub sdl_cflags {
	if($^O eq 'MSWin32' && !defined $sdl_config) {
		return ('-isystem' . $sdl_include_path, '-Dmain=SDL_main');
	} else {
		my @flags = get_command("$sdl_config --cflags");
		foreach my $flag (@flags) {
		    $flag =~ s/-I/-isystem/g;
		}
		return @flags;
	}
}

sub sdl_libs {
	if($^O eq 'MSWin32' && !defined $sdl_config) {
		return ('-L' . $sdl_lib_path, '-lmingw32', '-lSDL2main', '-lSDL2', '-mwindows');
	} else {
		return get_command("$sdl_config --libs");
	}
}
