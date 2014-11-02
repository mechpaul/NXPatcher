void PatchSplitMain(char* writeFile);
int IsPatchEXEFile(char* patchFileName);
struct PatchFile GetAllOffsets(char* patchFile);
int GetLenBase(FILE* patchFile);
int GetLenPatch(FILE* patchFile);
int GetLenNotice(FILE* patchFile);