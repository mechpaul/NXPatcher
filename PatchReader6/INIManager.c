#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "FileIO.h"
#include "Struct.h"

#include "iniparser.h"
#include "inimanager.h"
#include "version.h"

#define DICT_NAME "NXPatcher.ini"
#define KEY_NOT_FOUND_STR "0xBAADF00D"
#define BEGIN_TOKEN "{"
#define END_TOKEN "}"

dictionary* ConfigINI;
char* INIFileName;

void INILoadManager(char* fileNameEntered)
{
	//If the INI isn't loaded yet, load it now.
	if(ConfigINI == NULL)
	{
		ConfigINI = iniparser_load(DICT_NAME);
		INIFileName = fileNameEntered;
		//If loading the INI file failed, throw an exception
		if(ConfigINI == NULL)
		{
			printf("INI file %s could not be read.\n", DICT_NAME);
			MessageBox(NULL, "Could not read the INI file", "ERROR", MB_ICONEXCLAMATION);
			exit(EXIT_FAILURE);
		}
	}
}

void INIDestroyManager(void)
{
	iniparser_freedict(ConfigINI);
	ConfigINI = NULL;
}

//Expected input:
//Char string --> "section_name:section_variable"
//This can be done by str_join (shown below)
//so... str_join("nxpatcher", ":", "version")
//Returns the result of key/value pair
int INIGetInt(char* intname)
{
	int fetchedValue;
	char* stringValue;

	stringValue = INIGetString(intname);
	stringValue = INIVariableReplace(stringValue);
	fetchedValue = atoi(stringValue);

	return fetchedValue;
}

int INIGetBool(char* boolname)
{
	int fetchedValue;
	char* stringValue;

	stringValue = INIGetString(boolname);
	stringValue = INIVariableReplace(stringValue);
	fetchedValue = atoi(stringValue);

	return fetchedValue;
}

char* INIGetString(char* stringname)
{
	char* fetchedValue;

	fetchedValue = iniparser_getstring(ConfigINI, stringname, KEY_NOT_FOUND_STR);

	if(strcmp(fetchedValue, KEY_NOT_FOUND_STR) == 0)
	{
		printf("Cannot find str key for %s in %s\n", stringname, DICT_NAME);
		exit(EXIT_FAILURE);
	}

	fetchedValue = INIVariableReplace(fetchedValue);

	return fetchedValue;
}

//This looks inside of a fetched string to see if there are other {key:value} pairs.
//If it has key/value pairs, then resolve these pairings until we have a standard string.
char* INIVariableReplace(char *str)
{
	char* replace;
	char* rebuildString;
	char* tokbegin;
	char* tokend;
	char* token;
	char* recursiveToken;
	int lenstr;
	int numPairs;
	int i;

	//standard tokens
	int version;
	char* nxpatcherDef;
	
	replace = (char *) safe_malloc(strlen(str) + 1);
	memcpy(replace, str, strlen(str) + 1);

	//Let's make sure this is a valid string:
	numPairs = INIVariableReplaceSyntax(replace);

	//NumPairs contains the amount of key/value pairs given in the string.
	//So, "{nxpatcher:version}/{nxpatcher:version_to}" would have two pair (two sets of brackets)
	//And, "{nxpatcher:version}" would just have one (one set of brackets)
	for(i = 0; i < numPairs; i++)
	{
		tokbegin = strstr(replace, BEGIN_TOKEN);
		tokend = strstr(replace, END_TOKEN);

		//Fetch the token from the string
		lenstr = tokend - tokbegin - 1;
		token = (char *) safe_malloc(lenstr + 1);
		strncpy(token, tokbegin + 1, lenstr);
		token[lenstr] = '\0';

		//Is this a user-defined token?
		recursiveToken = iniparser_getstring(ConfigINI, token, KEY_NOT_FOUND_STR);
		if(recursiveToken != NULL && strcmp(recursiveToken, KEY_NOT_FOUND_STR) != 0)
		{
			//Ah, this is a user-defined token!
			//So we've found the key/value pair here, let's search the key/value reference
			//and see if __that__ contains other references too.
			recursiveToken = INIVariableReplace(recursiveToken);

			//Rebuild the string
			lenstr = (tokbegin - replace) + strlen(recursiveToken) + (replace + strlen(replace) - tokend);
			rebuildString = calloc(lenstr, 1);
			strncat(rebuildString, replace, tokbegin - replace);
			strcat(rebuildString, recursiveToken);
			strcat(rebuildString, tokend + 1);
			free(replace);
			replace = rebuildString;
		}
		//Is this a program-defined macro?
		else if(strstr(token, "nxpatcher") != 0)
		{
			//This is the most efficient version of if-statements
			//Flipping these if-statements could cause version checking to happen twice
			//and the result would be wrong.

			//version_to gets the current version of base.wz and adds one to it
			if(strstr(token, "version_to") != 0)
			{
				version = VersionMain("base.wz", 0);
				version++;
				nxpatcherDef = calloc(10, 1);
				_itoa(version, nxpatcherDef, 10);
			}
			//version just gets the current version of base.wz
			else if(strstr(token, "version") != 0)
			{
				version = VersionMain("base.wz", 0);
				nxpatcherDef = calloc(10, 1);
				_itoa(version, nxpatcherDef, 10);
			}
			//Inputfileprefix gets everything before the first dot in the filename
			//thisfile.tar.gz --> thisfile
			//00087to00088.patch --> 00087to00088
			else if(strstr(token, "inputfileprefix") != 0)
			{
				nxpatcherDef = GetFilePrefix(INIFileName);
			}
			else
			{
				//User called the nxpatcher referential variable but
				//called a variable which is not defined
				printf("token %s is not defined in the INI file?\n", token);
				MessageBox(NULL, "A token was used improperly in the INI file.", "ERROR", MB_ICONEXCLAMATION | MB_OK);
				exit(EXIT_FAILURE);
			}
			
			//Rebuild the string
			//Now that the replacement variable string is fetched from the nxpatcher defined vars
			//replace the {key:value} pair with the fetched string.
			lenstr = (tokbegin - replace) + strlen(nxpatcherDef) + (replace + strlen(replace) - tokend);
			rebuildString = calloc(lenstr, 1);
			strncat(rebuildString, replace, tokbegin - replace);
			strncat(rebuildString, nxpatcherDef, strlen(nxpatcherDef));
			strcat(rebuildString, tokend + 1);
			free(replace);
			replace = rebuildString;
			free(nxpatcherDef);
		}
		else
		{
			//User used a key/value which is not defined elsewhere in the INI file. Print it out.
			printf("An undefined token '%s' is found in %s.\n", token, DICT_NAME);
			printf("--> %s\n\n", str);
			MessageBox(NULL, "There's an error in your INI file. Please see the command prompt for the error message.", "Error", MB_ICONEXCLAMATION | MB_OK);
			exit(EXIT_FAILURE);
		}
		free(token);
	}

	return replace;
}

//Verifies a fetched string from the INI file to determine if it follows good form
//Checks for:
//  Missing beginning brackets
//  Missing end brackets
//Returns the number of key/value pairs found within the string.
int INIVariableReplaceSyntax(char* replace)
{
	int numPairs;
	char* tokenBegin;
	char* tokenEnd;

	numPairs = 0;

	do{
		tokenBegin = strstr(replace, BEGIN_TOKEN);
		tokenEnd = strstr(replace, END_TOKEN);
		if(tokenBegin == NULL && tokenEnd == NULL)
		{
			//There are no tokens left. Send back the number of pairs found.
			return numPairs;
		}
		else if(tokenBegin != NULL && tokenEnd == NULL)
		{
			//There's a beginning brace but no end brace.
			//Throw error message
			printf("There's a beginning brace but no ending brace in %s.\n", DICT_NAME);
			printf("--> %s\n\n", replace);
			MessageBox(NULL, "There's an error in your INI file. Please see the command prompt for the error message.", "Error", MB_ICONEXCLAMATION | MB_OK);
			exit(EXIT_FAILURE);
		}
		else if(tokenBegin == NULL && tokenEnd != NULL)
		{
			//There's an end brace but no beginning brace.
			//Throw error message
			printf("There's an ending brace but no beginning brace in %s.\n", DICT_NAME);
			printf("--> %s\n\n", replace);
			MessageBox(NULL, "There's an error in your INI file. Please see the command prompt for the error message.", "Error", MB_ICONEXCLAMATION | MB_OK);
			exit(EXIT_FAILURE);
		}
		else
		{
			//Pair found.
			numPairs++;
			replace = tokenEnd + 1;
		}
	}while(strstr(replace, BEGIN_TOKEN) != 0 && strstr(replace, END_TOKEN) != 0);

	return numPairs;
}

char* str_join(char* beginning, char* middle, char* end)
{
	int lenstr;
	char* returnStr;

	lenstr = strlen(beginning) + strlen(middle) + strlen(end) + 1;
	returnStr = calloc(lenstr, 1);
	strcat(returnStr, beginning);
	strcat(returnStr, middle);
	strcat(returnStr, end);

	return returnStr;
}