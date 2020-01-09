/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#include <ctype.h>
#include <dos.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCLUDE_DUMP_BODY
#include "dumpmem.h"
#define INCLUDE_HELPER_BODY
#include "helper.h"

#if defined _MSC_VER || defined __WATCOMC__
#pragma pack(1)
#endif

typedef struct REQHDR_BASE {
	unsigned char length;
	unsigned char unit;
	unsigned char command;
	unsigned short status;
	unsigned char reserved4[4];
	void far *ed4_next;
} REQHDR_BASE;


struct DEVHDR_BASE;

typedef struct DEVHDR_BASE {
	struct DEVHDR_BASE far *next;
	unsigned short attr;
	unsigned short strategy;
	unsigned short commands;
	char name[8];
} DEVHDR_BASE;


struct DPB_BASE3;
struct DPB_BASE4;

typedef struct DPB_BASE3 {
	unsigned char drive;
	unsigned char unit;
	unsigned short bytes_per_sector;
	unsigned char highest_secnum;
	unsigned char shift_count;
	unsigned short reserved_sectors;
	unsigned char fats;
	unsigned short root_directories;
	unsigned short first_data_sector;
	unsigned short highest_cluster;
	unsigned char sectors_per_fat;
	unsigned short first_directory_sector;
	void far *device_header;
	unsigned char media_descriptor;
	unsigned char is_accessed;
	struct DPB_BASE3 far *next;
} DPB_BASE3;
typedef struct DPB_BASE4 {
	unsigned char drive;
	unsigned char unit;
	unsigned short bytes_per_sector;
	unsigned char highest_secnum;
	unsigned char shift_count;
	unsigned short reserved_sectors;
	unsigned char fats;
	unsigned short root_directories;
	unsigned short first_data_sector;
	unsigned short highest_cluster;
	unsigned short sectors_per_fat;
	unsigned short first_directory_sector;
	void far *device_header;
	unsigned char media_descriptor;
	unsigned char is_accessed;
	struct DPB_BASE4 far *next;
} DPB_BASE4;


const void far *get_1stblkdev(void)
{
	DEVHDR_BASE far *dev;
	DEVHDR_BASE far *blk = NULL;
	dev = get_1stdosdev();
	while(1){
		if (!(dev->attr & 0x8000)) {
#if 1
			fprintf(stderr, "blkdev %Fp num=%u\n", dev, dev->name[0]);
#endif
			blk = dev;
		}
		dev = dev->next;
		if (FP_OFF(dev) == 0xffff) break;
	}
	return (const void far *)blk;
}


unsigned disp_bpb(const void far * const dev, int unit, unsigned char media_id)
{
	char rqbuf[13+1+4+4];
	char buf[0x15 + 4];
	REQHDR_BASE *rq;
	const DEVHDR_BASE far *blkdev;
	char *dskbuf;

	memset(rqbuf,0,sizeof(rqbuf));
	memset(buf,0,sizeof(buf));
	blkdev = dev ? (const DEVHDR_BASE far *)dev : get_1stblkdev();
	if (!blkdev) {
		fprintf(stderr, "FATAL: no block device.\n");
		return (unsigned)-1;
	}
	dskbuf = mymalloc(2048);

	rq = (REQHDR_BASE *)rqbuf;
	rq->unit = unit;
	rq->command = 2;
	rqbuf[13] = media_id;
	dskbuf[0] = media_id;
	dskbuf[1] = 0xff;
	*(void far * *)(rqbuf + 14) = (void far *)dskbuf;
	*(void far * *)(rqbuf + 18) = (void far *)buf;
	devcall(blkdev, rq);
#if 1
	printf("result status: %04X\n", rq->status);
	if ((rq->status & 0x8100) == 0x0100) {
		dumpmem(*(void far * *)(rqbuf + 18), 32, DUMP_ADDRESS);
	}
#endif
	free(dskbuf);
	return rq->status;
}


unsigned dummy_cmd(const void far * const dev, int unit, int cmd)
{
	char rqbuf[32];
	REQHDR_BASE *rq;
	const DEVHDR_BASE far *blkdev;
	
	memset(rqbuf,0,sizeof(rqbuf));
	blkdev = dev ? (const DEVHDR_BASE far *)dev : get_1stblkdev();
	rq = (REQHDR_BASE *)rqbuf;
	rq->unit = unit;
	rq->command = cmd;
	devcall(blkdev, rq);
	printf("result status: %04X\n", rq->status);
	return rq->status;
}


int print_dpb1(int drive1, int media_id) /* drive1 = 0 as default, 1 as A:, 2 as B: ... */
{
	union REGS r;
	struct SREGS sr;
	unsigned char far *dpb = NULL;
	unsigned offdpb = 0;
	void far *device;
	unsigned char extdpb[80];

	extdpb[0] = extdpb[1] = 0;
	r.x.ax = 0x3000;
	intdos(&r, &r);
	if (r.h.al > 3 && r.h.al != 10) offdpb = 1;

	printf("DPB for ");
	if (drive1) printf("drive %c: ... ", 'A' + drive1 - 1);
	else printf("default drive ... ");
	r.x.ax = 0x3200;
	r.x.dx = drive1;
	r.x.bx = sr.ds = 0xffffU;
	intdosx(&r, &r, &sr);
	if (!r.x.cflag && r.h.al == 0) {
		dpb = MK_FP(sr.ds, r.x.bx);
	}
	else {
		r.x.ax = 0x7302;
		r.x.dx = drive1;
		r.x.cx = sizeof(extdpb);
		r.x.si = 0xf1a6;
		r.x.di = FP_OFF(extdpb);
		sr.es = FP_SEG(extdpb);
		intdosx(&r, &r, &sr);
		if (!r.x.cflag && r.h.al == 0 && extdpb[0] >= 24) {
			dpb = extdpb + 2;
		}
	}
	if (!dpb) {
		printf(" (failure)\n");
		return -1;
	}
	if (extdpb[0] || extdpb[1]) printf("(FAT32 Extended DPB)\n");
	else printf("%Fp\n", dpb);

	device = (void far *)(peekod(dpb,0x12+offdpb));
	if (media_id == 0) {
		media_id = peeko(dpb,0x16+offdpb);
		if (media_id < 0xf0) media_id = media_id ^ 0xffU;
	}
	printf("drive number                        %u\n", peeko(dpb,0));
	printf("unit number in the device           %u\n", peeko(dpb,1));
	printf("bytes per sector                    %u\n", peekow(dpb,2));
	printf("reserved sectors                    %u\n", peekow(dpb,6));
	printf("number of FATs                      %u\n", peeko(dpb,8));
	printf("number of root directory entries    %u\n", peekow(dpb,9));
	printf("first sector number for user data   %u\n", peekow(dpb,0xb));
	printf("highest cluster number              %u (0x%X)\n", peekow(dpb,0x0d));
	printf("sectors per FAT                     %u\n", offdpb ? peekow(dpb,0xf) : peeko(dpb,0xf));
	printf("device header                       %Fp\n", device);
	printf("media ID byte                       0x%02X\n", peekod(dpb,0x16+offdpb));
	printf("disk access flags                   0x%02X\n", peeko(dpb,0x17+offdpb));
	if (extdpb[0] || extdpb[1]) {
		printf("---extended DPB---\n");
		printf("free clusters                       ");
		if (peekow(dpb,0x1f)==0xffff) printf("%u (unknown)\n", peekow(dpb,0x1f));
		else printf("%lu\n", peekod(dpb,0x1f));
		printf("file system information sector   ");
		if (peekow(dpb,0x25)==0xffff) printf("   none\n");
		else printf("at %u (0x%x)\n", peekow(dpb,0x25), peekow(dpb,0x25));
		printf("backup boot sector               ");
		if (peekow(dpb,0x27)==0xffff) printf("   none\n");
		else printf("at %u (0x%x)\n", peekow(dpb,0x27), peekow(dpb,0x27));
		printf("1st sector number of 1st cluster    %lu (0x%lx)\n", peekod(dpb,0x29), peekod(dpb,0x29));
		printf("maximum cluster number              %lu (0x%lx)\n", peekod(dpb,0x2d), peekod(dpb,0x2d));
		printf("number of sectors occupied by FAT   %lu (0x%lx)\n", peekod(dpb,0x31), peekod(dpb,0x31));
		printf("cluster number of root directory    %lu (0x%lx)\n", peekod(dpb,0x35), peekod(dpb,0x35));
		printf("cluster no. for searching freespace %lu (0x%lx)\n", peekod(dpb,0x39), peekod(dpb,0x39));
	}

	printf("\n");

	printf("device driver - BUILD BPB ... ");
	disp_bpb(device, peeko(dpb,1), media_id);

	return 0;
}


int param_drive1 = 0;
int param_mediaid = 0;


int main(int argc, char *argv[])
{
	int params = 0;
	while(--argc > 0) {
		char *s = *++argv;
		if (isalpha(*s) && s[1] == ':' && params == 0) {
			param_drive1 = toupper(*s) - 'A' + 1;
			++params;
		}
		else if (params == 1) {
			param_mediaid = (int)strtol(s, NULL, 0);
			++params;
		}
	}
	if (!param_drive1) {
		printf("usage: bpbdpb drive: media_id\n");
		return 0;
	}

	return print_dpb1(param_drive1, param_mediaid);
}
