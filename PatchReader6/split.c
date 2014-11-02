#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "FileIO.h"
#include "INIManager.h"
#include "Struct.h"

#include "split.h"

void PatchSplitMain(char* patchEXEFileName)
{
	struct PatchFile thisFile;
	struct Data Notice;
	struct Data Patch;
	struct Data Base;
	struct Data WriteFile;

	thisFile = GetAllOffsets(patchEXEFileName);
	thisFile.Data.fileptr = safe_fopen(patchEXEFileName, "rb");

	//BASE FILE
	if(INIGetBool("split:ifoutputbase") == 1)
	{
		printf("Length base file is 0x%X\n", thisFile.lenBase);
		Base.data = GetFile(thisFile.Data.fileptr, thisFile.baseOffset, thisFile.lenBase);
		Base.length = thisFile.lenBase;

		WriteFile.filename = INIGetString("split:outputbase");
		CreateNewDirectoryIterate(WriteFile.filename);
		WriteFile.fileptr = safe_fopen(WriteFile.filename, "wb");
		fwrite(Base.data, Base.length, 1, WriteFile.fileptr);
		fclose(WriteFile.fileptr);
		printf("%s created\n", WriteFile.filename);
		free(Base.data);

	}

	//PATCH FILE
	if(INIGetBool("split:ifoutputpatch") == 1)
	{
		printf("Length patch file is 0x%X\n", thisFile.lenPatch);
		Patch.data = GetFile(thisFile.Data.fileptr, thisFile.patchOffset, thisFile.lenPatch);
		Patch.length = thisFile.lenPatch;

		WriteFile.filename = INIGetString("split:outputpatch");
		CreateNewDirectoryIterate(WriteFile.filename);
		WriteFile.fileptr = safe_fopen(WriteFile.filename, "wb");
		fwrite(Patch.data, Patch.length, 1, WriteFile.fileptr);
		fclose(WriteFile.fileptr);
		printf("%s created\n", WriteFile.filename);

		free(Patch.data);
	}

	//NOTICE FILE
	if(INIGetBool("split:ifoutputnotice") == 1)
	{
		printf("Length notice file is 0x%X\n", thisFile.lenNotice);
		Notice.data = GetFile(thisFile.Data.fileptr, thisFile.noticeOffset, thisFile.lenNotice);
		Notice.length = thisFile.lenNotice;

		WriteFile.filename = INIGetString("split:outputnotice");
		CreateNewDirectoryIterate(WriteFile.filename);
		WriteFile.fileptr = safe_fopen(WriteFile.filename, "wb");
		fwrite(Notice.data, Notice.length, 1, WriteFile.fileptr);
		fclose(WriteFile.fileptr);
		printf("%s created.\n", WriteFile.filename);
		free(WriteFile.filename);

		free(Notice.data);
	}

	fclose(thisFile.Data.fileptr);

	printf("Patch file fully split.\n");
}


int IsPatchEXEFile(char* exeFile)
{
	struct Data PatchEXE;
	int checksum;
	const int patchCRC = 0xF2F7FBF3;

	//Get the final four bytes
	PatchEXE.fileptr = safe_fopen(exeFile, "rb");
	PatchEXE.length = GetEOF(PatchEXE.fileptr);
	fseek(PatchEXE.fileptr, PatchEXE.length - 4, SEEK_SET);
	checksum = rS32(PatchEXE.fileptr);
	fclose(PatchEXE.fileptr);

	if(patchCRC == checksum)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

struct PatchFile GetAllOffsets(char* patchFile)
{
	struct PatchFile PatchEXE;

	if(IsPatchEXEFile(patchFile) == 1)
	{
		PatchEXE.Data.fileptr = safe_fopen(patchFile, "rb");
		PatchEXE.eof = GetEOF(PatchEXE.Data.fileptr);
		PatchEXE.lenBase = GetLenBase(PatchEXE.Data.fileptr);
		PatchEXE.lenPatch = GetLenPatch(PatchEXE.Data.fileptr);
		PatchEXE.lenNotice = GetLenNotice(PatchEXE.Data.fileptr);

		PatchEXE.baseOffset = 0;
		PatchEXE.patchOffset = PatchEXE.lenBase;
		PatchEXE.noticeOffset = PatchEXE.lenBase + PatchEXE.lenPatch;

		fclose(PatchEXE.Data.fileptr);
	}
	else
	{
		printf("%s is not a valid Nexon patch executable file.\n", patchFile);
		MessageBox(NULL, "Not a valid Nexon patch executable file.\n", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}

	return PatchEXE;
}

int GetLenBase(FILE* patchFile)
{
	int eof;
	int lenPatch;
	int lenNotice;
	int lenBase;
	int remember;
	const int sizeFooter = 12;

	remember = ftell(patchFile);

	eof = GetEOF(patchFile);
	lenPatch = GetLenPatch(patchFile);
	lenNotice = GetLenNotice(patchFile);
	lenBase = eof - lenPatch - lenNotice - sizeFooter;

	fseek(patchFile, remember, SEEK_SET);

	return lenBase;
	
}

int GetLenPatch(FILE* patchFile)
{
	int eof;
	int lenFile;
	int remember;

	remember = ftell(patchFile);

	eof = GetEOF(patchFile);
	fseek(patchFile, eof - 12, SEEK_SET);
	lenFile = rS32(patchFile);

	fseek(patchFile, remember, SEEK_SET);

	return lenFile;
}

int GetLenNotice(FILE* patchFile)
{
	int eof;
	int lenFile;
	int remember;

	remember = ftell(patchFile);

	eof = GetEOF(patchFile);
	fseek(patchFile, eof - 8, SEEK_SET);
	lenFile = rS32(patchFile);

	fseek(patchFile, remember, SEEK_SET);

	return lenFile;
}