/*
  format2hd: a minimal 2HD (1024bytes/sector) FD formatter
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


#include <conio.h>
#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _T
#define _T TEXT
#endif



const unsigned char bootsect_dummy[512] = {
#include "bootdumy.h"
};

typedef struct BPBCORE {
  unsigned short bytes_per_sector;      /* +00  +11(0B) bytes per (logical) sector */
  unsigned char sectors_per_cluster;    /* +02  +13(0D) sectors per cluster */
  unsigned short reserved_sectors;      /* +03  +14(0E) reserved sectors */
  unsigned char fats;                   /* +05  +16(10) number of fats */
  unsigned short root_entries;          /* +06  +17(11) number of dirent in rootdir */
  unsigned short sectors;               /* +08  +19(13) number of sectors (if <64K) */
  unsigned char media_descriptor;       /* +10  +21(15) media descriptor */
  unsigned short sectors_per_fat;       /* +11  +22(16) sectors per a fat */
  unsigned short sectors_per_head;      /* +13  +24(18) sectors per a head */
  unsigned short heads_per_track;       /* +15  +26(1A) heads per a track */
} BPBCORE;


BPBCORE bpb_640 = { 512, 2, 1, 2, 0x70, 8*2*80, 0xfb, 2, 8, 2 };
BPBCORE bpb_720 = { 512, 2, 1, 2, 0x70, 9*2*80, 0xf9, 3, 9, 2 };
BPBCORE bpb_2hc = { 512, 1, 1, 2, 0xe0, 15*2*80, 0xf9, 7, 15, 2 };
BPBCORE bpb_2hd = { 1024, 1, 1, 2, 0xc0, 8*2*77, 0xfe, 2, 8, 2 };
BPBCORE bpb_1440 = { 512, 1, 1, 2, 0xe0, 18*2*80, 0xf0, 9, 18, 2 };
BPBCORE bpb_2880 = { 512, 1, 1, 2, 0xf0, 36*2*80, 0xf0, 9, 36, 2 };


MEDIA_TYPE MediaTypeFromBPB(const BPBCORE *bpb, int is_5inch)
{
	if (!bpb) return 0;
	if (bpb->bytes_per_sector == 1024) return is_5inch ? F5_1Pt23_1024 : F3_1Pt23_1024;
	if (bpb->bytes_per_sector == 128) return F8_256_128;
	if (bpb->bytes_per_sector == 512) {
		if (bpb->sectors_per_head == 8) return is_5inch ? F5_640_512 : F3_640_512;
		if (bpb->sectors_per_head == 9) return is_5inch ? F5_720_512 : F3_720_512;
		if (bpb->sectors_per_head == 15) return is_5inch ? F5_1Pt2_512 : F3_1Pt2_512;
		if (bpb->sectors_per_head == 18) return F3_1Pt44_512;
		if (bpb->sectors_per_head == 36) return F3_2Pt88_512;
	}
	if (bpb->media_descriptor == 0xf0) return RemovableMedia;
	if (bpb->media_descriptor == 0xf8) return FixedMedia;

	return 0;
}


struct ERRNAME {
	DWORD dwErr;
	char *sErr;
} errname[] = {
	{ ERROR_ACCESS_DENIED, "Access Denied" },
	{ ERROR_INVALID_HANDLE, "Invalid Handle" },
	{ ERROR_NOT_ENOUGH_MEMORY, "Not Enough Memory" },
	{ ERROR_INVALID_DRIVE, "Invalid Drive" },
	{ ERROR_WRITE_PROTECT, "Write Protected" },
	{ ERROR_BAD_UNIT, "Bad Unit" },
	{ ERROR_NOT_READY, "Device Not Ready" },
	{ ERROR_CRC, "CRC Error" },
	{ ERROR_SEEK, "Seek Error" },
	{ ERROR_NOT_DOS_DISK, "Not MS-DOS Compatible Disk" },
	{ ERROR_SECTOR_NOT_FOUND, "Sector Not found" },
	{ ERROR_LOCK_VIOLATION, "Can't Lock" },
	{ ERROR_INVALID_PARAMETER, "Invalid Parameter" },
	
	{ ERROR_IO_DEVICE, "Device I/O Error" },
	{ ERROR_UNRECOGNIZED_MEDIA, "Wrong (unrecognized) Media"},
	{ 0, NULL }
};


DWORD qdWinver(void)
{
	DWORD dw = GetVersion();
	return ((dw & 0xff) << 8 )|((dw >> 8) & 0xff);
}

int dispErr(DWORD dwErr)
{
	if (dwErr) {
		char *s = "Unknown Error";
		unsigned n;
		for(n=0; errname[n].dwErr != 0 || errname[n].sErr != NULL; ++n) {
			if (errname[n].dwErr == dwErr) s = errname[n].sErr;
		}
		fprintf(stderr, "Error %lu : %s\n", (long)dwErr, s);
	}
	return dwErr != 0;
}

DWORD DevIo_WithErr(HANDLE hDev, DWORD dwCode, LPVOID in, DWORD cb_in, LPVOID out, DWORD cb_out, LPDWORD cb_get, LPOVERLAPPED lpo)
{
	BOOL rc;
	DWORD dw;
	SetLastError(0);
	rc = DeviceIoControl(hDev, dwCode, in, cb_in, out, cb_out, cb_get ? cb_get : &dw, lpo);
	dw = GetLastError();
	if (!rc && dw == 0) dw = (DWORD)-1L;
	return dw;
}

DWORD DevIo_NoParam(HANDLE hDev, DWORD dwCode)
{
	return DevIo_WithErr(hDev, dwCode, NULL, 0, NULL, 0, NULL, NULL);
}


BOOL isDiskWriteProtected(HANDLE hDrive)
{
	DWORD dwPrevErr = GetLastError();
	DWORD dwRC;
	
	dwRC = DevIo_NoParam(hDrive, IOCTL_DISK_IS_WRITABLE);
	if (dwRC != ERROR_WRITE_PROTECT) {
		SetLastError(dwPrevErr);
		return FALSE;
	}
	return TRUE;
}

HANDLE open_drive_a(int drive0, DWORD dw_acc)
{
	HANDLE h;
	UINT uPrevErrMode;
	DWORD dwErr;
	TCHAR volpn[10];

	lstrcpy(volpn, _T("\\\\.\\?:"));
	volpn[4] = _T(( 'A' + drive0));

	uPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	h = CreateFile(volpn, dw_acc, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING, NULL);
	dwErr = GetLastError();
	SetErrorMode(uPrevErrMode);
	SetLastError(dwErr);

	return h;
}


DWORD format_track(HANDLE hDrive, MEDIA_TYPE mediaType, DWORD cylinder, DWORD head)
{
	DWORD dwRC;
	FORMAT_PARAMETERS fm;
	
	ZeroMemory(&fm, sizeof(fm));
	fm.MediaType = mediaType;
	fm.StartCylinderNumber = fm.EndCylinderNumber = cylinder;
	fm.StartHeadNumber = fm.EndHeadNumber = head;
	dwRC = DevIo_WithErr(hDrive, IOCTL_DISK_FORMAT_TRACKS, &fm, sizeof(fm), NULL, 0, NULL, NULL);
	if (dwRC == ERROR_IO_DEVICE && isDiskWriteProtected(hDrive)) dwRC = ERROR_WRITE_PROTECT;

	return dwRC;
}


DWORD format_track_bpb(HANDLE hDrive, const BPBCORE *bpb, MEDIA_TYPE mediaType, unsigned track, int do_verify)
{
	DWORD dwRC;
	LPVOID ptrkbuf = NULL;
	DWORD ntrkbuf = 0;
	unsigned cylinder, head;
	
	if (do_verify > 0) {
		ntrkbuf = (DWORD)(bpb->sectors_per_head) * bpb->bytes_per_sector;
		ptrkbuf = VirtualAlloc(NULL, ntrkbuf, MEM_COMMIT, PAGE_READWRITE);
		if (!ptrkbuf) return GetLastError();
	}

	cylinder = track / bpb->heads_per_track;
	head = track % bpb->heads_per_track;

	dwRC = format_track(hDrive, mediaType, cylinder, head);
	if (dwRC == 0 && do_verify > 0) {
		DWORD dwfp = ntrkbuf * (cylinder * bpb->heads_per_track + head);
		DWORD ntrkbuf_read;
		if (SetFilePointer(hDrive, (LONG)dwfp, NULL, FILE_BEGIN) == dwfp &&
		    ReadFile(hDrive, ptrkbuf, ntrkbuf, &ntrkbuf_read, NULL)) {
			// dwRC = 0;
		} else {
			dwRC = GetLastError();
			fprintf(stderr, " verify error\n");
		}
	}

	if (ptrkbuf) VirtualFree(ptrkbuf, 0, MEM_RELEASE);

	return dwRC;
}


BOOL is_drive_fdd(int drive0, BOOL *is_5inch)
{
	HANDLE hDrive;
	DISK_GEOMETRY g[8];		// enough? 
	DWORD dwRC, dwlen;
	BOOL rc = FALSE;
	BOOL b5inch = FALSE;

	hDrive = open_drive_a(drive0, 0);
	if (hDrive == INVALID_HANDLE_VALUE) return FALSE;

	ZeroMemory(g, sizeof(g));
	dwRC = DevIo_WithErr(hDrive,
	                     IOCTL_DISK_GET_MEDIA_TYPES /* IOCTL_STORAGE_GET_MEDIA_TYPES (will not work for FDD...) */,
	                     NULL, 0, 
	                     &g, sizeof(g),
	                     &dwlen,
	                     NULL);

	if (dwRC == 0) {
		unsigned i, n;
		rc = FALSE;
		n = dwlen / sizeof(g[0]);
		for(i=0; i<n; ++i) {
			if ((g[i].MediaType >= 1 && g[i].MediaType <= 10) ||
			    (g[i].MediaType >= 13 && g[i].MediaType <= 25) )
			{
				rc = TRUE;
			}
			switch(g[i].MediaType) {
				case F5_1Pt2_512:
				case F5_360_512:
				case F5_320_512:
				/* case F5_320_1024: */
				case F5_180_512:
				case F5_160_512:
				case F5_720_512:
				case F5_1Pt23_1024:
					b5inch = TRUE;
				default:
					// b5inch = FALSE;
					break;
			}
		}
	}
	if (rc && is_5inch) {
		*is_5inch = b5inch;
	}

	CloseHandle(hDrive);

	return rc;
}




static unsigned long  tm_to_volume_serial(const struct tm *tm)
{
    /*
      reference: disktut.txt 1993-03-26 (line 1919-)
      serial: xxyy-zzzz
      xx: month + second
      yy: day
      zzzz: year + hour * 0x100 + minute
      
    */
    unsigned xx, yy, zzzz;
    xx = (tm->tm_mon + 1) + tm->tm_sec;
    yy = tm->tm_mday;
    zzzz = tm->tm_year >= 1900 ? tm->tm_year : tm->tm_year + 1900;
    zzzz = zzzz + (tm->tm_hour * 256) + tm->tm_min;
    
    return ((unsigned long)zzzz << 16) | (xx << 8) | yy;
}

static unsigned long  current_time_to_volume_serial(void)
{
    time_t t;
    struct tm *tm;
    t = time(NULL);
    tm = localtime(&t);
    return tm_to_volume_serial(tm);
}

static void pokeow(void *mem, int ofs, unsigned v)
{
	unsigned char *p = mem;
	p[ofs] = v & 0xff;
	p[ofs + 1] = (v >> 8) & 0xff;
}

DWORD write_fs_fat12(HANDLE hDrv, const BPBCORE *bpb, const void *bootsect)
{
	DWORD dwRC;
	DWORD dwWritten;
	unsigned long lba_fat, lba_root, lba_total;
	unsigned dirent_sectors;
	unsigned i;
	unsigned sb, bufsize_total;
	unsigned char *buf;

	sb = bpb->bytes_per_sector;
	lba_fat = bpb->reserved_sectors;
	lba_root = lba_fat + (bpb->sectors_per_fat * bpb->fats);
	dirent_sectors = bpb->root_entries / (bpb->bytes_per_sector / 32);
	lba_total = lba_root + dirent_sectors;

	bufsize_total = lba_total * sb;
	// workaround for reformatting 2HD(1024b/sct) disk to 1.44or2HC(512b/sct)
	// bufsize_total = (bufsize_total + 1023) & (~1023UL);

	buf = VirtualAlloc(0, bufsize_total, MEM_COMMIT, PAGE_READWRITE);
	if (!buf) return GetLastError();
	ZeroMemory(buf, bufsize_total);

	CopyMemory(buf, bootsect, 512);
	pokeow(buf, 11, bpb->bytes_per_sector);
	buf[13] = bpb->sectors_per_cluster;
	pokeow(buf, 14, bpb->reserved_sectors);
	buf[16] = bpb->fats;
	pokeow(buf, 17, bpb->root_entries);
	pokeow(buf, 19, bpb->sectors);
	buf[21] = bpb->media_descriptor;
	pokeow(buf, 22, bpb->sectors_per_fat);
	pokeow(buf, 24, bpb->sectors_per_head);
	pokeow(buf, 26, bpb->heads_per_track);
	
	if (buf[38] == 0x29) {
		unsigned long serial = current_time_to_volume_serial();
		pokeow(buf, 39, serial & 0xffff);
		pokeow(buf, 39 + 2, (serial >> 16) & 0xffff);
		CopyMemory(buf + 43, "NO NAME    ", 11);
		CopyMemory(buf + 54, "FAT12   ", 8);
	}

	buf[510] = 0x55;
	buf[511] = 0xaa;

	for(i = 0; i < bpb->fats; ++i) {
		unsigned char *fatp = buf + ((i * bpb->sectors_per_fat + bpb->reserved_sectors) * sb);
		fatp[0] = bpb->media_descriptor;
		fatp[1] = fatp[2] = 0xff;
	}

	if (SetFilePointer(hDrv, 0, NULL, FILE_BEGIN) == 0 && WriteFile(hDrv, buf, bufsize_total, &dwWritten, NULL)) {
		dwRC = 0;
	} else{
		dwRC = GetLastError();
	}
	VirtualFree(buf, 0, MEM_RELEASE);

	return dwRC;
}



DWORD format_fd_bpb(int drive0, const BPBCORE *bpb, int do_verify, int buildfs, int do_prompt)
{
	int do_msg = 1;
	DWORD dwRC = 0;
	HANDLE hDrive;
	MEDIA_TYPE mediaType;
	unsigned total_tracks, track;
	BOOL is_5inch = FALSE;

	if (!is_drive_fdd(drive0, &is_5inch)) return ERROR_INVALID_DRIVE;
	mediaType = MediaTypeFromBPB(bpb, 0);
	if (!mediaType) return ERROR_INVALID_DRIVE;

	if (do_prompt) {
		if (FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE))) {
			int c;
			fprintf(stderr, "Insert a disk to drive %c:\n", 'A' + drive0);
			fprintf(stderr, "and press Enter when ready.");
			do { c = _getch(); } while (c != EOF && (c & 0xff) != 13 && (c & 0xff) != 10);
			fprintf(stderr, "\n");
		}
	}

	fprintf(stderr, "Formatting %uKiB (%u sectors, %u head(s), %u cylinders, %u bytes/sct)\n", (unsigned)((bpb->sectors * bpb->bytes_per_sector)/1024U), bpb->sectors_per_head, bpb->heads_per_track, bpb->sectors / (bpb->sectors_per_head * bpb->heads_per_track), bpb->bytes_per_sector);

	// at 1st, format track 0 with new mediaType
	// then, remount the medium 
	hDrive = open_drive_a(drive0, GENERIC_READ | GENERIC_WRITE);
	if (hDrive == INVALID_HANDLE_VALUE) return GetLastError();
	dwRC = DevIo_NoParam(hDrive,  FSCTL_LOCK_VOLUME);
	if (dwRC != 0) {
		CloseHandle(hDrive);
		return dwRC;
	}
	SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);
	dwRC = format_track_bpb(hDrive, bpb, mediaType, 0, 0);
	DevIo_NoParam(hDrive,  FSCTL_UNLOCK_VOLUME);
	DevIo_NoParam(hDrive,  FSCTL_DISMOUNT_VOLUME);
	CloseHandle(hDrive);
	if (dwRC != 0) {
		return dwRC;
	}

	hDrive = open_drive_a(drive0, GENERIC_READ | GENERIC_WRITE);
	if (hDrive == INVALID_HANDLE_VALUE) return GetLastError();
	dwRC = DevIo_NoParam(hDrive,  FSCTL_LOCK_VOLUME);
	if (dwRC != 0) {
		CloseHandle(hDrive);
		return dwRC;
	}
	track = 0;
	total_tracks = bpb->sectors / bpb->sectors_per_head;
	for(; track < total_tracks; ++track) {
		if (do_msg) fprintf(stderr, "\rcylinder%3u head%2u", track / bpb->heads_per_track, track % bpb->heads_per_track);
		dwRC = format_track_bpb(hDrive, bpb, mediaType, track, do_verify);
		if (dwRC) break;
		// if ((dwRC > 0 && dwRC <= 22) || dwRC == ERROR_IO_DEVICE) break;
	}
	if (do_msg) fprintf(stderr, "\n");

	SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);
	if (dwRC == 0 && buildfs) {
		if (do_msg) fprintf(stderr, "Building FAT12 filesystem...");
		dwRC = write_fs_fat12(hDrive, bpb, bootsect_dummy);
		if (do_msg) {
			fprintf(stderr, "%s\n", dwRC ? "failure" : "ok");
		}
	}

	DevIo_NoParam(hDrive,  FSCTL_UNLOCK_VOLUME);
	DevIo_NoParam(hDrive,  FSCTL_DISMOUNT_VOLUME);

	CloseHandle(hDrive);

	return dwRC;
}




static BPBCORE *opt_bpb;
static int optHelp;
static int optQ;
static int optS;
static int optU;
static int optRaw;
static int optVerify;
static int optNoPrompt;
int drivenum0 = -1;

static int cmpoptf(const char *s1, const char *s2)
{
	size_t n, n1, n2;
	int c;

	n1 = strlen(s1);
	n2 = strlen(s2);
	if (n1 <= n2) return stricmp(s1, s2);
	for(n=0; n<n2; ++n) {
		c = toupper(s1[n]) - toupper(s2[n]);
		if (c != 0) break;
	}

	return c;
}
# define eqf(s1,s2) (cmpoptf(s1,s2)==0)

int mygetopt(int argc, char *argv[])
{
	char c, *s;

	while(argc > 0) {
		s = *argv;
		c = *s;
		if (c == '-' || c == '/') {
			switch(toupper(*++s)) {
				case '?': optHelp = 1; break;
				case 'Q': optQ = 1; break;
				case 'S': optS = 1; break;
				case 'U': optU = 1; break;
				case 'R':
					if (eqf(s, "RAW")) { optRaw = 1; }
					break;
				case 'V':
					if (eqf(s, "VERIFY")) { optVerify = 1; }
					break;
				case 'N':
					if (eqf(s, "NOP")) { optNoPrompt = 1; }
					break;
				case 'F':
					if (s[1] == ':' || s[1] == '=') {
						s += 2;
						if (eqf(s, "640")) opt_bpb = &bpb_640;
						else if (eqf(s, "720") || eqf(s, "2DD")) opt_bpb = &bpb_720;
						else if (eqf(s, "2HD") || eqf(s, "1.23") || eqf(s, "1.25") || eqf(s, "123") || eqf(s, "125")) opt_bpb = &bpb_2hd;
						else if (eqf(s, "1.44") || eqf(s, "144")) opt_bpb = &bpb_1440;
						else if (eqf(s, "2.88") || eqf(s, "288")) opt_bpb = &bpb_2880;
						else if (eqf(s, "2HC") || strcmp(s, "1.2")==0 || eqf(s, "120")) opt_bpb = &bpb_2hc;
					}
					break;
			}
		}
		else if (((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) && s[1] == ':') {
			drivenum0 = toupper(c) - 'A';
		}
		--argc;
		++argv;
	}

	return 0;
}


void put_usage(void)
{
	const char *progname = "format2hd";
	const char msg[] = 
		"format a floppy disk\n"
		"\n"
		"%s drive: [/F:size] [/RAW] [/VERIFY] [/NOPROMPT]\n"
		"\n"
		"  /F:size  specify size of the disk\n"
		"     640   640K  2DD (8sectors, 80cylinders, 512bytes per sector)\n"
		"     720   720K  2DD (9sectors, 80cylinders, 512bytes per sector)\n"
		"     1.2   1.2M  2HC (15sectors, 80cylinders, 512bytes per sector)\n"
		"     1.23  1.23M 2HD (8sectors, 77cylinders, 1024bytes per sector)\n"
		"     1.44  1.44M (default. 18sectors, 80cylinders, 512bytes per sector)\n"
		"     2DD   same as /F:720\n"
		"     2HC   same as /F:1.2\n"
		"     2HD   same as /F:1.23\n"
		"\n"
		"  /RAW     format only (do not build FAT filesystem after format)\n"
		"  /VERIFY  format with verify (simply abort when error)\n"
		"  /NOPROMPT\n"
		"           do format without waiting keyboard input by user\n"
		;

	printf(msg, progname);
}



int main(int argc, char *argv[])
{
	DWORD dwRC;
	setvbuf(stderr, NULL, _IONBF, 0);
	mygetopt(argc-1, argv+1);
	if (optHelp || drivenum0 < 0) {
		put_usage();
	return optHelp ? 0 : -1;
	}

	if (drivenum0 >= 0 && !opt_bpb) opt_bpb = &bpb_1440;
	dwRC = format_fd_bpb(drivenum0, opt_bpb, optVerify, !optRaw, !optNoPrompt);
	return dispErr(dwRC);
}

