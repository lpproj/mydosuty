/*
	get55bio - get PC-9801-55 (compatibles) SCSI BIOS
	
	platform:
	  NEC PC-98x1 (normal mode)
	  PC-9801-55 or compatibles (WC33C93/A/B)
	  MS-DOS (2.11 or later)

	license:
	  Public domain
	  (You can use/modify/redistribute freely BUT NO WARRANTY)

	how to build:
	  lcc get55bio      (LSI-C 86 3.30c)
	  tcc get55bio      (Turbo C/C++ for DOS)
	  wcl -s get55bio   (OpenWatcom)
*/

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__WATCOMC__)
#include <i86.h>
#endif

#if defined(__TURBOC__) && __TURBOC__ <= 0x296 /* tc++ 1.01 or below */
# define inp(p) inportb(p)
# define inpw(p) inportw(p)
# define outp(p,v) outportb(p,v)
# define outpw(p,w) outportw(p,v)
# define _enable() enable()
# define _disable() disable()
#endif

#if defined(LSI_C)
#include <machine.h>
#define _enable()	ei()
#define _disable()	di()
#endif

enum {
	WD_NONE = 0,
	WD33C93 = 1,	/* or WD33C92 */
	WD33C93A = 2,
	WD33C93B = 3
};

unsigned wd_base = 0xcc0;


void flush_memory(void)
{
	outp(0x43f, 0xa0);
}

volatile unsigned io_wait_count;

static void io_wait(int n)
{
	char use_tswait = ((*(unsigned char far *)0x480UL & 3)==3);
	while(n-- > 0) {
		if (use_tswait) outp(0x005f, 0);
	}
}

unsigned char wd33_rdreg(unsigned base, unsigned index)
{
	io_wait(1);
	outp(base, (unsigned char)index);
	io_wait(1);
	return inp(base + 2);
}

unsigned char wd33_wrreg(unsigned base, unsigned index, unsigned char value)
{
	io_wait(1);
	outp(base, (unsigned char)index);
	io_wait(1);
	outp(base + 2, value);
	return value;
}

int wd33_chk(unsigned base)
{
	int rc = WD33C93;
	unsigned char b;

	if (inp(base) & 0x08) return WD_NONE;
	_disable();
	b = wd33_rdreg(base, 0);
	_enable();
	if (b & 0x08) rc = WD33C93A;	/* EAF */
	if (b & 0x20) rc = WD33C93B;	/* RAF */

	return rc;
}


void get_55bios(void *mem, unsigned base, unsigned romcount)
{
	unsigned char r30, r31;
	unsigned romseg, rombank;
	char *dst = mem;
	
	_disable();
	r30 = wd33_rdreg(base, 0x30);
	r31 = wd33_rdreg(base, 0x31);
	_enable();
	romseg = (0xd0 + ((r31 & 7)<<1)) * 256U;
	
	printf("get BIOS ROM %04X 4Kx%u=%ubytes (WD33C93 port %04Xh)\n", romseg, romcount, romcount * 4096U, wd_base);
	
	_disable();
	for(rombank = 0; rombank < romcount; ++rombank) {
		const char far *src = MK_FP(romseg, 0);
		unsigned n;
		wd33_wrreg(base, 0x30, (r30 & 0xbf) | (rombank << 6));
		flush_memory();
		for(n=0; n<4096; ++n) *dst++ = src[n];
	}
	wd33_wrreg(base, 0x30, r30);
	flush_memory();
	_enable();
}


int tofile(const char *fname, const void *mem, int len)
{
	FILE *fo = fopen(fname, "wb");
	int wlen;
	
	if (!fo) return -1;
	wlen = fwrite(mem, 1, len, fo);
	fclose(fo);
	
	return len == wlen ? 0 : -1;
}

char *fnbase;
int romcnt = 2;

int main(int argc, char *argv[])
{
	int wdtype;

	while (argc-- > 0) {
		char *s = *++argv;
		if (*s == '-' || *s == '/') {
			char c = toupper(*++s);
			if (c == 'L' || c == '4') romcnt = 4;
			else if (c == 'P') {
				long l = strtol(s + 1, NULL, 16);
				if ((0xffcfU & l) == 0xcc0) wd_base = (unsigned)l;
			}
		}
		else {
			if (!fnbase) fnbase = s;
		}
	}
	
	if (!fnbase) {
		printf(
			"get SCSI BIOS (PC-9801-55 hardware compatibles)\n"
			"usage: get55bio [-pPORT] [-l] basename\n"
			"	-pPORT	port address (-p0CC0 as default)\n"
			"	-l	16Kbytes (4banks) rom\n"
			);
		return 0;
	}
	
	if (fnbase && strlen(fnbase) > 8) {
		fnbase[8] = '\0';
		printf("base name too long - truncated to '%s'\n", fnbase);
	}
	
	wdtype = wd33_chk(wd_base);
	if (wdtype) {
		char *mem = malloc(4 * 4096);
		char fn[14];
		int n;
		if (!mem) {
			fprintf(stderr, "can't alloc memory(16k).\n");
			exit(1);
		}
		get_55bios(mem, wd_base, romcnt);
		for(n=0; n<romcnt; ++n) {
			sprintf(fn, "%s.B%02u", fnbase, n);
			printf("writing %s...", fn); fflush(stdout);
			printf("%s\n", tofile(fn, mem + 4096 * n, 4096) == 0 ? "ok" : "error");
		}
		sprintf(fn, "%s.rom", fnbase);
		printf("writing %s...", fn); fflush(stdout);
		printf("%s\n", tofile(fn, mem, 8192) == 0 ? "ok" : "error");
		if (romcnt >= 4) {
			sprintf(fn, "%s.16k", fnbase);
			printf("writing %s...", fn); fflush(stdout);
			printf("%s\n", tofile(fn, mem, 16384) == 0 ? "ok" : "error");
		}
		free(mem);
	}
	else {
		printf("PC-9801-55 compatible board not found (port %04Xh).\n", wd_base);
		return 1;
	}
	
	return 0;
}
