/*
  format2hd: a minimal 2HD (1024bytes/sector) FD formatter
             for Win10 anniversary(lol) update.
  
  Copyright (C) 2016-2023 sava

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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* workaround for some old (or wrong) SDK/DDK header */
#define F3_120M_512		13
#define F3_640_512		14
#define F5_640_512		15
#define F5_720_512		16
#define F3_1Pt2_512		17
#define F3_1Pt23_1024	18
#define F5_1Pt23_1024	19
#define F3_128Mb_512	20
#define F3_230Mb_512	21
#define F8_256_128		22
#define F3_200Mb_512	23
#define F3_240M_512		24
#define F3_32M_512		25

#ifndef _T
#define _T TEXT
#endif

#define ALLOW_FAKE640 1
# define ALLOW_FAKE640TEST 1
#define ALLOW_DMF 1
/* #define ALLOW_SINGLE_SIDE 1 */
/* #define EJECT_AFTER_FORMAT 1 */
/* #define BE_SENSITIVE_ABOUT_UPDAING_BPB 0 */

enum {
	NO_FAKE640 = 0,
	FAKE640,
	FAKE640_TEST
};

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
BPBCORE bpb_1dd360 = { 512, 2, 1, 2, 0x70, 9*1*80, 0xf8, 3, 9, 1 };
BPBCORE bpb_2hc = { 512, 1, 1, 2, 0xe0, 15*2*80, 0xf9, 7, 15, 2 };
BPBCORE bpb_2hd = { 1024, 1, 1, 2, 0xc0, 8*2*77, 0xfe, 2, 8, 2 };
BPBCORE bpb_2hd80 = { 1024, 1, 1, 2, 0xc0, 8*2*80, 0xf0, 2, 8, 2 };
BPBCORE bpb_1440 = { 512, 1, 1, 2, 0xe0, 18*2*80, 0xf0, 9, 18, 2 };
BPBCORE bpb_2880 = { 512, 1, 1, 2, 0xf0, 36*2*80, 0xf0, 9, 36, 2 };

BPBCORE bpb_1840 = { 512, 1, 1, 2, 0xe0, 23*2*80, 0xf0, 11, 23, 2 };
BPBCORE bpb_1680dmf = { 512, 4, 1, 2, 0x10, 21*2*80, 0xf0, 3, 21, 2 };

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
#if ALLOW_DMF
		if (bpb->sectors_per_head >= 19 && bpb->sectors_per_head <= 24) return F3_1Pt44_512;
#endif
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
	{ ERROR_INVALID_FUNCTION, "Invalid Function (IOCTL Error)" },
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
	{ ERROR_SHARING_VIOLATION, "Sharing Violation" },
	{ ERROR_LOCK_VIOLATION, "Can't Lock" },
	{ ERROR_NOT_SUPPORTED, "Not Supported" },
	{ ERROR_INVALID_PARAMETER, "Invalid Parameter" },
	
	{ ERROR_IO_DEVICE, "Device I/O Error" },
	{ ERROR_FLOPPY_ID_MARK_NOT_FOUND, "Floppy ID Mark Not Found" },
	{ ERROR_FLOPPY_WRONG_CYLINDER, "Floppy Wrong Cylinder" },
	{ ERROR_FLOPPY_UNKNOWN_ERROR, "Floppy Unknown Error" },

	{ ERROR_MEDIA_CHANGED, "Media Changed" },
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
	DWORD timeout_ms = 5000;
	DWORD tick_start = GetTickCount();

	if (dwCode == IOCTL_DISK_FORMAT_TRACKS || dwCode == IOCTL_DISK_FORMAT_TRACKS_EX)
		timeout_ms = 15000;

	do {
		DWORD tick, difftick;
		SetLastError(0);
		rc = DeviceIoControl(hDev, dwCode, in, cb_in, out, cb_out, cb_get ? cb_get : &dw, lpo);
		dw = GetLastError();
		if (!rc && dw == 0) dw = (DWORD)-1L;
		if (dw != ERROR_NOT_READY && dw != ERROR_IO_DEVICE /* && dw != ERROR_MEDIA_CHANGED */) break;
		tick = GetTickCount();
		difftick = (tick >= tick_start) ? tick - tick_start : (((DWORD)-1L) - tick_start) + 1 + tick;
		if (difftick >= timeout_ms) break;
		Sleep(1500);
	} while(1);

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

HANDLE open_drive_a_flag(int drive0, DWORD dw_acc, DWORD dwFlag)
{
	HANDLE h;
	UINT uPrevErrMode;
	DWORD dwErr;
	TCHAR volpn[10];
	int try_count = 3;

	lstrcpy(volpn, _T("\\\\.\\?:"));
	volpn[4] = _T(( 'A' + drive0));

	dwErr = GetLastError();
	uPrevErrMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	while(try_count-- > 0) {
		h = CreateFile(volpn
		             , dw_acc
		             , FILE_SHARE_READ | FILE_SHARE_WRITE
		             , NULL
		             , OPEN_EXISTING
		             , dwFlag
		             , NULL);
		dwErr = GetLastError();
		if (h != INVALID_HANDLE_VALUE) { dwErr = 0; break; }
		Sleep(1500); /* wait for the drive to get ready */
	}
	if (h != INVALID_HANDLE_VALUE) FlushFileBuffers(h);
	SetErrorMode(uPrevErrMode);
	SetLastError(dwErr);

	return h;
}

HANDLE open_drive_a(int drive0, DWORD dw_acc)
{
	/* not certain about FILE_SHARE_xxx and FILE_FLAG_xxx */
	return open_drive_a_flag(drive0, dw_acc, FILE_FLAG_NO_BUFFERING);
}

#if ALLOW_DMF
DWORD format_track(HANDLE hDrive, MEDIA_TYPE mediaType, DWORD cylinder, DWORD head, int sector)
{
	DWORD dwRC = (DWORD)-1;

	if (mediaType == F3_1Pt44_512 && sector != 0 && sector > 18) {
		WORD n;
		WORD secnum;
		BYTE expbuf[sizeof(FORMAT_EX_PARAMETERS) + sizeof(WORD)*255];
		DWORD expbufsiz;
		FORMAT_EX_PARAMETERS *exfm;

		exfm = (void *)expbuf;
		expbufsiz = sizeof(FORMAT_EX_PARAMETERS) - sizeof(exfm->SectorNumber) + (sizeof(exfm->SectorNumber[0]) * sector);
		ZeroMemory(exfm, sizeof(expbuf));
		exfm->MediaType = mediaType;
		exfm->StartCylinderNumber = exfm->EndCylinderNumber = cylinder;
		exfm->StartHeadNumber = exfm->EndHeadNumber = head;
		exfm->SectorsPerTrack = sector;
		secnum = 0;
		for(n=0; n<sector; n+=2, ++secnum) {
			exfm->SectorNumber[n] = secnum + 1;
		}
		for(n=1; n<sector; n+=2, ++secnum) {
			exfm->SectorNumber[n] = secnum + 1;
		}
		exfm->FormatGapLength = (sector > 18) ? 0x0c : 108;	/* 1440 = 0x1b 1680 = 0x0c */
		dwRC = DevIo_WithErr(hDrive, IOCTL_DISK_FORMAT_TRACKS_EX, exfm, expbufsiz, NULL, 0, NULL, NULL);
	}
	else {
		FORMAT_PARAMETERS fm;
		ZeroMemory(&fm, sizeof(fm));
		fm.MediaType = mediaType;
		fm.StartCylinderNumber = fm.EndCylinderNumber = cylinder;
		fm.StartHeadNumber = fm.EndHeadNumber = head;
		dwRC = DevIo_WithErr(hDrive, IOCTL_DISK_FORMAT_TRACKS, &fm, sizeof(fm), NULL, 0, NULL, NULL);
	}
	if (dwRC == ERROR_IO_DEVICE && isDiskWriteProtected(hDrive)) dwRC = ERROR_WRITE_PROTECT;

	return dwRC;
}
#else
DWORD format_track(HANDLE hDrive, MEDIA_TYPE mediaType, DWORD cylinder, DWORD head, int sector)
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
#endif

DWORD format_track_bpb(HANDLE hDrive, const BPBCORE *bpb, MEDIA_TYPE mediaType, unsigned track, int do_verify)
{
	DWORD dwRC;
	LPVOID ptrkbuf = NULL;
	DWORD ntrkbuf = 0;
	unsigned cylinder, head, sector;
	
	if (do_verify > 0) {
		ntrkbuf = (DWORD)(bpb->sectors_per_head) * bpb->bytes_per_sector;
		ptrkbuf = VirtualAlloc(NULL, ntrkbuf, MEM_COMMIT, PAGE_READWRITE);
		if (!ptrkbuf) return GetLastError();
	}

	cylinder = track / bpb->heads_per_track;
	head = track % bpb->heads_per_track;
	sector = bpb->sectors_per_head;

	dwRC = format_track(hDrive, mediaType, cylinder, head, sector);
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


static BOOL print_media_type(const DISK_GEOMETRY *g)
{
	static const char *mtstr[] = {
		"unknown medium",
		"5.25\" 1.2M",
		"3.5\" 1.44M",
		"3.5\" 2.88M",
		"3.5\" 20.8M",
		"3.5\" 720K",
		"5.25\" 360K",
		"5.25\" 320K",
		"5.25\" 320K 1024bytes/sct",
		"5.25\" 180K",
		"5.25\" 160K",
		"non-floppy removable medium",
		"non-removable drive",
		"3.5\" 120M",
		"3.5\" 640K",
		"5.25\" 640K",
		"5.25\" 720K",
		"3.5\" 1.2M",
		"3.5\" 1.23M 1024bytes/sct",
		"5.25\" 1.23M 1024bytes/sct",
		"3.5\" 128M",
		"3.5\" 230M",
		"8\" 256K 128bytes/sct",
		"3.5\" 200M HiFD",
		"3.5\" 240M HiFD",
		"3.5\" 32M"
	};
	BOOL rc = FALSE;

	printf("mediaType=%u", g->MediaType);
	if (g->MediaType < sizeof(mtstr)/sizeof(mtstr[0])) {
		printf(" (%s)", mtstr[g->MediaType]);
		rc = TRUE;
	}
	printf(", cyl=%u, head=%u, sct=%u, %ubytes/sct\n", (unsigned)(g->Cylinders.LowPart), (unsigned)(g->TracksPerCylinder), (unsigned)(g->SectorsPerTrack), (unsigned)(g->BytesPerSector));
	return rc;
}

static BOOL is_media_5inch(DWORD mt)
{
	switch(mt) {
		case F5_1Pt2_512:
		case F5_360_512:
		case F5_320_512:
		case F5_320_1024:
		case F5_180_512:
		case F5_160_512:
		/* case F8_256_128: */
		case F5_640_512:
		case F5_720_512:
		case F5_1Pt23_1024:
			return TRUE;
	}
	return FALSE;
}

BOOL check_drive_media_type(int drive0, BOOL *is_5inch, BOOL list_types)
{
	HANDLE hDrive;
	DISK_GEOMETRY g[16];		// enough?
	DWORD dwRC, dwlen;
	BOOL rc = FALSE;
	BOOL b5inch = FALSE;
	int try_count = 3;

	hDrive = open_drive_a(drive0, 0);
	if (hDrive == INVALID_HANDLE_VALUE) return FALSE;

	ZeroMemory(g, sizeof(g));
	while(try_count-- > 0) {
		dwRC = DevIo_WithErr(hDrive,
		                     IOCTL_DISK_GET_MEDIA_TYPES /* IOCTL_STORAGE_GET_MEDIA_TYPES (will not work for FDD...) */,
		                     NULL, 0, 
		                     &(g[0]), sizeof(g),
		                     &dwlen,
		                     NULL);
		if (dwRC != ERROR_INVALID_DRIVE && dwRC != ERROR_MEDIA_CHANGED) break;
		Sleep(1500);
	}

	if (dwRC == 0) {
		unsigned i, n;
		rc = FALSE;
		n = dwlen / sizeof(g[0]);
		if (list_types) {
			printf("Number of supported medium type : %u\n", n);
		}
		// detect 5.25 medium
		for(i=0; i<n; ++i) {
			if (is_media_5inch(g[i].MediaType)) {
				b5inch = TRUE;
				break;
			}
		}
		// detect 3.5 medium (and overwrite 5.25 setting)
		for(i=0; i<n; ++i) {
			const DWORD mt = g[i].MediaType;
			if (list_types) {
				print_media_type(&(g[i]));
			}
			if ((mt >= 1 && mt <= 10) ||
			    (mt >= 13 && mt <= 25) )
			{
				rc = TRUE;
				if (!is_media_5inch(mt) && mt != F8_256_128) {
					b5inch = FALSE;
				}
			}
		}
	}
	if (rc && is_5inch) {
		*is_5inch = b5inch;
	}

	CloseHandle(hDrive);

	return rc;
}

BOOL is_drive_fdd(int drive0, BOOL *is_5inch)
{
	return check_drive_media_type(drive0, is_5inch, FALSE);
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

void make_bootsector(void *buffer, const BPBCORE *bpb, const void *bootsect)
{
	unsigned char *buf = buffer;
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
#if ALLOW_DMF
	if (bpb->sectors == 21*2*80) {
		CopyMemory(buf + 3, "MSDMF3.2", 8); /* it seems that some USB-FDDs look "MSDMF?.?" OEM string in the boot sector to use it as DMF diskette... */
	}
#endif

	buf[510] = 0x55;
	buf[511] = 0xaa;
}


DWORD write_fs_fat12(HANDLE hDrv, const BPBCORE *bpb, const void *bootsect, unsigned physical_sectors_per_head, int build_fake640_testdisk)
{
	DWORD dwRC;
	DWORD dwWritten;
	unsigned long lba_fat, lba_root, lba_total;
	unsigned dirent_sectors;
	unsigned i;
	unsigned sb, bufsize_total;
	unsigned char *buf;

	if (physical_sectors_per_head == 0) physical_sectors_per_head = bpb->sectors_per_head;
	sb = bpb->bytes_per_sector;
	lba_fat = bpb->reserved_sectors;
	lba_root = lba_fat + (bpb->sectors_per_fat * bpb->fats);
	dirent_sectors = bpb->root_entries / (bpb->bytes_per_sector / 32);
	lba_total = lba_root + dirent_sectors;
	lba_total = (lba_total / bpb->sectors_per_head) * physical_sectors_per_head + (lba_total % bpb->sectors_per_head);

	bufsize_total = lba_total * sb;
	if (build_fake640_testdisk) {
		bufsize_total += sb * 3;
	}

	buf = VirtualAlloc(0, bufsize_total, MEM_COMMIT, PAGE_READWRITE);
	if (!buf) return GetLastError();
	ZeroMemory(buf, bufsize_total);

#if !(BE_SENSITIVE_ABOUT_UPDAING_BPB)
	make_bootsector(buf, bpb, bootsect);
#endif

	for(i = 0; i < bpb->fats; ++i) {
		unsigned long lbafat = i * bpb->sectors_per_fat + bpb->reserved_sectors;
		unsigned long lbafat_phys = (lbafat / bpb->sectors_per_head) * physical_sectors_per_head + (lbafat % bpb->sectors_per_head);
		unsigned char *fatp = buf + lbafat_phys * sb;
		fatp[0] = bpb->media_descriptor;
		fatp[1] = fatp[2] = 0xff;
		if (build_fake640_testdisk) {
			fatp[3] = 0xff;
			fatp[4] = 0x0f;
		}
	}

	if (build_fake640_testdisk) {
		const static unsigned char check_dirent[] = {
			'C', 'H', 'E', 'C', 'K', '6', '4', '0', 'T', 'X', 'T',  // 00-0A filename + ext
			0x20,                                                   // 0B attr
			0,                                                      // 0C flag (vfat)
			0, 0, 0,                                                // 0D-0F ctime (vfat)
			0, 0,                                                   // 10-11 cdate (vfat)
			0, 0,                                                   // 12-13 atime (vfat)
			0, 0,                                                   // 14-15 clusterHi (fat32)
			0, 0,                                                   // 16-17 mtime
			(1 << 5) | 1, 0,                                        // 18-19 mdate
			2, 0,                                                   // 1A-1B clusterLo
			8, 0, 0, 0                                              // 1C-1F filelength
		};
		const char ok640[] = "OK, 640K";
		const char no720[] = "NO, 720K";
		unsigned long lbadir = bpb->reserved_sectors + (bpb->sectors_per_fat * bpb->fats);
		unsigned char *pdir = buf + lbadir * sb;
		memcpy(pdir, check_dirent, sizeof(check_dirent));
		strcpy(buf + (lba_total + 1) * sb, ok640);
		strcpy(buf + (lba_total + 0) * sb, no720);
	}

	if (SetFilePointer(hDrv, 0, NULL, FILE_BEGIN) == 0 && WriteFile(hDrv, buf, bufsize_total, &dwWritten, NULL)) {
#if BE_SENSITIVE_ABOUT_UPDAING_BPB
		make_bootsector(buf, bpb, bootsect);
		if (SetFilePointer(hDrv, 0, NULL, FILE_BEGIN) == 0 && WriteFile(hDrv, buf, bpb->bytes_per_sector, &dwWritten, NULL))
#endif
		{
			FlushFileBuffers(hDrv);
			SetLastError(0);
		}
	}
	dwRC = GetLastError();
	VirtualFree(buf, 0, MEM_RELEASE);

	return dwRC;
}


static void print_formatting_parameter(const BPBCORE *bpb)
{
	fprintf(stderr, "Formatting %uKiB (%u sectors, %u head(s), %u cylinders, %u bytes/sct)\n"
	              , (unsigned)((bpb->sectors * bpb->bytes_per_sector)/1024U)
	              , bpb->sectors_per_head
	              , bpb->heads_per_track
	              , bpb->sectors / (bpb->sectors_per_head * bpb->heads_per_track)
	              , bpb->bytes_per_sector
	       );
}


DWORD format_fd_bpb(int drive0, const BPBCORE *bpb0, int do_verify, int buildfs, int do_prompt, int fake640)
{
	int do_msg = 1;
	DWORD dwRC = 0;
	DWORD dwEject;
	HANDLE hDrive;
	MEDIA_TYPE mediaType;
	unsigned total_tracks, track;
	BOOL is_5inch = FALSE;
	unsigned physical_sectors_per_head = 0;
	const BPBCORE *bpb = bpb0;

	if (!is_drive_fdd(drive0, &is_5inch)) return ERROR_INVALID_DRIVE;
	mediaType = MediaTypeFromBPB(bpb, is_5inch);
	if (!mediaType) return ERROR_INVALID_DRIVE;
#if ALLOW_FAKE640
	if ((mediaType == F5_640_512 || mediaType == F3_640_512) && buildfs && fake640 != NO_FAKE640) {
		bpb = &bpb_720;
		mediaType = (mediaType == F5_640_512) ? F5_720_512 : F3_720_512;
		physical_sectors_per_head = fake640 == FAKE640 ? bpb->sectors_per_head : 0;
	}
	else
#endif
	{
		fake640 = NO_FAKE640;
	}

	if (do_prompt) {
		if (FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE))) {
			int c;
			fprintf(stderr, "Insert a disk to drive %c:\n", 'A' + drive0);
			fprintf(stderr, "and press Enter when ready.");
			do {
				c = _getch();
				switch(c & 0xff) {
					case 13: case 10:
						c = 13;
						break;
					case 0x1b: case 0x03:
						exit(1);
				}
			} while (c != EOF && c != 13);
			fprintf(stderr, "\n");
		}
	}

	print_formatting_parameter(bpb);

	// at 1st, format track 0 with new mediaType
	// then, remount the medium 
	hDrive = open_drive_a(drive0, GENERIC_READ | GENERIC_WRITE);
	if (hDrive == INVALID_HANDLE_VALUE) return GetLastError();
	dwRC = DevIo_NoParam(hDrive,  FSCTL_LOCK_VOLUME);
	if (dwRC != 0) {
		CloseHandle(hDrive);
		return dwRC;
	}
	FlushFileBuffers(hDrive);
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
	DevIo_NoParam(hDrive,  FSCTL_DISMOUNT_VOLUME);
	track = 0;
	total_tracks = bpb->sectors / bpb->sectors_per_head;
	for(; track < total_tracks; ++track) {
		if (do_msg) fprintf(stderr, "\rcylinder%3u head%2u (%u%%) ", track / bpb->heads_per_track, track % bpb->heads_per_track, ((track + 1) * 100) / total_tracks);
		dwRC = format_track_bpb(hDrive, bpb, mediaType, track, do_verify);
		if (dwRC) break;
		// if ((dwRC > 0 && dwRC <= 22) || dwRC == ERROR_IO_DEVICE) break;
	}
	if (do_msg) fprintf(stderr, "\n");

	SetFilePointer(hDrive, 0, NULL, FILE_BEGIN);
	if (dwRC == 0 && buildfs) {
		if (do_msg) {
			fprintf(stderr, "Building FAT12 filesystem");
			if (fake640 != NO_FAKE640) {
				fprintf(stderr, " (fake640%s)", fake640 == FAKE640_TEST ? " testdisk" : "");
			}
			fprintf(stderr, "...");
		}
		dwRC = write_fs_fat12(hDrive, bpb0, bootsect_dummy, physical_sectors_per_head, fake640 == FAKE640_TEST);
		if (do_msg) {
			fprintf(stderr, "%s\n", dwRC ? "failure" : "ok");
		}
	}

	DevIo_NoParam(hDrive, FSCTL_UNLOCK_VOLUME);
	DevIo_NoParam(hDrive, FSCTL_DISMOUNT_VOLUME);
#if EJECT_AFTER_FORMAT
	dwEject = DevIo_NoParam(hDrive, IOCTL_DISK_EJECT_MEDIA);
	CloseHandle(hDrive);
	if (dwRC == 0 && dwEject != 0 && do_msg) {
		fprintf(stderr, "Please eject the medium in the drive %c.\n", 'A'+drive0);
	}
#else
	CloseHandle(hDrive);
#endif

	return dwRC;
}


static BPBCORE *opt_bpb;
static int optHelp;
static int optQ;
static int optS;
static int optU;
static int optList;
static int optRaw;
static int optVerify;
static int optNoPrompt;
static int optFake640 = NO_FAKE640;
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
			++s;
			if (c == '-' && *s == '-' && s[1] != '\0') ++s;
			switch(toupper(*s)) {
				case '?': case 'H': optHelp = 1; break;
				case 'L':
					if (eqf(s, "LIST")) { optList = 1; }
					break;
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
#if ALLOW_FAKE640
# if ALLOW_FAKE640TEST
						if (eqf(s, "fake640t")) { opt_bpb = &bpb_640; optFake640 = FAKE640_TEST; }
						else
# endif
						if (eqf(s, "640f") || eqf(s, "fake640")) { opt_bpb = &bpb_640; optFake640 = FAKE640; }
						else
#endif
						if (eqf(s, "640") && !eqf(s, "640f")) opt_bpb = &bpb_640;
						else if (eqf(s, "720") || eqf(s, "2DD")) opt_bpb = &bpb_720;
						else if (eqf(s, "2HD80")) opt_bpb = &bpb_2hd80;
						else if (eqf(s, "2HD") || eqf(s, "1.23") || eqf(s, "1.25") || eqf(s, "123") || eqf(s, "125")) opt_bpb = &bpb_2hd;
						else if (eqf(s, "1.44") || eqf(s, "144")) opt_bpb = &bpb_1440;
						else if (eqf(s, "2.88") || eqf(s, "288") || eqf(s, "2ED")) opt_bpb = &bpb_2880;
						else if (eqf(s, "2HC") || strcmp(s, "1.2")==0 || eqf(s, "120")) opt_bpb = &bpb_2hc;
#if ALLOW_DMF
						else if (eqf(s, "168") || eqf(s, "DMF")) opt_bpb = &bpb_1680dmf;
#endif
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
		"%s drive: /LIST\n"
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
#if ALLOW_DMF
		"     DMF   1.68M Microsoft DMF (21sectors, 80cylinders, 512bytes per sector)\n"
#endif
#if ALLOW_FAKE640
		"     fake640       format with 720K, and build FAT as 640K disk\n"
# if ALLOW_FAKE640TEST
		"     fake640test   format with 720K, and build 640K test disk for the drive\n"
# endif
#endif
		"\n"
		"  /RAW     format only (do not build FAT filesystem after format)\n"
		"  /VERIFY  format with verify (simply abort when error)\n"
		"  /NOPROMPT\n"
		"           do format without waiting keyboard input by user\n"
		"\n"
		"  /LIST    print supported medium type by the drive (just for reference)\n"
		;

	printf("%s", progname);
	printf(" (");
#if defined(WIN64)
	printf("Win64");
#else
	printf("Win32");
#endif
	printf(", built at " __DATE__ " " __TIME__ ")\n");
	printf(msg, progname, progname);
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

	if (optList) {
		BOOL is_5inch = FALSE;
		if (check_drive_media_type(drivenum0, &is_5inch, optList)) {
			if (is_5inch) printf("Guess the drive %c is 5.25\" FDD.\n", 'A' + drivenum0);
			return 0;
		}
		fprintf(stderr, "Error: the drive %c is not ready, not FDD or not exist.\n", 'A' + drivenum0);
		return 1;
	}
	if (drivenum0 >= 0 && !opt_bpb) opt_bpb = &bpb_1440;
	dwRC = format_fd_bpb(drivenum0, opt_bpb, optVerify, !optRaw, !optNoPrompt, optFake640);
	return dispErr(dwRC);
}

