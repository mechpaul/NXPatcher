#include <stdio.h>
#include <stdlib.h>

#include "FileIO.h"
#include "Struct.h"

#include "hijack.h"
#include "split.h"
#include "read.h"


/*
  Patcher EXE hijacking takes an already made Nexon pre-patcher, rips out the patch file,
  and then has NXPatcher execute the patch file itself. So that's why this module uses a mix
  of commands from both the "split" and "read" modules, since the pre-patcher is split apart
  and then the patch file is read.
*/
void HijackMain(char* exeFile)
{
	struct PatchFile PatchEXE; //The EXE file itself
	struct Data WzPatch;       //Represents the patch inside the EXE file
	const int sizeHeader = 16;

	//Here we use some of the "split" command to fetch the part of the EXE file
	//that represents the patch file.
	WzPatch.filename = exeFile;
	PatchEXE = GetAllOffsets(WzPatch.filename);
	PatchEXE.Data.fileptr = safe_fopen(WzPatch.filename, "rb");
	WzPatch.data = GetFile(PatchEXE.Data.fileptr, PatchEXE.patchOffset, PatchEXE.lenPatch);
	WzPatch.length = PatchEXE.lenPatch;
	fclose(PatchEXE.Data.fileptr);
	
	//Verify checksum & header
	VerifyHeader(WzPatch.data);
	VerifyChecksum(WzPatch.data, WzPatch.length - sizeHeader);

	//Unpack zlib block
	printf("Unpacking zlib block...\n");
	PatchEXE.Data.data = UnpackZlibBlock(WzPatch.data + sizeHeader, WzPatch.length - sizeHeader, &PatchEXE.Data.length);
	printf("Zlib block unpacked.\n");
	free(WzPatch.data);

	//Begin parsing
	printf("Begin parsing patch file...\n");
	ParsePatchFile(PatchEXE.Data.data, PatchEXE.Data.length, "hijack");			//Goes to read.c
	free(PatchEXE.Data.data);
	printf("Patch file complete.\n");
}