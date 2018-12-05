/*

description: read LGY98 EEPROM

to build (with OpenWatcom):
    wcl -zq -bt=dos lgy98rom.c


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

#include <conio.h>
#include <ctype.h>
#include <dos.h>
#if defined(__WATCOMC__)
# include <i86.h>
#endif
#include <io.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#if defined(__TURBOC__)
# define _enable    enable
# define _disable   disable
# define _inp       inportb
# define _inpw      inport
# define _outp      outportb
# define _outpw     outport
#elif defined(__WATCOMC__)
# define _inp       inp
# define _inpw      inpw
# define _outp      outp
# define _optpw     outpw
#endif


/*

https://sites.google.com/site/np21win/docs/lgy-98

BASE + 0x030d

  READ instruction:

            A  4 C 6 E 6 E 4 C  4
  bit 3 SK  1  0 1 0 1 0 1 0 1  0
      2 CS  0  1 1 1 1 1 1 1 1  1
      1 DI  1  0 0 1 1 1 1 0 0  0
      0 DO? 0  0 0 0 0 0 0 0 0  0

ATMEL AT93C46
Datasheet http://ww1.microchip.com/downloads/en/devicedoc/doc5140.pdf

tCSS (min)  50ns
tSKH (min) 250ns
tSKL (min) 250ns
fSK  (max) 1Mhz(tSKH + tSKL = 1us)
tDIS (min) 100ns
tPD0 (max) 250ns
tPD1 (max) 250ns


*/

unsigned addrbits = 6;  /* 9346 */

static void my_iowait(void)
{
    if ((*(unsigned short far *)MK_FP(0, 0x500) & 0x2801) == 0) {
        /* PC-9801 the original/E/F/M */
        volatile unsigned n = 3;
        while(n-- > 0)
            ;
    }
    else {
        _outp(0x5f, 0); /* wait approx. 600ns */
    }
}

static void my_outp(unsigned port, unsigned char value)
{
    _outp(port, value);
    my_iowait();
}


static void send_opparam(unsigned port, unsigned bits, unsigned mask)
{
    while(mask) {
        unsigned char data = (bits & mask) ? 6 : 4;     /* CS | DI | (SK=0) */
        my_outp(port, data);
        my_outp(port, data | 0x08);                     /* CS | DI | (SK=1) */
        mask >>= 1;
    }
}

unsigned
lgy_cmd9346(unsigned port, unsigned opcode, unsigned addr, unsigned data)
{
    unsigned rc = 0;
    unsigned bits1 = ((4 | (opcode & 3)) << addrbits) | addr;
    unsigned mask1 = 1 << (addrbits + 3);
    
    _disable();
    
#if 1
    if (opcode != 2) {
        fprintf(stderr, "command not supprted.\n");
        return (unsigned)-1;
    }
#endif

    my_outp(port, 0x0a);  /* deassert CS, SK=H (for safe) */
    send_opparam(port, bits1, mask1);
    switch(opcode) {
        case 2:     /* READ */
        {
            unsigned cnt = 17; /* leading '0' and trailing 16bits (D15->D0) */
            while(cnt) {
                rc <<= 1;
                /* note: DI=1 for np21w (not required for real LGY-98 board) */
                my_outp(port, 0x04 | 0x02);        /* CS | (DI=1) | (SK=0) */
                rc |= (_inp(port) & 1);
                my_outp(port, 0x04 | 0x02 | 0x08); /* CS | (DI=1) | (SK=1) */
                --cnt;
                my_iowait();
            }
            my_outp(port, 0);   /* deassert CS */
        }
        break;
    }
    
    _enable();
    return rc;
}

static int lgy_checkbase(unsigned base)
{
    unsigned char p30a, p30b, p30c;
    
    p30a = _inp(base + 0x30a);
    my_iowait();
    p30b = _inp(base + 0x30b);
    my_iowait();
    p30c = _inp(base + 0x30c);
    my_iowait();
    
    return (p30a == 0x00 && p30b == 0x40 && p30c == 0x26);
}

unsigned lgy_getbase(void)
{
    static const unsigned b[] = { 0x00d0, 0x10d0, 0x20d0, 0x30d0, 0x40d0, 0x50d0, 0x60d0, 0x70d0, 0 };
    unsigned n = 0;
    
    while(b[n]) {
        if (lgy_checkbase(b[n])) return b[n];
        ++n;
    }
    return 0;
}


void
lgy_geteeprom(unsigned base, void *buffer)
{
    unsigned short *p = buffer;
    unsigned n;
    
    for(n=0; n<64; ++n) {
        p[n] = lgy_cmd9346(base + 0x030d, 2, n, 0);
    }
}


void dumpwords(const void *mem, unsigned count)
{
    const unsigned ENTRY=8;
    const unsigned short *p = mem;
    unsigned n = 0;
    
    while(n<count) {
        if ((n % ENTRY) == 0) printf("%04X ", n);
        printf(" %04X", p[n]);
        ++n;
        if ((n % ENTRY) == 0) printf("\n");
    }
    if ((count % ENTRY) != 0) printf("\n");
}

void ne2k_dumpregs(unsigned base)
{
    const unsigned ENTRY=16;
    unsigned char r[64];
    unsigned char cr;
    unsigned page, index;
    unsigned n;
    
    _disable();
    cr = _inp(base);
    for(page=0; page<4; ++page) {
        my_outp(base, (cr & 0x3f) | (page << 6));
        for(index=0; index<16; ++index) {
            r[(page << 4) | index] = _inp(base + index);
            my_iowait();
        }
    }
    my_outp(base, cr);
    _enable();

    n = 0;
    while(n<sizeof(r)) {
        if ((n % ENTRY) == 0) printf("Page%u", (n>>4));
        printf(" %02X", r[n]);
        ++n;
        if ((n % ENTRY) == 0) printf("\n");
    }
}


void lgy_dumpeeprom(unsigned base)
{
    unsigned short r[64];
    
    lgy_geteeprom(base, r);
    dumpwords(r, sizeof(r)/sizeof(r[0]));
}

int main(int argc, char *argv[])
{
    unsigned base;
    
    base = lgy_getbase();
    printf("LGY98 base addr: %04X\n", base);
    if (!base) return 1;

    printf("NE2K registers:\n");
    ne2k_dumpregs(base);
    printf("\nEEPROM:\n");
    lgy_dumpeeprom(base);

    return 0;
}


