/*

WRITTEN BY: 
Fiel of Southperry.net

NAME OF PROGRAM:
NXPatcher

PURPOSE OF PROGRAM:
To read, write, and create Nexon pre-patchers for the game Maplestory - to apply
patch files, and to check versions of wz files distributed by Nexon America.

EXPLANATION OF STRUCTURE:
This program follows an extreme top-down perspective where functions closer to main
have a higher amount of abstraction, all the way down to the lowest level of abstraction
(FileIO, dictionary, etc.).

The absolute highest level of abstraction is for the user. The user inputs commands from the command line.
The second argument, "the command", from the command line is always one of five words - read, write, split, hijack, and version - and
the third argument is always the name of the file that should be used with the command.

read = read and apply the patch file to the current directory
write = read a patch file and create a Nexon pre-patcher from it
split = Take a nexon pre-patcher and split it into its three parts - base file, patch file, and notice file
hijack = Read the patch from from a Nexon pre-patcher and apply that patch file to the current directory
version = Determine the version of a given wz file

Main interprets the second and third arguments. Depending on what the second argument is, a module associated
with that command is executed. All functions from Main are handed off to somewhere in the Patch Functions. 
From there, each module then uses its own module's functions and routines, and occassionally borrows from
lower level functions in FileIO, Checksum, and INIManager.

Checksum.c - Performs standard PKZIP crc32 on a given data stream
FileIO.c - Standard IO library - built with Windows in mind and is not POSIX compliant
INIManager - This is an abstraction layer built upon the INIParser and dictionary objects to use INI files

LICENSING & DEPENDENCIES:
NXPatcher - MIT License
iniparser.c/h and dictionary.c - MIT License
zlib - See zlib.h


*/

/*

Purpose of this file:
Accept command line arguments. If the command arguments are not as expected,
print out a menu and exit.

*/

/* 

USAGE:

NXPatcher.exe read XXXXXtoYYYYY.patch
NXPatcher.exe write XXXXXtoYYYYY.patch
NXPatcher.exe split XXXXXtoYYYYY.exe
NXPatcher.exe hijack XXXXtoYYYYY.exe
NXPatcher.exe version XXXX.wz

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FileIO.h"
#include "INIManager.h"
#include "Struct.h"

#include "read.h"
#include "hijack.h"
#include "split.h"
#include "write.h"
#include "version.h"

void PrintMenu(void);

//Accept command line arguments
//Determine from second command line argument which procedure to perform
int main(int argc, char* argv[])
{
	char* fileSuffix;
	char* inisearch;
	char* command;
	char* argv2[] = {"", "", ""};

	INILoadManager(argv[argc-1]);

	if(argc == 3) 
	{
		if(strcmp(argv[1], "read") == 0)
		{
			PatchReaderMain(argv[2]);
		}
		else if(strcmp(argv[1], "write") == 0)
		{
			PatchWriterMain(argv);
		}
		else if(strcmp(argv[1], "split") == 0)
		{
			PatchSplitMain(argv[2]);
		}
		else if(strcmp(argv[1], "version") == 0)
		{
			VersionMain(argv[2], 1);
		}
		else if(strcmp(argv[1], "hijack") == 0)
		{
			HijackMain(argv[2]);
		}
		else
		{
			PrintMenu();
		}
	}
	else if(argc == 2)
	{
		//Possible drag & drop here or file association.
		//search in INI for that suffix to see which command to perform
		fileSuffix = GetFileSuffix(argv[1]);
		inisearch = str_join("fileassociations", ":", fileSuffix);
		command = INIGetString(inisearch);
		free(inisearch);

		argv2[0] = argv[0];
		argv2[1] = command;
		argv2[2] = argv[1];

		main(3, argv2);
	}
	else 
	{
		PrintMenu();
	}

	INIDestroyManager();

	return EXIT_SUCCESS;
}

//Output a menu - this is usually done when there is a problem with the input variables
void PrintMenu(void)
{
	printf("NXPatcher - Version %s\nhttp://www.southperry.net/\n\n", "1.7");

	system("PAUSE");

	exit(EXIT_FAILURE);
}