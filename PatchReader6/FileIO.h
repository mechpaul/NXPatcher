#include <zlib.h>

/* FILE READING */
long GetEOF(FILE* PatchFile);
long GetEOFFile(char* fileName);
long FileExists(char* fileName);
int rPackedInt(FILE* fileObj);
int rS32(FILE* fileObj);
unsigned char* ReadTheFile(char* fileName);
void WriteTheFile(char* fileName, unsigned char* toWrite, int lenFile);
char* SwitchSuffix(char* fileName, char* suffix);
char* GetFileSuffix(char* fileName);
char* GetFilePrefix(char* fileName);
unsigned char* GetFile(FILE* patchFile, int beginOffset, int lenFile);
/* END FILE READING */

/* ZLIB */
unsigned char* UnpackZlibBlock(unsigned char* zlibBlock, int lenZlib, int* sizeUnpackedZlib);
void VerifyZlibState(int zError, z_stream zlibStream);
/* END ZLIB */

/* PLATFORM DEPENDENT */
void CreateNewDirectory(char* fileName);
void CreateNewDirectoryIterate(char* dirName);
void WriteCarriageReturn(FILE* fileName);
char* GetCWD(void);
void RemDir(char* dirToRemove);
/* END PLATFORM DEPENDENT */

/* SAFE WRAPPERS */
unsigned char* safe_malloc(int size);
unsigned char* safe_realloc(unsigned char* buffer, int size);
FILE* safe_fopen(char* fileName, char* openType);
/* END SAFE WRAPPERS */

char* fgetstr(char* string, int n, FILE* stream);