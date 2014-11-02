#include <stdlib.h>
#include <stdio.h>

#include "FileIO.h"
#include "Checksum.h"
#include "windows.h"

#define POLYNOMIAL 0x04C11DB7
#define TOPBIT 0x80000000
#define BLOCKSIZE 0xF0000

//Official PKZIP CRC32 table generation
/*
    Width: 32
	Poly: 0x04C11DB7
	Reflect-in: False
	XOR-in: 0x00000000
	Reflect-out: False
	XOR-out: 0x00000000
*/
void Generatesbox(int* sbox)
{
	int remain;
	int dividend;
	int bit;

	for(dividend = 0; dividend < SIZESBOX; dividend++)
	{
		remain = dividend << 24;
		for(bit = 0; bit < 8; bit++)
		{
			if(remain & TOPBIT)
			{
				remain = (remain << 1) ^ POLYNOMIAL;
			}
			else
			{
				remain = (remain << 1);
			}
		}
		sbox[dividend] = remain;
	}
}

//If two checksums don't match, throw an exception
void CompareChecksums(unsigned int checksum, unsigned int verifyChecksum)
{
	if(checksum != verifyChecksum)
	{
		printf("Checksums do not match\nExpected: 0x%08X\nCalculated: 0x%08X\n", checksum, verifyChecksum);
		MessageBox(NULL, "Checksums do not match\nSolutions\nAre you sure you downloaded the right patch file?\nHave you edited any of your WZ files?\n", "Error", MB_ICONEXCLAMATION | MB_OK);
		exit(EXIT_FAILURE);
	}
}

//This is the standard CRC32 implementation
//"rollingChecksum" is used so the caller can maintain the current checksum between function calls
unsigned int CalculateChecksum(unsigned char* eachBlock, int* sbox, long lengthOfBlock, unsigned int rollingChecksum)
{
	int IndexLookup;
	int blockPos;

	for(blockPos = 0; blockPos < lengthOfBlock; blockPos++)
	{
		IndexLookup = (rollingChecksum >> 0x18) ^ eachBlock[blockPos];
		rollingChecksum = (rollingChecksum << 0x08) ^ sbox[IndexLookup];
	}
	return rollingChecksum;
}

//Wrapper/convenience function for "CalculateChecksum".
//It will calculate the checksum of the file given by "fileName"
unsigned int CalculateChecksumFile(char* fileName)
{
	unsigned char* block;
	unsigned int checksum = 0;
	int sizeOfBlock;
	FILE* wzFile;
	long eof;
	int i;
	int sbox[SIZESBOX];

	Generatesbox(sbox);

	sizeOfBlock = BLOCKSIZE;
	block = safe_malloc(BLOCKSIZE);
	wzFile = safe_fopen(fileName, "rb");
	eof = GetEOF(wzFile);

	for(i = 0; i < eof; i += BLOCKSIZE)
	{
		//This if statement ensures that the program does not
		//go past feof or create out of bounds errors
		if(i + BLOCKSIZE > eof)
		{
			sizeOfBlock = eof - i;
		}
		fseek(wzFile, i, SEEK_SET);
		fread(block, sizeOfBlock, 1, wzFile);
		checksum = CalculateChecksum(block, sbox, sizeOfBlock, checksum);
	}
	fclose(wzFile);
	free(block);
	return checksum;
}
