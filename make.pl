#!/usr/bin/perl -w

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
my $time_stamp_file = 'help.c';
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
		if(defined $r->{'ui_TWMainWnd.h'}) {
			$r->{'TWMainWnd.ui'} = 1;
		}

		# and remove dependancy for comptime.h
		if(defined $r->{'comptime.h'}) {
			delete $r->{'comptime.h'};
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
		
	} elsif($output_file =~ s/^(.*)\.h$/moc_$1.cpp/) {
		$action = "MOC-ing $input_file...";
		push @args, 'moc', '-o';
	} elsif($output_file =~ s/^(.*)\.ui$/ui_$1.h/) {
		$action = "Compiling UI $input_file...";
		push @args, './uic.pl', '-o';
	} else {
		die "Failed to understand input file $input_file";
	}
	
	if($force) { 

	} elsif (-e $output_file) {
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
	my $res = run(@args, $output_file, $input_file);
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
	
	# compile
	my @files_to_compile = qw|tworld.c series.c play.c encoding.c solution.c
	res.cpp lxlogic.c mslogic.c unslist.c messages.cpp help.c score.cpp
	random.c settings.cpp fileio.c err.c generic.c tile.c timer.c sdlsfx.c
	oshwbind.cpp CCMetaData.cpp TWDisplayWidget.cpp TWProgressBar.cpp TWMainWnd.ui
	TWMainWnd.cpp TWMainWnd.h moc_TWMainWnd.cpp TWApp.cpp|;
	foreach my $f (@files_to_compile) {
		compile_file($f, 0);
	}
}

sub linker {
	my @object_files = <*.o>;
	my $executable_name = 'tworld2';
	
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
	unlink qw|moc_TWMainWnd.cpp tworld2|;
	unlink <*.o>;
}

sub mkapp {
	system 'mkdir', '-p', 'Tile World.app/Contents/MacOS';
	system 'mkdir', '-p', 'Tile World.app/Contents/Resources';
	system 'cp', 'Info.plist', 'Tile World.app/Contents/';
	system 'cp', 'Tile World.icns', 'Tile World.app/Contents/Resources/';
	system 'cp', 'tworld2', 'Tile World.app/Contents/MacOS/Tile World';

	system 'cp', '-R', 'res', 'Tile World.app/Contents/Resources/';
	system 'cp', '-R', 'sets', 'Tile World.app/Contents/Resources/';
	system 'cp', '-R', 'data', 'Tile World.app/Contents/Resources/';
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
		next if defined $files_done{$file};
		
		if($file =~ /\.(c|cpp|h)$/) {
			open (my $fh, '<', $file) || next;
			while(my $l = <$fh>) {
				if($l =~ m/^[\t ]*#include[\t ]+"([^"]+)"/) {
					push @files, $1;
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

sub usage {
	say 'make.pl (compile|clean|mkapp|install)';
	exit;
}
