/*
    mtype98 - check int DCh CL=12h
             (Get MS-DOS product version and Machine Type)

    target: MS-DOS for PC-98x1 (or EPSON compatibles)

    to build with (Open)Watcom:
        wcl -zq -s -bt=dos mtype98.c

    to build with LSI-C86 3.30c(eval):
        lcc mtype98 -lintlib -ltinymain.obj


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


#include <dos.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*

0000:0481 BYTE bit6,3 keyboard type
6 3
----+---
1 1 | new keyboard with NUM (dip2-7 off)
0 1 | new keyboard with NUM (dip2-7 on)
1 0 | new keyboard without NUM
0 0 | old keyboard

0000:0500 WORD machine type
13 12 11  0
------------+----
 0  0  0  0 | PC-9801 original
 1  0  0  0 | E/F/M
 1  1  0  1 | U
 1  0  0  1 | PC-98x1 normal
 0  0  1  0 | PC-98XA
 1  0  1  0 | hi-res except (PC-98XA)
 1  1  0  0 | PC-98LT/HA, DB-P1


int DCh CL=12h
(20200207~0210: tested on NEC MS-DOS 6.2)
 0000:0500 word |0481|0487|0458|result 
  b13 12 11   0 | b6 |byte| b7 |  DX   | description
 ---------------+----+----+----+-------+----------------------
    0  0  0   0 |  0 |    |  0 | 0000h | PC-9801 the original
    1  0  0   0 |  0 |    |  0 | 0001h | PC-9801E/F/M
    1  1  0   1 |  0 |    |  0 | 0002h | PC-9801U
    1  0  0   1 |  0 |    |  0 | 0003h | PC-98x1 normal (old key mode?)
    1  0  0   1 |  1 |    |  0 | 0004h | PC-98x1 normal (new key mode?) [*1]
    1  1  0   0 |    |    |  0 |(0004h)| PC-98LT, PC-98HA, DB-P1
          0     |  1 |    |  0 |(0004h)| PC-98x1 normal (new key mode?) (NEC MS-DOS 5.0+)
          1     |    |    |  0 |(0101h)| (PC-98 hi-res)
    0  0  1   0 |  0 |    |  0 | 0100h | PC-98XA
    0  0  1   0 |  1 |    |  0 | 0101h | PC-98XA? [*2]
    1  0  1   0 |    |    |  0 | 0101h | PC-98 hi-res (except PC-H98, PC-98XA)
          1     |    |    |  0 |(0101h)| (PC-98 hi-res)
          0     |    |!04 |  1 | 1004h | PC-H98 normal (other than i486)
          0     |    | 04 |  1 | 1005h | PC-H98 normal (i486) [*3]
          1     |    |!04 |  1 | 1101h | PC-H98 hi-res (other than i486)
          1     |    | 04 |  1 | 1102h | PC-H98 hi-res (i486) [*4]

(20200208: tested on NEC MS-DOS 3.3D, 5.0A-H, EPSON MS-DOS 5.0/6.2)
  *1 DX=0003h on EPSON MS-DOS 5.0/6.2
     DX=0003h on NEC MS-DOS 3.3D with [0000:0458] bit6=0
     DX=0004h on NEC MS-DOS 3.3D with [0000:0458] bit6=1 (PC-98GS)
  *2 DX=0100h on NEC MS-DOS 3.3D, EPSON MS-DOS 5.0/6.2
  *3 DX=1004h on EPSON MS-DOS 5.0/6.2
  *4 DX=1101h on EPSON MS-DOS 5.0/6.2

*/

union REGS r;
struct SREGS sr;

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;
#define peek0(o) *(const U8 far *)MK_FP(0,o)
#define poke0(o,v) (*(U8 far *)MK_FP(0,o) = (v))
#define peek0w(o) *(const U16 far *)MK_FP(0,o)
#define poke0w(o,v) (*(U16 far *)MK_FP(0,o) = (v))

unsigned
put_intdc_machine(void)
{
	char *s;

	r.x.cx = 0x0012;
	r.x.ax = r.x.dx = -1;
	int86(0xdc, &r, &r);
	printf("int DCh CL=12h: AX=%04X DX=%04X\n", r.x.ax, r.x.dx);
	switch(r.x.dx) {
		case 0x0000: s = "PC-9801 original"; break;
		case 0x0001: s = "PC-9801E/F/M"; break;
		case 0x0002: s = "PC-9801U"; break;
		case 0x0003: s = "PC-9801 normal"; break;
		case 0x0004: s = "PC-98x1/PC-98GS normal"; break;
		case 0x0100: s = "PC-98XA"; break;
		case 0x0101: s = "PC-98 hi-res (except PC-98XA, PC-H98)"; break;
		case 0x1000:
		case 0x1001:
		case 0x1002:
			s = "PC-H98 normal?"; break;
		case 0x1004: s = "PC-H98 normal (386,pentium)"; break;
		case 0x1005: s = "PC-H98 normal (486)"; break;
		case 0x1100: s = "PC-H98 hi-res?"; break;
		case 0x1101: s = "PC-H98 hi-res (386,pentium)"; break;
		case 0x1102: s = "PC-H98 hi-res (486)"; break;
		default:
			s = "unknown";
	}
	printf("  machine type: %s\n", s);
	return r.x.dx;
}

static
char * myitob_mask(unsigned long v, size_t digits, unsigned long mask)
{
#define ITOB_NUMBER_OF_BUFFERS 4
	static unsigned cnt = ITOB_NUMBER_OF_BUFFERS - 1;
	static char buf[ITOB_NUMBER_OF_BUFFERS][36];
	char *s;
	unsigned n;

	if (digits >= sizeof(buf[0]) - 1) return NULL;
	if (++cnt >= ITOB_NUMBER_OF_BUFFERS) cnt = 0;
	for(n=digits, s=buf[cnt]; n-->0; ++s) {
		unsigned long l = 1 << n;
		if (l&mask) *s = (v&l) ? '1' : '0';
		else *s = '-';
	}
	*s = '\0';

	return buf[cnt];
}
static
char * myitob(unsigned long v, size_t digits)
{
	return myitob_mask(v, digits, 0xffffffffUL);
}


void
put_mtype_workarea(void)
{
	char *s;
	U16 v;

	v = peek0(0x0458);
	printf("0000:0458   %02X         %s", v, myitob_mask(v,8,0xcf));
	if (v & 0xc0) {
		printf(" (");
		if (v & 0x80) printf("H98");
		if (v & 0x40) {
			if (v & 0x80) printf(", ");
			printf("PC-98GS");
		}
		printf(")");
	}
	printf("\n");

	v = peek0(0x0481);
	switch(v & 0x48) {
		case 0x48: s = "new keyboard"; break;
		case 0x40: s = "new keyboard without NUM key"; break;
		case 0x08: s = "new keyboard, DIPSW 2-7 on"; break;
		default:
			s = "old keyboard";
	}
	printf("0000:0481   %02X         %s (%s)\n", v, myitob_mask(v,8,0xcf),s);

	v = peek0w(0x0486);
	printf("0000:0486 %04X", v);
	if (v) {
		U8 t = (v >> 8) & 0x0f;
		if (t>=3 && t<10) printf("                  (i%d86 compatible)", t);
		
	}
	printf("\n");

	v = peek0w(0x0500);
	switch(v & 0x3801) {
		case 0x0000: s = "PC-9801 original"; break;
		case 0x0800: s = "PC-98XA"; break;
		case 0x2000: s = "PC-9801E/F/M"; break;
		case 0x2001: s = "PC-98x1 normal"; break;
		case 0x2800: s = "PC-98 hi-res (except PC-98XA)"; break;
		case 0x3000: s = "PC-98LT/HA or DB-P1"; break;
		case 0x3001: s = "PC-9801U"; break;
		default:
			s = "???";
	}
	printf("0000:0500 %04X %s (%s)\n", v, myitob_mask(v, 16, 0x3801), s);
}

void
put_fake_machine(U16 val0500, U8 val0481, U8 val0487, U8 val0458)
{
	U16 prev0500 = peek0w(0x0500);
	U8 prev0481 = peek0(0x0481);
	U8 prev0487 = peek0(0x0487);
	U8 prev0458 = peek0(0x458);
	
	poke0w(0x0500, (val0500 & 0x3801) | (prev0500 & ~0x3801U));
	poke0(0x481, (val0481 & 0xcf) | (prev0481 & ~0xcfU));
	poke0(0x487, val0487);
	poke0(0x458, val0458);
	put_mtype_workarea();
	put_intdc_machine();
	poke0w(0x0500,prev0500);
	poke0(0x0481,prev0481);
	poke0(0x0487,prev0487);
	poke0(0x0458,prev0458);
}

unsigned myatou(const char *s)
{
	long l = strtol(s, NULL, 0);
	return l > UINT_MAX || l < 0 ? UINT_MAX : (unsigned)l;
}

int main(int argc, char *argv[])
{
	U16 v500;
	U8 v481, v487, v458;

	if (argc <= 1) {
		put_mtype_workarea();
		put_intdc_machine();
		printf("type MTYPE98 -? to help\n");
		return 0;
	}
	if (argv[1][0] == '-' || argv[1][0] == '/') {
		printf("usage: mtype98 val_0500 val_0481 val_0487 val_0458\n");
		return 1;
	}
	v500 = peek0w(0x0500);
	v481 = peek0(0x0481);
	v487 = peek0(0x0487);
	v458 = peek0(0x0458);
	if (argc > 1) v500 = myatou(argv[1]);
	if (argc > 2) v481 = myatou(argv[2]);
	if (argc > 3) v487 = myatou(argv[3]);
	if (argc > 4) v458 = myatou(argv[4]);
	put_fake_machine(v500, v481, v487, v458);
	return 0;
}
