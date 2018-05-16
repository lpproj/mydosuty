/*
    iosnap.c: dump IO.SYS part in MS-DOS
    target: DOS 3+ (mostly MS-DOS for NEC PC-98 series)
    to build:
        lcc iosnap.c -lintlib -ltinymain.obj (LSI-C 3.30c trial)
    
    license: UNLICENSE (http://unlicense.org/)
    

*/

#include <ctype.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int optA;
int optS;
int optHelp;
int optI;
int optM;
char *outfile;

void mygetopt(int argc, char *argv[])
{
	while(--argc >= 0) {
		char *s = *++argv;
		if (*s == '-' || *s == '/') {
			switch(toupper(s[1])) {
				case 'S':
					optS = 1;
					break;
				case 'A':
					optA = 1;
					break;
				case 'I':
					optI = 1;
					break;
				case 'M':
					optM = 1;
					break;
				case '?':
				case 'H':
					optHelp = 1;
					break;
			}
		}
		else {
			if (!outfile) outfile = s;
		}
	}
}

static char usagestr[] = \
	"usage: iosnap [/s] [/m] [/a] [/i] output_filename\n"
	"  /s BIOS memory area (0000:0400~05FF)\n"
	"  /m IO.SYS memory area (0060:0000~)\n"
	"  /a BIOS + IO.SYS memory area\n"
	"  /i IDE BIOS work area (D800:2000~21FF)\n"
	;

int main(int argc, char *argv[])
{
	int rc = -1;
	union REGS r;
	struct SREGS sr;
	unsigned iosys_len;
	unsigned blk_seg, blk_len;
	
	mygetopt(argc, argv);
	
	/* assume IO.SYS area as 0060:0000...DOSdataseg:0000 */
	r.h.ah = 0x52;
	intdosx(&r, &r, &sr);
	iosys_len = (sr.es - 0x60) * 16U;

	if (optA) {
		/* BIOS memory block + IO.SYS */
		blk_seg = 0x40;
		blk_len = 0x200 + iosys_len;
	}
	else if (optS) {
		/* BIOS memory block 0000:0400...0000:05FF */
		blk_seg = 0x40;
		blk_len = 0x0200;
	}
	else if (optM) {
		/* IO.SYS */
		blk_seg = 0x60;
		blk_len = iosys_len;
	}
	else if (optI) {
		/* IDE BIOS work (D800:2000~21FF) */
		blk_seg = 0xd800 + (0x2000 >> 4);
		blk_len = 512;
	}
	else {
		fprintf(stderr, "%s", usagestr);
		return !optHelp;
	}
	
	r.h.ah = 0x3c;			/* creat */
	r.x.cx = 0;
	r.x.dx = FP_OFF(outfile); sr.ds = FP_SEG(outfile);
	intdosx(&r, &r, &sr);
	if (!r.x.cflag) {
		unsigned h = r.x.ax;
		r.h.ah = 0x40;		/* write */
		r.x.bx = h;
		r.x.cx = blk_len;
		r.x.dx = 0; sr.ds = blk_seg;
		intdosx(&r, &r, &sr);
		if (!r.x.cflag) rc = 0;
		r.h.ah = 0x3e;		/* close */
		r.x.bx = h;
		intdos(&r, &r);
	}
	
	return rc;
}
