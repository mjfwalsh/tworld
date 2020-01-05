Experimental version of Tile World. Tile World is an emulation of the game "Chip's Challenge" for the Atari Lynx, created by Chuck Sommerville, and subsequently ported to Windows.

![Screenshot](screenshot.png "Screenshot")

This is a fork of a fork or a fork. The original Tile World was written by Brian Raiter. Madhav Shanbhag created a fork of version 1.3.0 using the Qt Library and called it Tile World 2, although Brian Raiter subsequently produced a version 1.3.2 of the original.

The is a fork of Tile World 2.2.0 (the highest version I could find). I originally started it so I could get an 64-bit executable for Mac, but have added in a few UI improvements on the way, along with removing some of the older code.

To compile and run it you need [qt](https://www.qt.io/) and [SDL](https://www.libsdl.org/) (version 1 or 2) which can be installed via [Homebrew](https://brew.sh/).

To compile just run `./make.pl` from the command line.

`./make.sh install` will (hopefully) create an app bundle and move it to the /Applications folder.

Original source from: https://tw2.bitbusters.club/

Copyright &copy; 2001-2019 by Brian Raiter, Madhav Shanbhag, Eric Schmidt and Michael J Walsh

Released under GNU General Public License version 2 and above.

The sound effects were created by Brian Raiter, with assistance from SoX. They have been released into the public domain.

The tile images were created by Anders Kaseorg, with assistance from POV-Ray. They have also been released into the public domain.