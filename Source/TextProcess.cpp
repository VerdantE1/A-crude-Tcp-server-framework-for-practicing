#include "TextProcess.h"
#include <string.h>
void DeleteLChar(char* str, const char chr)
{
	if (str == 0) return;
	if (strlen(str) == 0) return;

	char strTemp[strlen(str) + 1];
	int iTemp = 0;
	memset(strTemp, 0, sizeof(strTemp));
	strcpy(strTemp, str);
	while (strTemp[iTemp] == chr)  iTemp++;

	memset(str, 0, strlen(str) + 1);
	strcpy(str, strTemp + iTemp);
	return;
}

void DeleteRChar(char* str, const char chr)
{
	if (str == 0) return;
	if (strlen(str) == 0) return;

	int istrlen = strlen(str);
	while (istrlen > 0)
	{
		if (str[istrlen - 1] != chr) break;
		str[istrlen - 1] = 0;
		istrlen--;
	}
}

void DeleteLRChar(char* str, const char chr)
{
	DeleteLChar(str, chr);
	DeleteRChar(str, chr);
}
