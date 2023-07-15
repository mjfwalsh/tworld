# determine OS
OS ?= $(shell uname)
OS := $(OS)

# qt utilities have a habit of being placed in odd places
export PATH := $(shell qmake6 -query QT_HOST_LIBEXECS):$(PATH)

# useful qt vars
QT_PREFIX := $(shell qmake6 -query QT_INSTALL_HEADERS)
QT_MODULES := QtCore QtGui QtXml QtWidgets

# generic compiler flags
CFLAGS := -std=gnu++17 -Wall -pedantic -DNDEBUG -O2 -I. -Werror -fPIC

# qt compiler flags (spaces after -isystem helps mingw gcc)
CFLAGS += -isystem $(QT_PREFIX)
CFLAGS += $(foreach module,$(QT_MODULES),-isystem $(QT_PREFIX)/$(module))

# sdl compiler flags
CFLAGS += $(shell sdl2-config --cflags | sed -e 's|-I|-isystem |g')

# linker flags
LDFLAGS := $(shell sdl2-config --libs | sed -e 's|-L|-L |g')
LDFLAGS += -L $(shell qmake6 -query QT_INSTALL_LIBS)

# frameworks on Mac, libraries on other systems
ifeq ($(OS),Darwin)
	LDFLAGS += -F $(shell qmake6 -query QT_HOST_PREFIX)/Frameworks
	LDFLAGS += $(foreach module,$(QT_MODULES),-framework $(module))
else
	LDFLAGS += $(subst Qt,-lQt6,$(QT_MODULES))
endif

SRC := $(filter-out src/help.cpp,$(wildcard src/*.cpp)) # leave help.cpp to the end
MOC := obj/moc_TWMainWnd.o
GEN_SRC := obj/ui_TWMainWnd.h obj/moc_TWMainWnd.cpp
OBJS := $(patsubst src/%.cpp,obj/%.o,$(SRC))
DEP := $(patsubst src/%.cpp,deps/%.d,$(SRC))

# use .exe filename extension on windows and add icon target
ifeq ($(OS),Windows_NT)
    TWORLD := tworld.exe
    SRC += tworld.ico
    OBJS += obj/icon_tworld.o
else
    TWORLD := tworld
endif

# only compile obj/comptime.h and obj/help.o when we're actually linking
$(TWORLD): $(GEN_SRC) $(OBJS) $(MOC) src/help.h src/help.cpp
	date '+#define COMPILE_TIME "%d %b %Y %T %Z"' > obj/comptime.h
	$(CXX) $(CFLAGS) -c src/help.cpp -o obj/help.o
	$(CXX) -o $(TWORLD) $(OBJS) obj/help.o $(MOC) $(LDFLAGS)

# avoid creating dep files when we're about to delete them
ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
endif

obj/moc_TWMainWnd.o: obj/moc_TWMainWnd.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

obj/ui_TWMainWnd.h: src/TWMainWnd.ui src/tile.h
	./run_uic.pl $< $@

obj/moc_TWMainWnd.cpp: src/TWMainWnd.h
	env moc $< -o $@

obj/icon_%.o: src/%.ico
	echo 1 ICON $< | windres -o $@

deps/%.d: src/%.cpp
	$(CXX) $(CFLAGS) -MM -MG -MT obj/$(@F:.d=.o) -c $< -o - | ./fix_deps.pl $@

.PHONY: clean
clean:
	rm -fr 'Tile World.app'
	rm -fr dist
	rm -f deps/* obj/* tworld

.PHONY: mkapp
mkapp: tworld
	rm -fr 'Tile World.app'
	mkdir -p 'Tile World.app/Contents/MacOS'
	mkdir -p 'Tile World.app/Contents/Resources'
	cp Info.plist 'Tile World.app/Contents/'
	cp tworld.icns 'Tile World.app/Contents/Resources/Tile World.icns'
	cp tworld 'Tile World.app/Contents/MacOS/Tile World'
	cp -R res 'Tile World.app/Contents/Resources/'
	cp -R data 'Tile World.app/Contents/Resources/'
	cp -R licences 'Tile World.app/Contents/Resources/'

.PHONY: mkdist
mkdist: $(TWORLD)
	rm -fr dist
	mkdir dist
	cp $(TWORLD) dist/
	cp tworld.png dist/
	cp -R res dist/
	cp -R data dist/
	cp -R licences dist/

.PHONY: install
install: $(TWORLD)
	cp $(TWORLD) /usr/local/games/
	strip /usr/local/games/$(TWORLD)
	mkdir -p /usr/local/share/tworld
	cp -R res /usr/local/share/tworld/
	cp -R data /usr/local/share/tworld/
	cp -R licences /usr/local/share/tworld/
	cp tworld.png /usr/local/share/tworld/
	-cp tworld.desktop /usr/share/applications/
