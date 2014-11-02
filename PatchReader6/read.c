#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "FileIO.h"
#include "Checksum.h"
#include "INIManager.h"
#include "Struct.h"

#include "read.h"

void PatchReaderMain(char* patchFileName)
{
	struct Data PatchFile;
	struct Data WzPatch;
	const int sizeHeader = 16;

	//Read the file and perform runtime checks to verify that this is indeed a patch file
	printf("Read patch file into memory...\n");
	WzPatch.length = GetEOFFile(patchFileName);
	WzPatch.data = ReadTheFile(patchFileName);

	//Verify header and checksum
	VerifyHeader(WzPatch.data);
	VerifyChecksum(WzPatch.data, WzPatch.length - sizeHeader);

	//All of that checks out. Let's get the zlib block.
	printf("Unpacking zlib block...\n");
	PatchFile.data = UnpackZlibBlock(WzPatch.data + sizeHeader, WzPatch.length - sizeHeader, &PatchFile.length);
	printf("Zlib block unpacked.\n");
	free(WzPatch.data);

	//Parse the patch file
	printf("Begin parsing patch file...\n");
	ParsePatchFile(PatchFile.data, PatchFile.length, "read");
	free(PatchFile.data);
	printf("Patch file complete.\n");
}

void VerifyHeader(unsigned char* patchFile)
{
 	char Header[13] = {0x57, 0x7A, 0x50, 0x61, 0x74, 0x63, 0x68, 0x1A, 0x02};
	char verifyHeader[13] = {0};
	const int sizeHeader = 12;

	printf("Verifying patch header...\n");
	memcpy(verifyHeader, patchFile, sizeHeader);

	if(strcmp(Header, verifyHeader) == 0)
	{
		printf("Patch header OK.\n");
	}
	else
	{
		printf("Header incorrect.\n");
		MessageBox(NULL, "Not a valid Nexon patch file.\nSolution - Are you sure you specified the right file?\n", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}
}
void VerifyChecksum(unsigned char* patchFileBytes, int lenZlib)
{
	struct Checksum PatchChecksum;
	int sbox[SIZESBOX];
	const int sizeHeader = 16;

	Generatesbox(sbox);

	printf("Verifying patch checksum...\n");
	memcpy(&PatchChecksum.checksum, patchFileBytes + 12, 4);
	PatchChecksum.verify = CalculateChecksum(patchFileBytes + sizeHeader, sbox, lenZlib, 0);
	CompareChecksums(PatchChecksum.checksum, PatchChecksum.verify);
	printf("Patch checksum OK.\n");
}

void ParsePatchFile(unsigned char* zlibBlock, long unpackedLenZlib, char* patchSection)
{
	unsigned char* zlibBlockPtr = zlibBlock;
	char fileName[FILENAME_MAX];
	char switchByte;
	struct INIInt ifusefastpatch;
	struct INIString outputfolder;

	//INI Lookups
	//See if the user wants to use a faster patching mechanism
	ifusefastpatch.lookup = str_join(patchSection, ":", "usefastpatchtype");
	ifusefastpatch.data = INIGetBool(ifusefastpatch.lookup);
	free(ifusefastpatch.lookup);

	//Look for where we need to output files
	outputfolder.lookup = str_join(patchSection, ":", "outputfolder");
	outputfolder.string = INIGetString(outputfolder.lookup);
	free(outputfolder.lookup);

	CreateNewDirectoryIterate(outputfolder.string);

	do{
		switchByte = GetString(&zlibBlock, fileName);
		switch(switchByte)
		{
		case 0x00:
			printf("%s - Create File/Folder\n", fileName);
			zlibBlock = ParseType00(zlibBlock, fileName, outputfolder.string);
			break;
		case 0x01:
			printf("%s - Rebuild file\n", fileName);
			if(ifusefastpatch.data == 1)
			{
				zlibBlock = ParseType01FP(zlibBlock, fileName, outputfolder.string);
			}
			else if(ifusefastpatch.data == 2)
			{
				zlibBlock = ParseType01VFP(zlibBlock, fileName, outputfolder.string);
			}
			else
			{
				zlibBlock = ParseType01(zlibBlock, fileName, outputfolder.string);
			}			
			break;
		case 0x02:
			printf("%s - Delete File/Folder\n", fileName);
			ParseType02(fileName, outputfolder.string, patchSection);
			break;
		default:
			printf("Unknown switchbyte %d?\n", switchByte);
			MessageBox(NULL, "Unknown switchbyte?", "Error", MB_ICONEXCLAMATION | MB_OK);
			exit(EXIT_FAILURE);
		}
		AddToLists(fileName, patchSection);
	}while((zlibBlock - zlibBlockPtr) < unpackedLenZlib);
	//zlibBlockPtr contains the address of the first byte of the zlib block
	//Using this is similar to "CurPos - BasePos < eof" so the program does not
	//go out of bounds

	ApplyLists(patchSection);
}

void ApplyLists(char* patchSection)
{
	struct INIInt backup;
	struct INIInt autoapply;
	struct INIString outputfolder;
	struct INIString backupdir;
	char outputlocation[FILENAME_MAX] = {0};
	char backuplocation[FILENAME_MAX] = {0};
	char applylocation[FILENAME_MAX] = {0};
	char token[FILENAME_MAX] = {0};
	FILE* inputFile;

	backup.lookup = str_join(patchSection, ":", "ifbackup");
	backup.data = INIGetBool(backup.lookup);
	free(backup.lookup);

	autoapply.lookup = str_join(patchSection, ":", "ifautoapply");
	autoapply.data = INIGetBool(autoapply.lookup);
	free(autoapply.lookup);

	backupdir.lookup = str_join(patchSection, ":", "backupdir");
	backupdir.string = INIGetString(backupdir.lookup);
	free(backupdir.lookup);

	outputfolder.lookup = str_join(patchSection, ":", "outputfolder");
	outputfolder.string = INIGetString(outputfolder.lookup);
	free(outputfolder.lookup);

	strcat(outputlocation, outputfolder.string);
	strcat(outputlocation, "/bufiles.txt");

	if(FileExists(outputlocation) == 1 && backup.data == 1)
	{
		//backup files
		printf("Backing up files:\n", backuplocation);
		inputFile = fopen(outputlocation, "r");
		fgetstr(token, FILENAME_MAX, inputFile);

		while(!feof(inputFile))
		{
			if(FileExists(token))
			{
				printf("  %s\n", token);
				strcpy(backuplocation, backupdir.string);
				strcat(backuplocation, "\\");
				strcat(backuplocation, token);

				CreateNewDirectoryIterate(backuplocation);

				MoveFile(token, backuplocation);
			}
			fgetstr(token, FILENAME_MAX, inputFile);
		}
		fclose(inputFile);
		remove(outputlocation);
	}

	outputlocation[0] = '\0';

	strcat(outputlocation, outputfolder.string);
	strcat(outputlocation, "/applyfiles.txt");

	if(FileExists(outputlocation) == 1 && autoapply.data == 1)
	{
		//apply files
		printf("Applying files to Maplestory directory:\n");
		inputFile = fopen(outputlocation, "r");
		fgetstr(token, FILENAME_MAX, inputFile);

		while(!feof(inputFile))
		{
			printf("  %s\n", token);
			CreateNewDirectoryIterate(token);

			strcpy(applylocation, outputfolder.string);
			strcat(applylocation, "\\");
			strcat(applylocation, token);

			MoveFile(applylocation, token);

			fgetstr(token, FILENAME_MAX, inputFile);
		}
		fclose(inputFile);
		remove(outputlocation);
		RemDir(outputfolder.string);
	}
}

void AddToLists(char* fileName, char* patchSection)
{
	struct INIInt backup;
	struct INIInt autoapply;
	struct INIString outputfolder;
	struct Data BackupArchive;
	char outputlocation[256] = {0};

	backup.lookup = str_join(patchSection, ":", "ifbackup");
	backup.data = INIGetBool(backup.lookup);
	free(backup.lookup);

	autoapply.lookup = str_join(patchSection, ":", "ifautoapply");
	autoapply.data = INIGetBool(autoapply.lookup);
	free(autoapply.lookup);

	outputfolder.lookup = str_join(patchSection, ":", "outputfolder");
	outputfolder.string = INIGetString(outputfolder.lookup);
	free(outputfolder.lookup);

	if(backup.data == 1)
	{
		strcat(outputlocation, outputfolder.string);
		strcat(outputlocation, "/bufiles.txt");

		BackupArchive.fileptr = safe_fopen(outputlocation, "ab");
		fwrite(fileName, strlen(fileName), 1, BackupArchive.fileptr);
		WriteCarriageReturn(BackupArchive.fileptr);
		fclose(BackupArchive.fileptr);

		outputlocation[0] = '\0';
	}
	if(autoapply.data == 1)
	{
		strcat(outputlocation, outputfolder.string);
		strcat(outputlocation, "/applyfiles.txt");

		BackupArchive.fileptr = safe_fopen(outputlocation, "ab");
		fwrite(fileName, strlen(fileName), 1, BackupArchive.fileptr);
		WriteCarriageReturn(BackupArchive.fileptr);
		fclose(BackupArchive.fileptr);
	}
}

unsigned char* ParseType00(unsigned char* zlibBlock, char* fileName, char* outputfolder)
{
	long lengthOfBlock;
	struct Checksum New;
	char newFileName[FILENAME_MAX] = {0};
	FILE* wzFile;
	int sbox[SIZESBOX];

	Generatesbox(sbox);
	
	//New files might be buried within another directory.
	//Best to create those folders now
	//Example --> "HShield/aspinet.dll" would create the folder "HShield/"
	//Example --> "HShield1/HShield2" would create the folder "HShield1", then create the folder "HShield1/HShield2"
	strcat(newFileName, outputfolder);
	strcat(newFileName, "/");
	strcat(newFileName, fileName);
	CreateNewDirectoryIterate(newFileName);

	if((fileName[strlen(fileName)-1] != '\\') && (fileName[strlen(fileName)-1] != '/'))
	{
		//New file
		//Read 0x00 header bytes
		lengthOfBlock = GetInt(&zlibBlock);
		New.checksum = GetInt(&zlibBlock);

		//Verify the checksum of the file written
		printf("  Verifying checksum...\n");
		New.verify = CalculateChecksum(zlibBlock, sbox, lengthOfBlock, 0);
		CompareChecksums(New.checksum, New.verify);
		printf("  Checksum OK - 0x%08X, 0x%08X\n", New.checksum, New.verify);

		//Write "lengthOfBlock" number of bytes from zlibBlock to the file
		wzFile = safe_fopen(newFileName, "wb");
		fwrite(zlibBlock, lengthOfBlock, 1, wzFile);
		zlibBlock += lengthOfBlock;
		fclose(wzFile);
		printf("  File complete.\n");
	}

	return zlibBlock;
}

unsigned char* ParseType01VFP(unsigned char* zlibBlock, char* fileName, char* outputfolder)
{
	//Used for checksums and files
	struct Checksum Old = {0, 0};
	struct Checksum New = {0, 0};

	//Buffered writing
	unsigned char* bufferPos = NULL;

	//Used for parsing the patch file
	int lengthOfBlock; 
	int beginOffset;
	unsigned int Command;
	struct Data OldFile = {NULL, 0, NULL, NULL};
	struct Data NewFile = {NULL, 0, NULL, NULL};

	int sbox[SIZESBOX];

	Generatesbox(sbox);

	printf("  Reading the entire file into memory...\n");
	OldFile.data = ReadTheFile(fileName);
	OldFile.length = GetEOFFile(fileName);

	//Tidy up the checksums portion
	Old.checksum = GetInt(&zlibBlock);
	New.checksum = GetInt(&zlibBlock);
	printf("  Calculating checksum...\n");
	Old.verify = CalculateChecksum(OldFile.data, sbox, OldFile.length, 0);
	CompareChecksums(Old.checksum, Old.verify);
	printf("  Pre-update checksum OK\n", Old.checksum, Old.verify);

	//Determine size of new buffer
	NewFile.length = FindNewFileSize(zlibBlock);
	printf("  File size - %0.1f --> %0.1f mb\n", (float)OldFile.length / 1000000, (float)NewFile.length / 1000000);
	NewFile.data = safe_malloc(NewFile.length);
	bufferPos = NewFile.data;

	//Open up the new files
	NewFile.filename = (char*) safe_malloc(FILENAME_MAX);
	NewFile.filename[0] = '\0';
	strcat(NewFile.filename, outputfolder);
	strcat(NewFile.filename, "/");
	strcat(NewFile.filename, fileName);
	CreateNewDirectoryIterate(NewFile.filename);
	NewFile.fileptr = safe_fopen(NewFile.filename, "wb");

	//Begin parsing patch data
	printf("  Parsing updates...\n");
	for(Command = GetInt(&zlibBlock); Command != 0x00000000; Command = GetInt(&zlibBlock))
	{
		switch(Command >> 0x1C)
		{
		case 0x08:
			//0x08 block
			//<-- MSB            LSB -->
			//<--4 bits--> <--28 bits-->
			//<Block Type> <Length Bits>

			//Setup
			lengthOfBlock = Command & 0x0FFFFFFF;

			//Get

			//Commit
			memcpy(bufferPos, zlibBlock, lengthOfBlock);
			bufferPos += lengthOfBlock;
			zlibBlock += lengthOfBlock;
			break;
		case 0x0C:
			//0x0C block
			//<-- MSB                            LSB -->
			//<--4 bits--> <--20 bits--> <-- 8 bits  -->
			//<Block Type> <Length Bits> <Repeated Byte>
			//ex. 0xC00005FF
			//Block Type = 0xC
			//Length = 0x5
			//Repeated byte = 0xFF
			//Output = FF FF FF FF FF
			//This block type is essentially RLE (Run Length Encoding)

			//Setup
			lengthOfBlock = (Command >> 8) & 0xFFFFF;

			//Get

			//Commit
			memset(bufferPos, Command & 0xFF, lengthOfBlock);
			bufferPos += lengthOfBlock;
			break;
		default:
			//Other block
			//<-- MSB             LSB -->
			//<--32 bits--><--32 bits -->
			//<Length Bits><Begin Offset>
			//1. fseek to beginOffset in baseFile
			//2. Read "lengthOfBlock" number of bytes from baseFile
			//3. Write those bytes to writeFile
			//4. Update checksum with the stream read

			//Setup
			beginOffset = GetInt(&zlibBlock);

			//Get

			//Commit
			memcpy(bufferPos, OldFile.data + beginOffset, Command);
			bufferPos += Command;
			break;
		}
	}

	if(bufferPos - NewFile.data != NewFile.length)
	{
		printf("Massive buffer error in VFP. Program terminating.\nbufferPos = %d\nNewFile.length = %d\n", bufferPos - NewFile.data, NewFile.length);
		exit(EXIT_FAILURE);
	}

	//Tidy up checksum portion
	New.verify = CalculateChecksum(NewFile.data, sbox, NewFile.length, 0);
	CompareChecksums(New.checksum, New.verify);
	printf("  Post-update checksum OK\n");
	fwrite(NewFile.data, 1, NewFile.length, NewFile.fileptr);
	printf("  File complete.\n");

	fclose(NewFile.fileptr);
	free(OldFile.data);
	free(NewFile.filename);
	free(NewFile.data);

	return zlibBlock;
}

unsigned int FindNewFileSize(unsigned char* zlibBlock)
{
	unsigned int Command;
	unsigned int length;
	unsigned int returnSize = 0;

	for(Command = GetInt(&zlibBlock); Command != 0x00000000; Command = GetInt(&zlibBlock))
	{
		switch(Command >> 0x1C)
		{
		case 0x08:
			length = Command & 0x0FFFFFFF;
			returnSize += length;
			zlibBlock += length;
			break;
		case 0x0C:
			returnSize += (Command & 0x0FFFFF00) >> 8;
			break;
		default:
			returnSize += Command;
			zlibBlock += 4; //Skip beginning offset
			break;
		}
	}
	return returnSize;
}

unsigned char* ParseType01FP(unsigned char* zlibBlock, char* fileName, char* outputfolder)
{
	//Used for checksums and files
	struct Checksum Old = {0, 0};
	struct Checksum New = {0, 0};
	char newFileName[FILENAME_MAX] = {0};
	FILE* writeFile;

	//Buffered writing
	unsigned char* buffer = safe_malloc(5000000);
	unsigned int bufferPos = 0;

	//Used for parsing the patch file
	int lengthOfBlock; 
	int beginOffset;
	unsigned int Command;
	unsigned int interpretedCommand;
	struct Data Repeat = {NULL, 0, NULL, NULL}; //0x0C block type
	struct Data OldFile;

	int sbox[SIZESBOX];

	Generatesbox(sbox);

	printf("  Reading the entire file...\n");
	OldFile.data = ReadTheFile(fileName);
	OldFile.length = GetEOFFile(fileName);

	//Tidy up the checksums portion
	Old.checksum = GetInt(&zlibBlock);
	New.checksum = GetInt(&zlibBlock);
	printf("  Calculating checksum...\n");
	Old.verify = CalculateChecksum(OldFile.data, sbox, OldFile.length, 0);
	CompareChecksums(Old.checksum, Old.verify);
	printf("  Pre-update checksum OK\n", Old.checksum, Old.verify);

	//Open up the new files
	strcat(newFileName, outputfolder);
	strcat(newFileName, "/");
	strcat(newFileName, fileName);
	CreateNewDirectoryIterate(newFileName);
	writeFile = safe_fopen(newFileName, "wb");
	
	//Begin parsing patch data
	printf("  Parsing updates...\n");
	for(Command = GetInt(&zlibBlock); Command != 0x00000000; Command = GetInt(&zlibBlock))
	{
		interpretedCommand = Command >> 0x1C;
		switch(interpretedCommand)
		{
		case 0x08:
			//0x08 block
			//<-- MSB            LSB -->
			//<--4 bits--> <--28 bits-->
			//<Block Type> <Length Bits>

			//Setup
			lengthOfBlock = Command & 0xFFFFFFF;

			//Get

			//Commit
			New.verify = CalculateChecksum(zlibBlock, sbox, lengthOfBlock, New.verify);
			managed_fwrite(writeFile, buffer, &bufferPos, zlibBlock, lengthOfBlock, 5000000);
			zlibBlock += lengthOfBlock;
			break;
		case 0x0C:
			//0x0C block
			//<-- MSB                            LSB -->
			//<--4 bits--> <--20 bits--> <-- 8 bits  -->
			//<Block Type> <Length Bits> <Repeated Byte>
			//ex. 0xC00005FF
			//Block Type = 0xC
			//Length = 0x5
			//Repeated byte = 0xFF
			//Output = FF FF FF FF FF
			//This block type is essentially RLE (Run Length Encoding)

			//Setup
			lengthOfBlock = (Command >> 8) & 0xFFFFF;
			
			//Make sure cache has enough space to hold the repeated bytes
			if(lengthOfBlock > Repeat.length)
			{
				Repeat.data = safe_realloc(Repeat.data, lengthOfBlock);
				Repeat.length = lengthOfBlock;
			}

			//Get
			memset(Repeat.data, Command & 0xFF, lengthOfBlock);

			//Commit
			New.verify = CalculateChecksum(Repeat.data, sbox, lengthOfBlock, New.verify);
			managed_fwrite(writeFile, buffer, &bufferPos, Repeat.data, lengthOfBlock, 5000000);
			break;
		default:
			//Other block
			//<-- MSB             LSB -->
			//<--32 bits--><--32 bits -->
			//<Length Bits><Begin Offset>
			//1. fseek to beginOffset in baseFile
			//2. Read "lengthOfBlock" number of bytes from baseFile
			//3. Write those bytes to writeFile
			//4. Update checksum with the stream read

			//Setup
			lengthOfBlock = Command;
			beginOffset = GetInt(&zlibBlock);

			//Get

			//Commit
			New.verify = CalculateChecksum(OldFile.data + beginOffset, sbox, lengthOfBlock, New.verify);
			managed_fwrite(writeFile, buffer, &bufferPos, OldFile.data + beginOffset, lengthOfBlock, 5000000);
			break;
		}
	}

	//Flush the buffer
	if(bufferPos > 0)
	{
		fwrite(buffer, bufferPos, 1, writeFile);
		bufferPos = 0;
	}

	CompareChecksums(New.checksum, New.verify);
	printf("  Post-update checksum OK\n", New.checksum, New.verify);
	printf("  File complete.\n");

	fclose(writeFile);
	free(OldFile.data);
	free(Repeat.data);
	free(buffer);

	return zlibBlock;
}

unsigned char* ParseType01(unsigned char* zlibBlock, char* fileName, char* outputfolder)
{
	//Used for checksums and files
	struct Checksum Old = {0, 0};
	struct Checksum New = {0, 0};
	char newFileName[FILENAME_MAX] = {0};
	FILE* writeFile;

	//Buffered writing
	unsigned char* buffer = safe_malloc(5000000);
	unsigned int bufferPos = 0;

	//Used for parsing the patch file
	int lengthOfBlock; 
	int beginOffset;
	unsigned int Command;
	struct Data Repeat = {NULL, 0, NULL, NULL}; //0x0C block type
	struct Data OldFile = {NULL, 0, NULL, NULL};

	int sbox[SIZESBOX];

	Generatesbox(sbox);

	OldFile.fileptr = safe_fopen(fileName, "rb");

	//Tidy up the checksums portion
	Old.checksum = GetInt(&zlibBlock);
	New.checksum = GetInt(&zlibBlock);
	printf("  Calculating checksum...\n");
	Old.verify = CalculateChecksumFile(fileName);
	CompareChecksums(Old.checksum, Old.verify);
	printf("  Pre-update checksum OK\n", Old.checksum, Old.verify);

	//Open up the new files
	strcat(newFileName, outputfolder);
	strcat(newFileName, "/");
	strcat(newFileName, fileName);
	CreateNewDirectoryIterate(newFileName);
	writeFile = safe_fopen(newFileName, "wb");
	
	//Begin parsing patch data
	printf("  Parsing updates...\n");
	for(Command = GetInt(&zlibBlock); Command != 0x00000000; Command = GetInt(&zlibBlock))
	{
		switch(Command >> 0x1C)
		{
		case 0x08:
			//0x08 block
			//<-- MSB            LSB -->
			//<--4 bits--> <--28 bits-->
			//<Block Type> <Length Bits>

			//Setup
			lengthOfBlock = Command & 0xFFFFFFF;

			//Get

			//Commit
			New.verify = CalculateChecksum(zlibBlock, sbox, lengthOfBlock, New.verify);
			managed_fwrite(writeFile, buffer, &bufferPos, zlibBlock, lengthOfBlock, 5000000);
			zlibBlock += lengthOfBlock;
			break;
		case 0x0C:
			//0x0C block
			//<-- MSB                            LSB -->
			//<--4 bits--> <--20 bits--> <-- 8 bits  -->
			//<Block Type> <Length Bits> <Repeated Byte>
			//ex. 0xC00005FF
			//Block Type = 0xC
			//Length = 0x5
			//Repeated byte = 0xFF
			//Output = FF FF FF FF FF
			//This block type is essentially run length encoding

			//Setup
			lengthOfBlock = (Command >> 8) & 0xFFFFF;
			
			//Make sure cache has enough space to hold the repeated bytes
			if(lengthOfBlock > Repeat.length)
			{
				Repeat.data = safe_realloc(Repeat.data, lengthOfBlock);
				Repeat.length = lengthOfBlock;
			}

			//Get
			memset(Repeat.data, Command & 0xFF, lengthOfBlock);

			//Commit
			New.verify = CalculateChecksum(Repeat.data, sbox, lengthOfBlock, New.verify);
			managed_fwrite(writeFile, buffer, &bufferPos, Repeat.data, lengthOfBlock, 5000000);
			break;
		default:
			//Other block
			//<-- MSB             LSB -->
			//<--32 bits--><--32 bits -->
			//<Length Bits><Begin Offset>
			//1. fseek to beginOffset in baseFile
			//2. Read "lengthOfBlock" number of bytes from baseFile
			//3. Write those bytes to writeFile
			//4. Update checksum with the stream read

			//Setup
			lengthOfBlock = Command;
			beginOffset = GetInt(&zlibBlock);

			//Get
			//Resize the buffer if needed
			if(lengthOfBlock > OldFile.length)
			{
				OldFile.data = safe_realloc(OldFile.data, lengthOfBlock);
				OldFile.length = lengthOfBlock;
			}
			fseek(OldFile.fileptr, beginOffset, SEEK_SET);
			fread(OldFile.data, lengthOfBlock, 1, OldFile.fileptr);

			//Commit
			New.verify = CalculateChecksum(OldFile.data, sbox, lengthOfBlock, New.verify);
			managed_fwrite(writeFile, buffer, &bufferPos, OldFile.data, lengthOfBlock, 5000000);
			break;
		}
	}

	//Flush the buffer
	if(bufferPos > 0)
	{
		fwrite(buffer, bufferPos, 1, writeFile);
		bufferPos = 0;
	}

	CompareChecksums(New.checksum, New.verify);
	printf("  Post-update checksum OK\n", New.checksum, New.verify);
	printf("  File complete.\n");

	free(buffer);
	free(OldFile.data);
	free(Repeat.data);
	fclose(writeFile);

	return zlibBlock;
}

__inline void managed_fwrite(FILE* writeFile, unsigned char* buffer, unsigned int* bufferPos, unsigned char* memory, unsigned int memoryLength, unsigned int bufferlength)
{
	if((*bufferPos + memoryLength) > bufferlength)
	{
		//Since the additional information given in this fwrite will cause a buffer overrun
		//Flush the buffer, then flush the information that would have been written to the buffer
		fwrite(buffer, *bufferPos, 1, writeFile);
		*bufferPos = 0;
		fwrite(memory, memoryLength, 1, writeFile);
	}
	else
	{
		//Append to the buffer
		memcpy(buffer + *bufferPos, memory, memoryLength);
		(*bufferPos) += memoryLength;
	}
}

void ParseType02(char* fileName, char* outputfolder, char* patchSection)
{
	char newFileName[FILENAME_MAX] = {0};
	struct INIInt ifdeltxt;
	struct INIString deltxt;
	FILE* writeFile;

	ifdeltxt.lookup = str_join(patchSection, ":", "ifdeltxt");
	ifdeltxt.data = INIGetBool(ifdeltxt.lookup);
	free(ifdeltxt.lookup);

	if(ifdeltxt.data == 1)
	{
		deltxt.lookup = str_join(patchSection, ":", "deltxt");
		deltxt.string = INIGetString(deltxt.lookup);
		free(deltxt.lookup);

		strcat(newFileName, outputfolder);
		strcat(newFileName, "/");
		strcat(newFileName, deltxt.string);
		CreateNewDirectoryIterate(newFileName);

		//Writes newFileName to the text file "deltxt"
		writeFile = safe_fopen(newFileName, "ab");
		fwrite(fileName, strlen(fileName), 1, writeFile);
		WriteCarriageReturn(writeFile);
		fclose(writeFile);
	}
}

char GetString(unsigned char** zlibBlock, char fileName[])
{
	char switchByte;
	int fileNamePos = 0;

	do{
		switchByte = *((char*) *zlibBlock);
		(*zlibBlock)++;
		switch(switchByte)
		{
		case 0x00:
		case 0x01:
		case 0x02:
			fileName[fileNamePos] = '\0';
			break;
		default:
			fileName[fileNamePos] = switchByte;
			fileNamePos++;
		}
		if(fileNamePos > FILENAME_MAX)
		{
			printf("File name has overflowed the buffer in GetString\n");
			MessageBox(NULL, "Buffer overflow in GetString", "Error", MB_ICONEXCLAMATION | MB_OK);
			exit(EXIT_FAILURE);
		}
	}while(switchByte > 2);

	return switchByte;
}

char GetCharacter(unsigned char** zlibBlock)
{
	char eachChar;

	eachChar = *((char*) *zlibBlock);
	(*zlibBlock)++;
	return eachChar;
}

unsigned int GetInt(unsigned char** zlibBlock)
{
	unsigned int eachInt;

	eachInt = *((unsigned int*) *zlibBlock);
	*zlibBlock += 4;
	return eachInt;
}