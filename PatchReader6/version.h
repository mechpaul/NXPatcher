int VersionMain(char* wzFile, int ifprint);
int GetDecryptionBytes(FILE* VersionCheck, unsigned int *decryptionBytes, unsigned int *decryptionOffset, unsigned int *fileStart);
unsigned short GetWzVersion(FILE* VersionCheck);
unsigned short BruteForceVersion(FILE* VersionCheck, struct WzVersion, int folderType);
unsigned int CalculateHashKey(unsigned short version, unsigned short encryptedVersion);
unsigned int CalculateOffset(unsigned int decryptionBytes, unsigned int decryptionOffset, unsigned int fileStart, unsigned int hashKey);