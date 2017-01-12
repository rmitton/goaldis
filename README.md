# goaldis

This utility disassembles all the code/data on a PlayStation2 disc of Jak & Daxter.

Usage:

	dumpall.bat dirname C:\my_jak_files

Where `my_jak_files` is the directory containing the game files. You should be able to just put a PS2 disc in and dump it straight from the CD drive also (I didn't try it though).

I haven't tried this with Jak 2 or 3, no idea if it'll work or not.

See http://www.codersnotes.com/notes/disassembling-jak for more information.

NOTE: Visual Studio 2013 can't seem to compile this project in Release builds (srsly), so you'll only be able to build a debug build.
