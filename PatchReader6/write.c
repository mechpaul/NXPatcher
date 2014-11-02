#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "FileIO.h"
#include "INIManager.h"
#include "Struct.h"

#include "write.h"

void PatchWriterMain(char* argv[])
{
	//Base file
	struct Data Base;

	//Patch file
	struct Data Patch;

	//Notice
	struct Data Notice;
	const char defaultNotice[] = "Written by Fiel - http://www.southperry.net/";

	//Output file
	struct Data Output;
	const int patchCRC = 0xF2F7FBF3;

	//Self EXE file
	struct Data PatchEXE;
	struct Data ZlibBlock;

	PatchEXE.filename = SwitchSuffix(argv[0], ".exe");
	Patch.filename = SwitchSuffix(argv[2], ".patch");
	Base.filename = INIGetString("write:inputbase");
	Notice.filename = INIGetString("write:inputnotice");
	Output.filename = INIGetString("write:outputexe");

	if(INIGetBool("write:ifinputbase") == 1)
	{
		printf("Accessing user-created base file %s...\n", Base.filename);
		Base.length = GetEOFFile(Base.filename);
		Base.data = ReadTheFile(Base.filename);
	}
	else
	{
		printf("Accessing default base file...\n");
		//The "default base file" here is in NXPatcher itself.
		//The NXPatcher.exe is formatted like so...
		//NXPatcher EXE --> Standard Base File (packed in ZLIB) --> length of packed zlib (4 bytes)
		//So PatchEXE struct reads NXPatcher.exe. The struct ZlibBlock takes in the zlib data
		//and the Base struct receives the unpacked ZlibBlock data.

		//By the end of this block of code, Base.data and Base.length MUST be populated to write
		//the finished EXE file at the end of this function
		PatchEXE.length = GetEOFFile(PatchEXE.filename);
		PatchEXE.fileptr = safe_fopen(PatchEXE.filename, "rb");

		//Get final four bytes
		fseek(PatchEXE.fileptr, PatchEXE.length - 4, SEEK_SET);
		fread(&ZlibBlock.length, 4, 1, PatchEXE.fileptr);

		//Find the location of the zlib block & unpack it
		fseek(PatchEXE.fileptr, PatchEXE.length - (4 + ZlibBlock.length), SEEK_SET);
		ZlibBlock.data = safe_malloc(ZlibBlock.length);
		fread(ZlibBlock.data, ZlibBlock.length, 1, PatchEXE.fileptr);
		Base.data = UnpackZlibBlock(ZlibBlock.data, ZlibBlock.length, &Base.length);
		free(ZlibBlock.data);
		fclose(PatchEXE.fileptr);
	}


	//Patch file
	printf("Accessing patch file...\n", Patch.filename);
	Patch.data = ReadTheFile(Patch.filename);
	Patch.length = GetEOFFile(Patch.filename);

	//Notice file
	if(INIGetBool("write:ifinputnotice") == 1)
	{
		printf("Accessing user-created notice file %s...\n", Notice.filename);
		Notice.length = GetEOFFile(Notice.filename);
		Notice.data = ReadTheFile(Notice.filename);
	}
	else
	{
		printf("Accessing default notice file...\n");
		Notice.length = strlen(defaultNotice) + 1;
		Notice.data = safe_malloc(Notice.length);
		memcpy(Notice.data, defaultNotice, strlen(defaultNotice) + 1);
	}

	//Commit
	printf("Assembling pre-patcher...\n");
	CreateNewDirectoryIterate(Output.filename);
	Output.fileptr = safe_fopen(Output.filename , "wb");
	fwrite(Base.data, Base.length, 1, Output.fileptr);
	fwrite(Patch.data, Patch.length, 1, Output.fileptr);
	fwrite(Notice.data, Notice.length, 1, Output.fileptr);
	fwrite(&Patch.length, 4, 1, Output.fileptr);
	fwrite(&Notice.length, 4, 1, Output.fileptr);
	fwrite(&patchCRC, 4, 1, Output.fileptr);
	fclose(Output.fileptr);

	printf("%s successfully assembled!\n", Output.filename);

	//Let's see if the user wants this to be autorun
	if(INIGetBool("write:ifautorun") == 1)
	{
		printf("Autostarting %s\n", Output.filename);
		system(Output.filename);
	}

	//Tidy up
	free(Notice.data);
	//free(Notice.filename); <-- gotten from the INI file

	free(Patch.data);
	free(Patch.filename);
	
	free(Base.data);
	//free(Base.filename); <-- gotten from the INI file

	free(PatchEXE.filename);

	//free(Output.filename);  <-- gotten from the INI file
}