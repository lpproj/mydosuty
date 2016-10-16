/*
  unmountfs: dismount filesystem on the drive
             for Win10 anniversary(lol) update.
  
  Copyright (C) 2016 sava

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


  (in short term: `under the ZLIB license') 

*/

#include <windows.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int GetCurrentDrive0(void)
{
	int drive0 = -1;
	DWORD buflen;
	LPTSTR buf = NULL;

	buflen = GetCurrentDirectory(0, NULL);
	if (buflen > 0 && (buflen / sizeof(TCHAR)) < INT_MAX - sizeof(TCHAR)) {
		buf = malloc((buflen + 1) * sizeof(TCHAR));
	}
	if (!buf) return -1;
	buf[0] = 0; /* just for a proof */
	buflen = GetCurrentDirectory(buflen + 1, buf);
	if (buflen >= 3 && buf[1] == TEXT(':') && buf[2] == TEXT('\\')) {
		if (buf[0] >= TEXT('A') && buf[0] <= TEXT('Z'))
			drive0 = (int)(buf[0] - TEXT('A'));
		else if (buf[0] >= TEXT('a') && buf[0] <= TEXT('z'))
			drive0 = (int)(buf[0] - TEXT('a'));
	}
	if (buf) free(buf);
	return drive0;
}

DWORD DevIo_NoParam(HANDLE hDev, DWORD dwCode)
{
	DWORD dwRC, dwPrevRC, dwDummy;
	BOOL rc;
	
	dwPrevRC = GetLastError();
	SetLastError(0);
	rc = DeviceIoControl(hDev, dwCode, NULL, 0, NULL, 0, &dwDummy, NULL);
	dwRC = GetLastError();
	if (!rc && dwRC == 0) dwRC = (DWORD)-1;
	if (dwRC == 0) SetLastError(dwPrevRC);

	return dwRC;
}


DWORD unmountfs(int drivenum0)
{
	DWORD dwRC = 0;
	DWORD dw_acc = GENERIC_READ /* | GENERIC_WRITE */ ;
	HANDLE h;
	TCHAR volpn[8];
	UINT uPrevErrMode;

	wsprintf(volpn, TEXT("\\\\.\\\\%c:"), drivenum0 + 'A');
	uPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	h = CreateFile(volpn, dw_acc, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
	if (h != INVALID_HANDLE_VALUE) {
		dwRC = DevIo_NoParam(h, FSCTL_DISMOUNT_VOLUME);
		CloseHandle(h);
	}
	else {
		dwRC = GetLastError();
	}
	SetErrorMode(uPrevErrMode);

	return dwRC;
}

int optF = 0;

int main(int argc, char *argv[])
{
	int do_unmount = 0;

	while(argc-- > 1) {
		char *s = *++argv;
		int drive0 = -1;
		
		if (strcmpi(s, "-f") == 0) {
			optF = 1;
			continue;
		}
		if (isalpha(*s) && s[1] == ':' && s[2] == '\0')
			drive0 = toupper(*s) - 'A';
		else if (s[0] == '.' && s[1] == '\0')
			drive0 = GetCurrentDrive0();
		if (drive0 >= 0) {
			DWORD dwRC;
			++do_unmount;
			printf("unmount filesystem on drive %c: ... ", 'A' + drive0); fflush(stdout);
			dwRC = unmountfs(drive0);
			if (dwRC == 0) printf("ok\n");
			else {
				printf("failure! (errcode=%lu)\n", (unsigned long)dwRC);
				if (!optF) break;
			}
		}
	}
	
	if (do_unmount == 0) {
		printf("usage: unmountfs drive: [drive1:] [drive2:] ...");
	}
	
	return 0;
}

