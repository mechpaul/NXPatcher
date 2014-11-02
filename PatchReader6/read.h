void PatchReaderMain(char* patchFileName);
void VerifyHeader(unsigned char* PatchFile);
void VerifyChecksum(unsigned char* patchFileBytes, int eof);
void ParsePatchFile(unsigned char* zlibBlock, long unpackedLenZlib, char* patchSection);
void AddToLists(char* fileName, char* patchSection);
void ApplyLists(char* patchSection);
unsigned char* ParseType00(unsigned char* zlibBlock, char* fileName, char* outputfolder);
unsigned char* ParseType01(unsigned char* zlibBlock, char* fileName, char* outputfolder);
unsigned char* ParseType01FP(unsigned char* zlibBlock, char* fileName, char* outputfolder);
unsigned char* ParseType01VFP(unsigned char* zlibBlock, char* fileName, char* outputfolder);
void managed_fwrite(FILE* writeFile, unsigned char* buffer, unsigned int* bufferPos, unsigned char* memory, unsigned int memoryLength, unsigned int bufferlength);
void ParseType02(char* fileName, char* outputfolder, char* patchSection);

char GetString(unsigned char** zlibBlock, char fileName[]);
char GetCharacter(unsigned char** zlibBlock);
unsigned int GetInt(unsigned char** zlibBlock);
unsigned int FindNewFileSize(unsigned char* zlibBlock);