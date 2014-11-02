struct Data
{
	FILE* fileptr;
	int length;
	unsigned char* data;
	char* filename;
};

struct PatchFile
{
	struct Data Data;

	int eof;

	//Base file
	int baseOffset;
	int lenBase;

	//Patch file
	int patchOffset;
	int lenPatch;

	//Notice file
	int noticeOffset;
	int lenNotice;
};

struct WzVersion
{
	unsigned short encryptedVersion;
	unsigned int decryptionBytes;
	unsigned int decryptionOffset;
	unsigned int fileStart;
};