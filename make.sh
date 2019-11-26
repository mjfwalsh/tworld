#!/bin/bash
PATH=$PATH:/usr/local/opt/qt/bin
C_BASE="cc -Wall -pedantic -O2 -I. -DNDEBUG -Dstricmp=strcasecmp -std=gnu11 -Werror"
CPP_BASE="c++ -Wall -pedantic -O2 -I. -DNDEBUG -Dstricmp=strcasecmp -std=gnu++11 -Werror"
SDL_OPTS='-I/usr/local/include/SDL -D_GNU_SOURCE=1 -D_THREAD_SAFE'
QT_OPTS='-I/usr/local/opt/qt/include -I/usr/local/opt/qt/include/QtCore -I/usr/local/opt/qt/include/QtGui -I/usr/local/opt/qt/include/QtXml -I/usr/local/opt/qt/include/QtWidgets'
PRINT_DIR=''

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
	compile_file "$C_BASE" tworld.o tworld.c
	compile_file "$C_BASE" series.o series.c
	compile_file "$C_BASE" play.o play.c
	compile_file "$C_BASE" encoding.o encoding.c
	compile_file "$C_BASE" solution.o solution.c
	compile_file "$C_BASE" res.o res.c
	compile_file "$C_BASE" lxlogic.o lxlogic.c
	compile_file "$C_BASE" mslogic.o mslogic.c
	compile_file "$C_BASE" unslist.o unslist.c
	compile_file "$CPP_BASE" messages.o messages.cpp
	echo Generating comptime.h...
	echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" \> comptime.h
	echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" > comptime.h
	compile_file "$C_BASE" help.o help.c
	compile_file "$CPP_BASE" score.o score.cpp
	compile_file "$C_BASE" random.o random.c
	compile_file "$CPP_BASE" settings.o settings.cpp
	compile_file "$C_BASE" fileio.o fileio.c
	compile_file "$C_BASE" err.o err.c

	#cd oshw-qt
	#PRINT_DIR='generic/'
	compile_file "$C_BASE" generic.o generic.c
	compile_file "$C_BASE" tile.o tile.c
	compile_file "$C_BASE" timer.o timer.c
	compile_file "$CPP_BASE $QT_OPTS" _in.o _in.cpp
	#PRINT_DIR='oshw-qt/'
	compile_file "$C_BASE $SDL_OPTS" _sdlsfx.o _sdlsfx.c
	compile_file "$CPP_BASE $QT_OPTS" oshwbind.o oshwbind.cpp
	compile_file "$CPP_BASE $QT_OPTS" CCMetaData.o CCMetaData.cpp
	compile_file "$CPP_BASE $QT_OPTS" TWDisplayWidget.o TWDisplayWidget.cpp
	compile_file "$CPP_BASE $QT_OPTS" TWProgressBar.o TWProgressBar.cpp
	make_file "Compiling UI TWMainWnd.ui..." "uic -o" ui_TWMainWnd.h TWMainWnd.ui
	compile_file "$CPP_BASE $QT_OPTS" TWMainWnd.o TWMainWnd.cpp
	make_file "MOC-ing TWMainWnd.h..." "moc -o" moc_TWMainWnd.cpp TWMainWnd.h
	compile_file "$CPP_BASE $QT_OPTS" moc_TWMainWnd.o moc_TWMainWnd.cpp
	compile_file "$CPP_BASE $QT_OPTS" TWApp.o TWApp.cpp
	#cd ..
	#PRINT_DIR=''

	echo Linking tworld2...
	run c++ -o tworld2 tworld.o series.o play.o encoding.o solution.o res.o lxlogic.o mslogic.o unslist.o messages.o help.o score.o random.o settings.o fileio.o err.o \
	generic.o tile.o timer.o _in.o _sdlsfx.o oshwbind.o CCMetaData.o TWDisplayWidget.o TWProgressBar.o TWMainWnd.o moc_TWMainWnd.o TWApp.o \
	-L/usr/local/opt/qt/lib -F/usr/local/opt/qt/Frameworks -framework QtCore -framework QtGui -framework QtXml -framework QtWidgets -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa
	if [ $? -eq 0 ]
	then
		echo Done...
	else
		echo Failed...
	fi
}

clean () {
	#rm -fR Tile\ World.app
	rm -f cmdline.o comptime.h encoding.o err.o fileio.o help.o lxlogic.o messages.o mslogic.o _in.o _sdlsfx.o CCMetaData.o generic.o moc_TWMainWnd.cpp moc_TWMainWnd.o oshwbind.o tile.o timer.o TWApp.o TWDisplayWidget.o TWMainWnd.o TWProgressBar.o ui_TWMainWnd.h play.o random.o res.o score.o series.o settings.o solution.o tworld.o tworld2 unslist.o
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
	cp -R sets Tile\ World.app/Contents/Resources/
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

