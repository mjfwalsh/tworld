#!/bin/bash
OSHW=oshw-qt
PATH=$PATH:/usr/local/opt/qt/bin

ROOT='-DROOTDIR="/usr/local/share/tworld"'

C_BASE="cc -Wall -pedantic -O2 -I. -DNDEBUG $ROOT -Dstricmp=strcasecmp -DTWPLUSPLUS -std=gnu11"
CPP_BASE="c++ -Wall -pedantic -O2 -I. -DNDEBUG $ROOT -Dstricmp=strcasecmp -DTWPLUSPLUS -std=gnu++11"
SDL_OPTS='-I/usr/local/include/SDL -D_GNU_SOURCE=1 -D_THREAD_SAFE'
QT_OPTS='-I/usr/local/Cellar/qt/5.12.3/include -I/usr/local/Cellar/qt/5.12.3/include/QtCore -I/usr/local/Cellar/qt/5.12.3/include/QtGui -I/usr/local/Cellar/qt/5.12.3/include/QtXml -I/usr/local/Cellar/qt/5.12.3/include/QtWidgets'

make_file2 () {
	if [ -e $3 ]
	then 
		echo "Nothing to do for $4"
		return
	fi

	echo $1
	echo $2 $3 $4
	$2 $3 $4
	if [ $? -ne 0 ]
	then
		echo Failed...
		exit
	fi
}

make_file () {
	make_file2 "Compiling $3..." "$@"
}

make_file "$C_BASE -c -o" tworld.o tworld.c
make_file "$C_BASE -c -o" series.o series.c
make_file "$C_BASE -c -o" play.o play.c
make_file "$C_BASE -c -o" encoding.o encoding.c
make_file "$C_BASE -c -o" solution.o solution.c
make_file "$C_BASE -c -o" res.o res.c
make_file "$C_BASE -c -o" lxlogic.o lxlogic.c
make_file "$C_BASE -c -o" mslogic.o mslogic.c
make_file "$C_BASE -c -o" unslist.o unslist.c
make_file "$CPP_BASE -c -o" messages.o messages.cpp
echo Generating comptime.h...
echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" > comptime.h
make_file "$C_BASE -c -o" help.o help.c
make_file "$CPP_BASE -c -o" score.o score.cpp
make_file "$C_BASE -c -o" random.o random.c
make_file "$C_BASE -c -o" cmdline.o cmdline.c
make_file "$CPP_BASE -c -o" settings.o settings.cpp
make_file "$C_BASE -c -o" fileio.o fileio.c
make_file "$C_BASE -c -o" err.o err.c

echo --- Building oshw-qt...

cd oshw-qt
make_file "$C_BASE $SDL_OPTS -c -o" generic.o ../generic/generic.c
make_file "$C_BASE $SDL_OPTS -c -o" tile.o ../generic/tile.c
make_file "$C_BASE $SDL_OPTS -c -o" timer.o ../generic/timer.c
make_file "$CPP_BASE $QT_OPTS -c -o" _in.o ../generic/_in.cpp
make_file "$C_BASE $SDL_OPTS -c -o" _sdlsfx.o _sdlsfx.c
make_file "$CPP_BASE $QT_OPTS -c -o" oshwbind.o oshwbind.cpp
make_file "$CPP_BASE $QT_OPTS -c -o" CCMetaData.o CCMetaData.cpp
make_file "$CPP_BASE $QT_OPTS -c -o" TWDisplayWidget.o TWDisplayWidget.cpp
make_file "$CPP_BASE $QT_OPTS -c -o" TWProgressBar.o TWProgressBar.cpp
make_file2 "Compiling UI TWMainWnd.ui..." "uic -o" ui_TWMainWnd.h TWMainWnd.ui
make_file "$CPP_BASE $QT_OPTS -c -o" TWMainWnd.o TWMainWnd.cpp
make_file2 "MOC-ing TWMainWnd.h..." "moc -o" moc_TWMainWnd.cpp TWMainWnd.h
make_file "$CPP_BASE $QT_OPTS -c -o" moc_TWMainWnd.o moc_TWMainWnd.cpp
make_file "$CPP_BASE $QT_OPTS -c -o" TWApp.o TWApp.cpp
echo ---
cd ..
echo Linking tworld2...
c++ -o tworld2 tworld.o series.o play.o encoding.o solution.o res.o lxlogic.o mslogic.o unslist.o messages.o help.o score.o random.o cmdline.o settings.o fileio.o err.o \
oshw-qt/generic.o oshw-qt/tile.o oshw-qt/timer.o oshw-qt/_in.o oshw-qt/_sdlsfx.o oshw-qt/oshwbind.o oshw-qt/CCMetaData.o oshw-qt/TWDisplayWidget.o oshw-qt/TWProgressBar.o oshw-qt/TWMainWnd.o oshw-qt/moc_TWMainWnd.o oshw-qt/TWApp.o \
-L/usr/local/Cellar/qt/5.12.3/lib -F/usr/local/opt/qt/Frameworks -framework QtCore -framework QtGui -framework QtXml -framework QtWidgets -L/usr/local/lib -lSDLmain -lSDL -Wl,-framework,Cocoa