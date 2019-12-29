#!/bin/bash

QT_MODULES=(Core Gui Xml Widgets)

# common compiler flags
COMMON_PARAMS="-Wall -pedantic -O2 -I. -DNDEBUG -Dstricmp=strcasecmp -Werror"

# c and c++ flags
CC="cc -std=gnu11 $COMMON_PARAMS"
CPP="c++ -std=gnu++11 $COMMON_PARAMS"

run () {
	echo "$@"
	"$@"
}

make_file () {
	if [ -e $3 ]
	then
		echo "Nothing to do for $4"
		return
	fi

	echo $1
	run $2 $3 $4
	if [ $? -ne 0 ]
	then
		echo Failed...
		exit 1
	fi
}

compile_file () {
	make_file "Compiling $PRINT_DIR$3..." "$1 -c -o" "$2" "$3"
}

compile () {
	# qmake
	which qmake > /dev/null
	if [ $? -ne 0 ]
	then
		# Homebrew does not link qt to /usr/local/bin by default
		if [ -e /usr/local/opt/qt/bin ]
		then
			PATH=$PATH:/usr/local/opt/qt/bin
		else
			echo Can\'t find qmake
			exit 1
		fi

		which qmake > /dev/null
		if [ $? -ne 0 ]
		then
			echo Can\'t find qmake
			exit 1
		fi
	fi

	# sdl-config or sdl2-config
	which sdl2-config > /dev/null
	if [ $? -ne 0 ]
	then
		which sdl-config > /dev/null
		if [ $? -ne 0 ]
		then
			echo Can\'t find sdl-config or sdl2-config in your path
			exit 1
		else
			echo Using SDL1 as can\'t find sdl2-config
			SDLCONFIG='sdl-config'
		fi
	else
		SDLCONFIG='sdl2-config'
	fi

	# Qt compiler opts
	QT_INCLUDE=-I`qmake -query QT_INSTALL_HEADERS`
	QT_DIRS[0]=$QT_INCLUDE
	QT_DIRS+=("${QT_MODULES[@]/#/$QT_INCLUDE/Qt}")
	QT_OPTS="${QT_DIRS[@]}"
	QT_OPTS+=" -fPIC"

	# Qt linker flags
	QT_LINKER_FLAGS=-L`qmake -query QT_INSTALL_LIBS`

	if [ `uname` = "Darwin" ]
	then
		QT_LINKER_FLAGS+=" -F"`qmake -query QT_INSTALL_PREFIX`"/Frameworks "
		QT_FRAMEWORKS_ARR=("${QT_MODULES[@]/#/-framework Qt}")
		QT_LINKER_FLAGS+="${QT_FRAMEWORKS_ARR[@]}"
	else
		QT_FRAMEWORKS_ARR=("${QT_MODULES[@]/#/-lQt5}")
		QT_LINKER_FLAGS+=" ${QT_FRAMEWORKS_ARR[@]}"
	fi

	# SDL compiler opts
	SDL_OPTS=`$SDLCONFIG --cflags`

	# SDL linker flags
	SDL_LINKER_FLAGS=`$SDLCONFIG --libs`

	# now compile the files
	compile_file "$CC" tworld.o tworld.c
	compile_file "$CC" series.o series.c
	compile_file "$CC" play.o play.c
	compile_file "$CC" encoding.o encoding.c
	compile_file "$CC" solution.o solution.c
	compile_file "$CC" lxlogic.o lxlogic.c
	compile_file "$CC" mslogic.o mslogic.c
	compile_file "$CC" unslist.o unslist.c
	compile_file "$CPP" messages.o messages.cpp
	echo Generating comptime.h...
	echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" \> comptime.h
	echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" > comptime.h
	compile_file "$CC" help.o help.c
	compile_file "$CPP" score.o score.cpp
	compile_file "$CC" random.o random.c
	compile_file "$CPP" settings.o settings.cpp
	compile_file "$CC" fileio.o fileio.c
	compile_file "$CC" err.o err.c
	compile_file "$CC" generic.o generic.c
	compile_file "$CC" tile.o tile.c
	compile_file "$CC" timer.o timer.c

	compile_file "$CPP $QT_OPTS" res.o res.cpp
	compile_file "$CC $SDL_OPTS" sdlsfx.o sdlsfx.c
	compile_file "$CPP $QT_OPTS" oshwbind.o oshwbind.cpp
	compile_file "$CPP $QT_OPTS" CCMetaData.o CCMetaData.cpp
	compile_file "$CPP $QT_OPTS" TWDisplayWidget.o TWDisplayWidget.cpp
	compile_file "$CPP $QT_OPTS" TWProgressBar.o TWProgressBar.cpp
	make_file "Compiling UI TWMainWnd.ui..." "./uic.pl -o" ui_TWMainWnd.h TWMainWnd.ui
	compile_file "$CPP $QT_OPTS" TWMainWnd.o TWMainWnd.cpp
	make_file "MOC-ing TWMainWnd.h..." "moc -o" moc_TWMainWnd.cpp TWMainWnd.h
	compile_file "$CPP $QT_OPTS" moc_TWMainWnd.o moc_TWMainWnd.cpp
	compile_file "$CPP $QT_OPTS" TWApp.o TWApp.cpp

	echo Linking tworld2...
	run c++ -o tworld2 *.o $QT_LINKER_FLAGS $SDL_LINKER_FLAGS
	if [ $? -eq 0 ]
	then
		echo Done...
	else
		echo Failed...
	fi
}

clean () {
	rm -fR Tile\ World.app
	rm -f *.o
	rm -f comptime.h moc_TWMainWnd.cpp ui_TWMainWnd.h tworld2
}

cleanui () {
	rm -f tworld2 ui_TWMainWnd.h TWMainWnd.o moc_TWMainWnd.cpp  moc_TWMainWnd.o
}

mkapp () {
	mkdir -p Tile\ World.app/Contents/MacOS
	mkdir -p Tile\ World.app/Contents/Resources
	cp Info.plist Tile\ World.app/Contents/
	cp Tile\ World.icns Tile\ World.app/Contents/Resources/
	cp tworld2 Tile\ World.app/Contents/MacOS/Tile\ World

	cp -R res Tile\ World.app/Contents/Resources/
	cp -R data Tile\ World.app/Contents/Resources/
}



if [ "$1" = "install" ]; then
	compile
	mkapp
	mv Tile\ World.app /Applications/
elif [ "$1" = "compile" ]; then
	compile
elif [ "$1" = "mkapp" ]; then
	mkapp
elif [ "$1" = "clean" ]; then
	clean
elif [ "$1" = "cleanui" ]; then
	cleanui
else
	compile
fi

