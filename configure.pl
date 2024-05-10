#!/usr/bin/env perl

use v5.20;
use strict;
use warnings;
use Getopt::Long;

my %vars;
my %opt = (
qmake => 'qmake6',
sdl => 'sdl2-config',
windres => 'windres',
moc => 'moc',
uic => 'uic',
cxx => 'c++',
);

# override default options which command line variables
for my $k (keys %opt) {
	if(defined $ENV{uc $k}) {
		$opt{$k} = $ENV{uc $k};
	}
}

GetOptions(
\%opt,
"help!",
"qmake=s",
"sdl=s",
"windres=s",
"moc=s",
"uic=s",
"cxx=s",
) || usage(1);

if($opt{help}) {
	usage(0);
}


# qmake
my $qmake = get_cmd_path('qmake');

# sdl2-config
my $sdl2_config = get_cmd_path('sdl');

# get useful qt vars
my %qt_vars = map { split /:/, $_, 2 } get_command_safe($qmake, '-query');

# major version
my $major_qt_version;
if($qt_vars{QT_VERSION} =~ /^5/) {
	$major_qt_version = '';
} elsif($qt_vars{QT_VERSION} =~ /^6/) {
	$major_qt_version = '6';
} else {
	say STDERR "QT version check failed: $qmake";
	exit 1;
}

# windows
if(defined $ENV{OS} && $ENV{OS} eq 'Windows_NT') {
	# fix qt paths on msys2
	if(-d '/C') {
		foreach my $v (values %qt_vars) {
			$v =~ s|\\|/|g;
			$v =~ s|^([A-Z]):|/$1|;
		}
	}

	# windres
	$vars{WINDRES} = get_cmd_path('windres');
}

# add qt to path
$ENV{PATH} = join ':', $qt_vars{QT_INSTALL_BINS}, $qt_vars{QT_INSTALL_LIBEXECS}, $ENV{PATH};

# get moc
$vars{MOC} = get_cmd_path('moc');

# get moc
$vars{UIC} = get_cmd_path('uic');

# compiler
$vars{CXX} = get_cmd_path('cxx');

# requires qt modules
my @qt_modules = qw|QtCore QtGui QtXml QtWidgets|;

# generic compiler flags
$vars{CFLAGS} = '-std=gnu++17 -Wall -pedantic -DNDEBUG -O2 -I. -Werror -fPIC';

# qt compiler flags (spaces after -isystem helps mingw gcc)
$vars{CFLAGS} .= " -isystem $qt_vars{QT_INSTALL_HEADERS}";
foreach my $module (@qt_modules) {
	$vars{CFLAGS} .= " -isystem $qt_vars{QT_INSTALL_HEADERS}/$module";
}

# sdl compiler flags
my $sdl2_config_cflags = get_command_safe($sdl2_config, '--cflags');
$sdl2_config_cflags =~ s|-I|-isystem |g;
$vars{CFLAGS} .= " $sdl2_config_cflags";

# linker flags
my $sdl2_config_libs = get_command_safe($sdl2_config, '--libs');
$sdl2_config_libs =~ s|-L|-L |g;
$vars{LDFLAGS} = $sdl2_config_libs;

# frameworks on Mac, libraries on other systems
if($^O eq 'darwin') {
	$vars{LDFLAGS} .= " -F $qt_vars{QT_INSTALL_LIBS}";
	foreach my $module (@qt_modules) {
		$vars{LDFLAGS} .= " -framework $module";
	}
} else {
	foreach my $module (@qt_modules) {
		my $m = $module;
		$m =~ s/Qt/-lQt$major_qt_version/;
		$vars{LDFLAGS} .= " $m";
	}
}

# create a list of source files
my @source_files = grep { !m/help\.cpp/ } <src/*.cpp>; # leave help.cpp to the end

# create a list of object files
my @object_files = @source_files;
map { s|^src/(.*?)\.[a-z]+$|obj/$1.o| } @object_files;

# use .exe filename extension on windows and add icon target
if(defined $ENV{OS} && $ENV{OS} eq 'Windows_NT') {
		$vars{TWORLD} = 'tworld.exe';
		push @object_files, 'obj/icon_tworld.o';
} else {
		$vars{TWORLD} = 'tworld';
}

$vars{OBJ_FILES} = join ' ', @object_files;
$vars{MOC_FILES} = 'obj/moc_TWMainWnd.o';
$vars{GEN_FILES} = 'obj/ui_TWMainWnd.h obj/moc_TWMainWnd.cpp';

#open makefile
open my $mkfile, '>', 'Makefile' || die "Failed to open Makefile";

# print out vars
foreach my $k (sort keys %vars) {
	say $mkfile "$k := $vars{$k}";
}

print $mkfile <<'MAKEFILE';

CREATE_OBJ_DIR := $(shell [ ! -d obj ] && echo create_obj_dir)

# only compile obj/comptime.h and obj/help.o when we're actually linking
$(TWORLD): $(GEN_FILES) $(OBJ_FILES) $(MOC_FILES) src/help.h src/help.cpp $(CREATE_OBJ_DIR)
	date '+#define COMPILE_TIME "%d %b %Y %T %Z"' > obj/comptime.h
	$(CXX) $(CFLAGS) -c src/help.cpp -o obj/help.o
	$(CXX) -o $(TWORLD) $(OBJ_FILES) obj/help.o $(MOC_FILES) $(LDFLAGS)

.PHONY: create_obj_dir
create_obj_dir:
	mkdir -p obj

obj/moc_TWMainWnd.o: obj/moc_TWMainWnd.cpp $(CREATE_OBJ_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp $(CREATE_OBJ_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

obj/ui_TWMainWnd.h: src/TWMainWnd.ui src/tile.h $(CREATE_OBJ_DIR)
	$(UIC) $< | ./fix_uic.pl > $@

obj/moc_TWMainWnd.cpp: src/TWMainWnd.h $(CREATE_OBJ_DIR)
	$(MOC) $< -o $@

obj/icon_tworld.o: src/tworld.ico $(CREATE_OBJ_DIR)
	echo 1 ICON $< | $(WINDRES) -o $@

.PHONY: clean
clean:
	rm -fr 'Tile World.app'
	rm -fr dist
	rm -f obj/* $(TWORLD)

.PHONY: app
app: tworld
	rm -fr 'Tile World.app'
	mkdir -p 'Tile World.app/Contents/MacOS'
	mkdir -p 'Tile World.app/Contents/Resources'
	cp Info.plist 'Tile World.app/Contents/'
	cp tworld.icns 'Tile World.app/Contents/Resources/Tile World.icns'
	cp tworld 'Tile World.app/Contents/MacOS/Tile World'
	cp -R res 'Tile World.app/Contents/Resources/'
	cp -R data 'Tile World.app/Contents/Resources/'
	cp -R licences 'Tile World.app/Contents/Resources/'

.PHONY: dist
dist: $(TWORLD)
	rm -fr dist
	mkdir dist
	cp $(TWORLD) dist/
	cp tworld.png dist/
	cp -R res dist/
	cp -R data dist/
	cp -R licences dist/

.PHONY: install
install: tworld
	cp tworld /usr/local/games/
	strip /usr/local/games/tworld
	mkdir -p /usr/local/share/tworld
	cp -R res /usr/local/share/tworld/
	cp -R data /usr/local/share/tworld/
	cp -R licences /usr/local/share/tworld/
	cp tworld.png /usr/local/share/tworld/
	-cp tworld.desktop /usr/share/applications/


MAKEFILE
;


# now get the dependencies
my @dep_command = ($vars{CXX});
push @dep_command, split / +/, $vars{CFLAGS};
push @dep_command, '-MM', '-MG', '-MT';

foreach my $src (@source_files) {
	my $obj = $src;
	$obj =~ s|^src/(.*?)\.cpp$|obj/$1.o| || next;

	say join ' ', @dep_command, $obj, '-c', $src, '-o', '-';
	open(my $PIPE, '-|', @dep_command, $obj, '-c', $src, '-o', '-') || error("c++ failed to run");
	my $deps = join '', <$PIPE>;
	close $PIPE;

	# clean up bad paths
	# changes "../obj/ui_TWMainWnd.h" to "obj/ui_TWMainWnd.h"
	$deps =~ s![^ ]+(src|obj)/([^ ]+\.h)!$1/$2!g;

	# remove obj/comptime.h dep
	$deps =~ s|obj/comptime.h||;

	print $mkfile "\n";
	print $mkfile $deps;
}

close $mkfile;


sub get_command {
	open(my $fh, '-|', @_) || return '';

	my @out;
	while(my $line = <$fh>) {
		$line =~ s/[\012\015]+$//;
		push @out, $line;
	}

	return undef if @out == 0 || !close $fh || $? != 0;

	return wantarray ? @out : $out[0];
}

sub get_command_safe {
	my @r = get_command(@_);

	if(!@r) {
		say "Cmd failed: ", join ' ', @_;
		exit 1;
	}

	return wantarray ? @r : $r[0];
}

sub get_cmd_path {
	my $cmd = shift;

	die $cmd unless defined $opt{$cmd};
	$cmd = $opt{$cmd};

	my $path;
	if($cmd =~ m|^/|) {
		$path = $cmd;
	} else {
		$path = get_command('which', $cmd);
	}

	if(!$path || !-f $path) {
		say STDERR "Failed to find $cmd";
		exit 1;
	}
	return $path;
}


sub usage {
	my $return_value = shift;
	print <<"END_OF_USAGE";
Usage: ./configure.pl

Options:
	--qmake    name of or path to qmake executable
	--sdl      name of or path to sdl config executable
	--moc      name of or path to moc executable
	--uic      name of or path to uic executable
	--cxx      name of or path to c++ executable
	--windres  name of or path to windres executable

These options can also be set using uppercase environmental
variables eg QMAKE=/path/to/qmake ./configure.pl

END_OF_USAGE

	exit $return_value;
}

