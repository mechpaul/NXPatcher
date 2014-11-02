struct INIString
{
	char* lookup;
	char* string;
};

struct INIInt
{
	char* lookup;
	int data;
};

void INILoadManager(char* fileNameEntered);
void INIDestroyManager(void);
int INIGetInt(char* intname);
int INIGetBool(char* boolname);
char* INIGetString(char* stringname);
char* INIVariableReplace(char *str);
int INIVariableReplaceSyntax(char* replace);
char* str_join(char* beginning, char* middle, char* end);