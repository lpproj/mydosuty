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

#define DEBUG

#define INCLUDE_DUMP_BODY
#include "dumpmem.h"
#define INCLUDE_HELPER_BODY
#include "helper.h"


static void *mymalloc_64k(unsigned size, void **aligned_pointer)
{
	void *p;
	void *p2;
	unsigned long lp, lp2;
	if (size >= (UINT_MAX / 2)) return NULL;
	p = mymalloc(size * 2);
	lp = 16UL * FP_SEG(p) + FP_OFF(p);
	lp2 = (lp + 65535UL) & 0xffff0000UL;
	if (lp + size > lp2) {
		p2 = (char *)p + (unsigned)(lp2 - lp);
	}
	else {
		p2 = p;
	}
	*aligned_pointer = p2;
	return p;
}


typedef struct R_INTCALL {
	unsigned r_flags;
	unsigned r_es;
	unsigned r_ds;
	unsigned r_di;
	unsigned r_si;
	unsigned r_bp;
	unsigned r_sp;
	unsigned r_bx;
	unsigned r_cx;
	unsigned r_dx;
	unsigned r_ax;
	unsigned r_ss;
} R_INTCALL;

#ifdef LSI_C
# define CDECL
#else
# define CDECL cdecl
#endif

extern R_INTCALL far * CDECL my_get_regpack_call(void);
extern R_INTCALL far * CDECL my_get_regpack_ret(void);
extern unsigned char far * CDECL my_get_intcall_invocation(void);
extern unsigned CDECL my_get_flags(void);
extern int CDECL my_intcall_body(void);


static int my_int_or_farcall(unsigned long ent, R_INTCALL *r_call, R_INTCALL *r_ret)
{
	int rc;
	R_INTCALL far *r0 = my_get_regpack_call();
	R_INTCALL far *r1 = my_get_regpack_ret();
	unsigned char far *inst_int = my_get_intcall_invocation();

	myfmemcpy(r0, r_call, sizeof(R_INTCALL));
	if (ent <= 0xffU) {
		inst_int[0] = 0xcd;
		inst_int[1] = (unsigned char)ent;
		inst_int[2] = 0x90;
		inst_int[3] = 0x90;
		inst_int[4] = 0x90;
		inst_int[5] = 0x90;
	}
	else {
		inst_int[0] = 0x9a;
		inst_int[1] = 0x78;
		*(unsigned long far *)(inst_int + 2) = ent;
	}
	rc = my_intcall_body();
	r_call->r_sp = r0->r_sp;
	r_call->r_ss = r0->r_ss;
	myfmemcpy(r_ret, r1, sizeof(R_INTCALL));
	r_call->r_sp = r0->r_sp;
	return rc;
}

int my_intcall(int intno, R_INTCALL *r_call, R_INTCALL *r_ret)
{
	return my_int_or_farcall(intno & 0xffU, r_call, r_ret);
}

int my_farcall(const void * const func, R_INTCALL *r_call, R_INTCALL *r_ret)
{
	return my_int_or_farcall((unsigned long)func, r_call, r_ret);
}


unsigned my_dosver;
unsigned my_truever;

unsigned my_getdosver(void)
{
	R_INTCALL ri;
	unsigned dv, tv;
	ri.r_flags = my_get_flags();
	ri.r_ax = 0x3000;
	my_intcall(0x21, &ri, &ri);
	if ((ri.r_ax & 0xff) == 0) ri.r_ax = 1;
	dv = ri.r_bx = ri.r_ax;
	ri.r_ax = 0x3306;
	my_intcall(0x21, &ri, &ri);
	tv = ri.r_bx;
	if ((tv & 0xff) == 10) tv = 0x1e03;	/* OS/2 1.x as DOS 3.3 */
	my_dosver = ((dv & 0xff) << 8) | ((dv >> 8) & 0xff);
	my_truever = ((tv & 0xff) << 8) | ((tv >> 8) & 0xff);

	return my_dosver;
}


void *dskbuf_top;
unsigned char *dskbuf;

typedef struct INT13LBAPKT {
	unsigned char size_of_packet;
	unsigned char reserved01[1];
	unsigned short count;
	void far *buffer;
	unsigned long lba48l;
	unsigned long lba48h;
} INT13LBAPKT;

typedef struct DOSLBAPKT {
	unsigned long lba;
	unsigned short count;
	void far *buffer;
} DOSLBAPKT;



union REGS r;
struct SREGS sr;

int is_nec98_bios(void)
{
	return *(unsigned short far *)MK_FP(0xffff, 3) == 0xfd80U;
}

int param_unit = -1;
long param_lba = -1L;
int param_count = -1;
int do_with_bios = 1;
int optHelp;
int optO;
char *param_optO = NULL;

static long my_atol(const char *s)
{
	long l = strtol(s, NULL, 0);
	return l;
}
static int my_atoi(const char *s)
{
	long l = strtol(s, NULL, 0);
#if (INT_MAX < LONG_MAX)
	if (l > INT_MAX) {
		errno = ERANGE;
		return INT_MAX;
	}
#endif
#if (INT_MIN > LONG_MIN)
	if (l < INT_MIN) {
		errno = ERANGE;
		return INT_MIN;
	}
#endif
	return (int)l;
}


int readlba_ibmpc(int drive, void far *buf, unsigned long lba, unsigned *length)
{
	INT13LBAPKT pkt;
	R_INTCALL ri;
	int rc;
	memset(&pkt, 0, sizeof(pkt));
	pkt.size_of_packet = 16;
	pkt.count = 1;
	pkt.buffer = buf;
	pkt.lba48l = lba;
	pkt.lba48h = 0;
	ri.r_ax = 0x4200;
	ri.r_dx = drive;
	ri.r_si = FP_OFF(&pkt);
	ri.r_ds = FP_SEG(&pkt);
	rc = my_intcall(0x13, &ri, &ri);
	if (rc < 0) return (ri.r_ax >> 8);
	if (length) *length = 512;
	return 0;
}

int readlba_nec98(int drive, void far *buf, unsigned long lba, unsigned *length)
{
	int rc;
	R_INTCALL ri;
	unsigned sector_size;

	ri.r_ax = 0x8400 | (drive & 0xffU);
	ri.r_bx = 0xffff;
	ri.r_flags = my_get_flags();
	rc = my_intcall(0x1b, &ri, &ri);
	sector_size = ri.r_bx;
	if (sector_size == 0xffff || sector_size == 0) sector_size = 256;
	ri.r_ax = 0x0600 | (drive & 0x7fU);
	ri.r_bx = sector_size;
	ri.r_cx = (unsigned short)lba;
	ri.r_dx = (unsigned short)(lba >> 16);
	ri.r_bp = FP_OFF(buf);	/* LSI-C only */
	ri.r_es = FP_SEG(buf);
	rc = my_intcall(0x1b, &ri, &ri);
	if (rc < 0) return (ri.r_ax >> 8);
	if (length) *length = sector_size;

	return 0;
}

static R_INTCALL rdos_c;
static R_INTCALL rdos_r;
static char clb_ax;
static char clb_bx;
static char clb_cx;
static char clb_dx;
static char clb_si;
static char clb_di;
static char clb_bp;
static char clb_sp;
static char clb_ds;
static char clb_es;
static char clb_ss;
#define chkclb(R) clb_ ## R = rdos_c.r_ ## R != rdos_r.r_ ## R
#define prnclb(R) { if (clb_ ## R ) printf("  " #R ": 0x%04X -> 0x%04X\n", rdos_c.r_ ## R , rdos_r.r_ ## R ); }


static void clrclbs(void)
{
	clb_ax = clb_bx = clb_cx = clb_dx =
	clb_si = clb_di = clb_bp = clb_sp =
	clb_ds = clb_es = clb_ss = 0;
}

static void chkclbs(int stack_offset)
{
	chkclb(ax);
	chkclb(bx);
	chkclb(cx);
	chkclb(dx);
	chkclb(si);
	chkclb(di);
	chkclb(bp);
	clb_sp = rdos_c.r_sp != (rdos_r.r_sp + stack_offset);
	chkclb(ds);
	chkclb(es);
	chkclb(ss);
}

static int get_sector_size(int drive, unsigned *sct)
{
	int rc;
	unsigned sector_size = 0;
	unsigned check_size;

	rdos_c.r_ax = 0x3200;
	rdos_c.r_dx = (drive & 0xff) + 1;
	rdos_c.r_flags = my_get_flags();
	rdos_c.r_ds = rdos_c.r_bx = 0xffffU;
	rc = my_intcall(0x21, &rdos_c, &rdos_c);
	if (rc < 0) rc = rdos_c.r_ax;
	else if (rdos_c.r_bx == 0xffffU)
		sector_size = 0xffffU;
	else
		sector_size = *(unsigned short far *)MK_FP(rdos_c.r_ds, rdos_c.r_bx + 2);

	if (sector_size == 0 || sector_size == 0xffff) {
		unsigned char dpbbuf[64];
		memset(dpbbuf, 0, sizeof(dpbbuf));
		rdos_c.r_ax = 0x7302;
		rdos_c.r_dx = (drive & 0xff) + 1;
		rdos_c.r_flags = my_get_flags();
		rdos_c.r_cx = 63;
		rdos_c.r_si = 0xf1a6;
		rdos_c.r_di = FP_OFF(dpbbuf);
		rdos_c.r_es = FP_SEG(dpbbuf);
		rc = my_intcall(0x21, &rdos_c, &rdos_c);
		if (rc < 0) return rc = rdos_c.r_ax;
		sector_size = *(unsigned short *)(dpbbuf + 2 + 2);
	}

	for(check_size = 32; check_size <= 4096; check_size <<= 1) {
		if (sector_size == check_size) {
			*sct = sector_size;
			return 0;
		}
	}
	return rc ? rc : -1;
}

int readlba_dos(int drive, void far *buf, unsigned long lba, unsigned *length)
{
	int rc;
	unsigned sector_size;

	rc = get_sector_size(drive, &sector_size);
	if (rc != 0) return rc;

	/* modification check */
	clrclbs();
	rdos_c.r_ax = rdos_c.r_bx = rdos_c.r_cx = rdos_c.r_dx = 
	rdos_c.r_si = rdos_c.r_di = rdos_c.r_bp =
	rdos_c.r_ds = rdos_c.r_es = 0xdead;

	if (my_truever >= 0x031f) {
		DOSLBAPKT abspkt;
		abspkt.lba = lba;
		abspkt.count = 1;
		abspkt.buffer = buf;
		rdos_c.r_ax = (drive & 0xffU);
		rdos_c.r_cx = 0xffffU;
		rdos_c.r_bx = FP_OFF(&abspkt);
		rdos_c.r_ds = FP_SEG(&abspkt);
		rc = my_intcall(0x25, &rdos_c, &rdos_r);
	}
	else {
		if (lba > 0xffffUL) return -1;
		rdos_c.r_ax = (drive & 0xffU);
		rdos_c.r_cx = 1;
		rdos_c.r_dx = (unsigned)(lba & 0xffffU);
		rdos_c.r_bx = FP_OFF(buf);
		rdos_c.r_ds = FP_SEG(buf);
		rc = my_intcall(0x25, &rdos_c, &rdos_r);
	}

	chkclbs(2);

	if (rc < 0) return rdos_r.r_ax;
	if (length) *length = sector_size;

	return 0;
}


int readlba_dos_fat32(int drive, void far *buf, unsigned long lba, unsigned *length)
{
	int rc;
	unsigned sector_size;
	DOSLBAPKT abspkt;

	rc = get_sector_size(drive, &sector_size);
	if (rc != 0) return rc;

	abspkt.lba = lba;
	abspkt.count = 1;
	abspkt.buffer = buf;

	clrclbs();
	rdos_c.r_ax = 0x7305;
	rdos_c.r_cx = 0xffffU;
	rdos_c.r_dx = (drive & 0xffU) + 1;
	rdos_c.r_bx = FP_OFF(&abspkt);
	rdos_c.r_ds = FP_SEG(&abspkt);
	rdos_c.r_si = 0;
	rc = my_intcall(0x21, &rdos_c, &rdos_r);

	chkclbs(0);

	if (rc < 0) return rdos_r.r_ax;
	if (length) *length = sector_size;

	return 0;
}


int mygetopt(int argc, char *argv[])
{
	int params = 0;

	while(argc > 0) {
		char *s = *argv;
		char c = *s;
		if (optO && !param_optO) {
			param_optO = s;
		} else if (c == '/' || c == '-') {
			switch(*++s) {
				case '?': case 'H': case 'h':
					optHelp = 1;
					break;
				case 'O': case 'o':
					optO = 1;
					++s;
					if (*s == ':' || *s == '=') ++s;
					if (*s) param_optO = s;
					break;
			}
		}
		else {
			if (params == 0) {
				if (isalpha(*s) && s[1] == ':') {
					param_unit = toupper(*s) - 'A';
					do_with_bios = 0;
				}
				else {
					param_unit = my_atoi(s);
				}
				++params;
			}
			else if (params == 1) {
				param_lba = my_atol(s);
				++params;
			}
			else if (params == 2) {
				param_count = my_atoi(s);
				++params;
			}
		}
		++argv;
		--argc;
	}
	if (params == 2) param_count = 1;

	return params;
}

int main(int argc, char *argv[])
{
	FILE *fo = NULL;
	unsigned dumplen;
	int params;
	int isnec98;
	void *abuf;

	my_getdosver();
	dskbuf_top = mymalloc_64k(4096, &abuf);
	dskbuf = abuf;
	memset(dskbuf, 0xff, 4096);
	params = mygetopt(argc - 1, argv + 1);
	isnec98 = is_nec98_bios();

	if (params < 2) {
		printf(
		    "usage: dumpdsk [-o filename] unitno lba count\n"
		    "option:  -o filename   output raw image to filename\n"
		    "ex (read MBR in 1st HD): dumpdsk 0x80 0 1\n"
		);
	}
	if (param_optO) {
		fo = fopen(param_optO, "wb");
		if (!fo) {
			fprintf(stderr, "FATAL: can't open \"%s\".\n", param_optO);
			exit(1);
		}
	}

	while(param_count > 0) {
		int status;
		printf("read LBA 0x%lx...", param_lba);
		if (do_with_bios) {
			status = isnec98 ? readlba_nec98(param_unit, dskbuf, param_lba, &dumplen)
			                 : readlba_ibmpc(param_unit, dskbuf, param_lba, &dumplen);
		}
		else {
			status = readlba_dos(param_unit, dskbuf, param_lba, &dumplen);
			if (my_truever >= 0x070a && status == 0x0207) {
				status = readlba_dos_fat32(param_unit, dskbuf, param_lba, &dumplen);
			}
		}
		if (status == 0) {
			printf("success (sector_size=%u)\n", dumplen);
			if (fo) {
				fwrite(dskbuf, 1, dumplen, fo);
			}
			else {
				dumpmem(dskbuf, dumplen, 0);
				printf("\n");
			}
		}
		else {
			printf("failure (result=0x%02X)\n", status);
		}
		--param_count;
		++param_lba;
	}
	if (fo) fclose(fo);

	if (clb_ax + clb_bx + clb_cx + clb_dx + clb_si + clb_di + clb_bp + clb_sp + clb_ds + clb_es + clb_ss) {
		printf("modified register(s):\n");
		prnclb(ax);
		prnclb(bx);
		prnclb(cx);
		prnclb(dx);
		prnclb(si);
		prnclb(di);
		prnclb(bp);
		prnclb(sp);
		prnclb(ds);
		prnclb(es);
		prnclb(ss);
		printf("\n");
	}

	return 0;
}
