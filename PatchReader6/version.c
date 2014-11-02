#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "FileIO.h"
#include "INIManager.h"
#include "Struct.h"

#include "version.h"

int VersionMain(char* wzFile, int ifprint)
{
	FILE* VersionCheck;
	struct WzVersion baseFile;
	unsigned short version;
	int ifallwzfilesbool;
	char* wzFiles[] = {"Base.wz", "Character.wz", "Etc.wz", "Item.wz", "Map.wz", "Mob.wz", "Morph.wz", "Npc.wz", "Quest.wz", "Reactor.wz",
		               "Skill.wz", "Sound.wz", "String.wz", "TamingMob.wz", "UI.wz"};
	char ifallwzfiles[] = "version:ifallwzfiles";
	int i;
	int folderType;

	ifallwzfilesbool = INIGetBool(ifallwzfiles);

	if(ifprint == 1)
	{
		if(ifallwzfilesbool == 1)
		{
			for(i = 0; i < 15; i++)
			{
				VersionCheck = safe_fopen(wzFiles[i], "rb");
				baseFile.encryptedVersion = GetWzVersion(VersionCheck);
				folderType = GetDecryptionBytes(VersionCheck, &baseFile.decryptionBytes, &baseFile.decryptionOffset, &baseFile.fileStart);
				version = BruteForceVersion(VersionCheck, baseFile, folderType);
				fclose(VersionCheck);
				printf("%s version is %d\n", wzFiles[i], version);
			}
		}
		else
		{
			VersionCheck = safe_fopen(wzFile, "rb");
			baseFile.encryptedVersion = GetWzVersion(VersionCheck);
			folderType = GetDecryptionBytes(VersionCheck, &baseFile.decryptionBytes, &baseFile.decryptionOffset, &baseFile.fileStart);
			version = BruteForceVersion(VersionCheck, baseFile, folderType);
			fclose(VersionCheck);
			printf("%s version is %d\n", wzFile, version);
		}
	}
	else
	{
		VersionCheck = safe_fopen(wzFile, "rb");
		baseFile.encryptedVersion = GetWzVersion(VersionCheck);
		folderType = GetDecryptionBytes(VersionCheck, &baseFile.decryptionBytes, &baseFile.decryptionOffset, &baseFile.fileStart);
		version = BruteForceVersion(VersionCheck, baseFile, folderType);
		fclose(VersionCheck);
	}

	return version;
}


unsigned short BruteForceVersion(FILE* VersionCheck, struct WzVersion baseFile, int folderType)
{
	unsigned short version;
	unsigned int hashKey;
	char byteCheck;
	unsigned int offset;
	unsigned int eof = GetEOF(VersionCheck);

	for(version = 0; version < 0xFFFF; version++)
	{
		hashKey = CalculateHashKey(version, baseFile.encryptedVersion);
		if(hashKey > 0)
		{
			offset = CalculateOffset(baseFile.decryptionBytes, baseFile.decryptionOffset, baseFile.fileStart, hashKey);
			if(offset < eof)
			{
				fseek(VersionCheck, offset, SEEK_SET);
				if(folderType == 0x03)
				{
					rPackedInt(VersionCheck);
				}
				byteCheck = (char) fgetc(VersionCheck);
				//0x73 = Beginning of a file in the wz dir structure
				//0x04 = beginning of a directory tree folder
				//0x03 = Beginning of a directory tree subfolder
				if((byteCheck == 0x73) || (byteCheck == 0x04))
				{
					return version;
				}
			}
		}
	}
	MessageBox(NULL, "Could not determine version.\nSolutions\n(1) Your WZ file is corrupt. Try redownloading Maplestory.\n(2) Bad WZ repacking. Try again.\n", "Error", MB_ICONEXCLAMATION | MB_OK);
	exit(EXIT_FAILURE);
}

unsigned int CalculateHashKey(unsigned short version, unsigned short encryptedVersion)
{
	char VersionStr[6];	
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
	unsigned char checkVersion;
	unsigned int hashKey;
	int i;

	hashKey = 0;
	_itoa(version, VersionStr, 10);
	for(i = 0; VersionStr[i] != '\0'; i++)
	{
		hashKey = (hashKey << 5) + VersionStr[i] + 1;
	}
	a = (hashKey >> 24) & 0xFF;
	b = (hashKey >> 16) & 0xFF;
	c = (hashKey >> 8 ) & 0xFF;
	d = (hashKey >> 0 ) & 0xFF;
	checkVersion = a ^ b ^ c ^ d ^ 0xFF;
	if(checkVersion == encryptedVersion)
	{
		return hashKey;
	}
	else
	{
		return 0;
	}
}

unsigned int CalculateOffset(unsigned int decryptionBytes, unsigned int decryptionOffset, unsigned int fileStart, unsigned int hashKey)
{
	unsigned int offset;

	offset = (decryptionOffset - fileStart) ^ 0xFFFFFFFF;
	offset = offset * hashKey;
	offset = offset - 0x581C3F6D;
	offset = _lrotl(offset, offset & 0x1F);
	offset = offset ^ decryptionBytes;
	offset = offset + (2 * fileStart);

	return offset;
}

int GetDecryptionBytes(FILE* VersionCheck, unsigned int *decryptionBytes, unsigned int *decryptionOffset, unsigned int *fileStart)
{
	unsigned short encryptedVersion;
	int numFiles;
	int fileType;
	int lengthFilename;
	int checksum;
	int length;
	char header[5] = "\0\0\0\0";

	fseek(VersionCheck, 0, SEEK_SET);
	fread(header, 4, 1, VersionCheck);

	if(strcmp(header, "PKG1") != 0)
	{
		MessageBox(NULL, "This is not a properly formatted WZ file. Header is incorrect.\nSolution - Are you sure you provided the name of a WZ file? (List.wz doesn't count)\n", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}

	fseek(VersionCheck, 12, SEEK_SET);
	*fileStart = rS32(VersionCheck);

	fseek(VersionCheck, *fileStart, SEEK_SET);
	encryptedVersion = GetWzVersion(VersionCheck);
	numFiles = rPackedInt(VersionCheck);
	fileType = getc(VersionCheck);
	lengthFilename = (unsigned char) (getc(VersionCheck) * -1);
	fseek(VersionCheck, lengthFilename, SEEK_CUR);
	checksum = rPackedInt(VersionCheck);
	length = rPackedInt(VersionCheck);

	*decryptionOffset = ftell(VersionCheck);
	*decryptionBytes = rS32(VersionCheck);

	return fileType;
}

unsigned short GetWzVersion(FILE* VersionCheck)
{	
	unsigned short returnedValue;
	unsigned int fileStart;

	fseek(VersionCheck, 12, SEEK_SET);
	fileStart = rS32(VersionCheck);
	fseek(VersionCheck, fileStart, SEEK_SET);
	fread(&returnedValue, 2, 1, VersionCheck);

	return returnedValue;
}