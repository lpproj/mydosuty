/*
    pc98tick - test PC-98 timers

    target: MS-DOS for PC-98x1 (or EPSON compatibles)

    legend:
    i8253    - PIT, counted by 10ms (all of PC-98 series)
    hitimer  - IDE timer, counted by 32ndsecs (PC-9821 with IDE HD)
    ARTIC    - 307.2KHz time stamper, counted by 3.26us (PC-9821, PC-9801fellow, H98)

    to build with (Open)Watcom:
        nasm -f obj -o pc98thdl.obj pc98thdl.asm
        wcl -zq -s -bt=dos pc98tick.c pc98thdl.obj

    to build with Turbo C/C++:
        nasm -f obj -o pc98thdl.obj pc98thdl.asm
        tcc pc98tick.c pc98thdl.obj

    to build with LSI-C86 3.30c(eval):
        nasm -DLSI_C -f obj -o pc98thdl.obj pc98thdl.asm
        lcc pc98tick.c pc98thdl.obj -lintlib -ltinymain.obj


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
#include <dos.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined __TURBOC__
# define CDECL cdecl
# define FAR far
# define inp(p) inportb(p)
# define inpw(p) inport(p)
# define outp(p,v) outportb(p,v)
static void my_enable(void) { enable(); }
static void my_disable(void) { disable(); }
#elif defined LSI_C
static void my_enable(void) { ei(); }
static void my_disable(void) { di(); }
# define CDECL
# define FAR far
#else /* MS, Watcom */
# define CDECL _cdecl
# define FAR _far
static void my_enable(void) { _enable(); }
static void my_disable(void) { _enable(); }
#endif


extern void CDECL FAR new_int08(void);
extern void FAR * CDECL FAR org_int08;
extern unsigned long CDECL FAR timer_ticks;
extern unsigned short CDECL FAR pit_rearm;
extern unsigned char CDECL FAR do_chain;

static union REGS r;
static struct SREGS sr;
static int hasARTIC;
static int hasHitimer;
static int hasWait5F;
static int is8MHz;
static volatile int isPC98orgEFM;
volatile int dummy_counter;

unsigned char prev_imr;


void get98env(void)
{
    const unsigned char m458 = *(unsigned char FAR *)MK_FP(0, 0x458);
    const unsigned char m45b = *(unsigned char FAR *)MK_FP(0, 0x45b);
    const unsigned char m500 = *(unsigned char FAR *)MK_FP(0, 0x500);
    const unsigned char m501 = *(unsigned char FAR *)MK_FP(0, 0x501);

    is8MHz = (m501 & 0x80) != 0;
    isPC98orgEFM = ((m500 & 1)==0) && ((m501 & 0x18)==0);
    if (m458 & 0x80) {  /* H98 NESA system */
        hasARTIC = hasWait5F = 1;
    }
    else {              /* Normal, or hi-res on non-NESA system */
        hasWait5F = (m45b & 0x80) != 0;
        hasARTIC = (m45b & 0x04) != 0;
    }
    r.x.ax = 0x80ff;
    r.x.cx = 0xffff;
    int86x(0x1c, &r, &r, &sr);
    hasHitimer = (r.h.al <= 3 && r.x.cx != 0xffff);

    /*
        PIT counter for 10ms tick
        8MHz: 1cnt = (1M / 1.9968M) = 0.5008012820512821 (us)
        0.5008012820512821 * 19968 = 10000 (us) = 10ms
        5MHz: 1cnt = (1M / 2.4576M) = 0.4069010416666667 (us)
        0.4069010416666667 * 24576 = 10000 (us) = 10ms
    */
    pit_rearm = is8MHz ? 19968 : 24576;
}

static void io_wait(void)
{
    if (hasWait5F || isPC98orgEFM) {
        outp(0x5f, 0);
    }
    else {
        /* jmp short $*2 */
    }
}

int hook_timer(void)
{
    if (org_int08) return -1;
    prev_imr = inp(0x02);
    r.x.ax = 0x3508;
    intdosx(&r, &r, &sr);
    org_int08 = MK_FP(sr.es, r.x.bx);
    r.x.ax = 0x2508;
    r.x.dx = FP_OFF(new_int08);
    sr.ds = FP_SEG(new_int08);
    intdosx(&r, &r, &sr);
    return 0;
}

void start_timer(int use_bios)
{
    if (use_bios) {
        r.x.ax = 0x3507;
        intdosx(&r, &r, &sr);   /* es:bx = vect(7) */
        r.h.ah = 0x02;
        r.x.cx = 0xffff;
        int86x(0x1c, &r, &r, &sr);
    }
    else {
        unsigned char imr;
        my_disable();
        outp(0x77, 0x36);
        imr = inp(0x02);
        outp(0x71, pit_rearm & 0xff);
        io_wait();
        outp(0x71, pit_rearm >> 8);
        io_wait();
        outp(0x02, imr & 0xfe);
        my_enable();
    }
}

int unhook_timer(void)
{
    outp(0x02, prev_imr);
    r.x.ax = 0x2508;
    r.x.dx = FP_OFF(org_int08);
    sr.ds = FP_SEG(org_int08);
    intdosx(&r, &r, &sr);
    return 0;
}

void iowait(void)
{
    if (isPC98orgEFM) {
        ++dummy_counter;
    }
    else
        outp(0x5f, 0);
}

int peek_or_getkey(void)
{
    r.x.ax = 0x0b00;
    intdos(&r, &r);
    if (r.h.al == 0) return -1;
    r.h.ah = 0x06;
    r.h.dl = 0xff;
    intdos(&r, &r);
    return r.h.al;
}

static unsigned ht_days = 0;
static unsigned long ht_prev;

unsigned long get_ht24(void)
{
    unsigned long ht = *(unsigned long FAR *)MK_FP(0, 0x4f1) & 0x3fffffUL;
    if (ht < ht_prev) ++ht_days;
    ht_prev = ht;
    return ht;
}

static unsigned long ar_prev24;
static unsigned ar_2432;
unsigned long get_artic32(void)
{
    unsigned long ar;
    unsigned tl0, tl;
    unsigned th;

    my_disable();
    tl0 = inpw(0x5c);
    th = inpw(0x5e);
    tl = inpw(0x5c);
    if (tl < tl0) th = inpw(0x5e);
    my_enable();
    ar = ((unsigned long)(th & 0xff00U) << 8) | tl;
    if (ar_prev24 > ar) ar_2432 += 0x0100;
    ar_prev24 = ar;
    return ar | ((unsigned long)ar_2432 << 16);
}


/* -------------------------------- */

int optA = 1;
int optB;
int optV;
int optHelp;

static int getnum(const char *s)
{
    char c = *s;
    if (c == '-' && !s[1]) return 0;
    if (!c || (c == '+' && !s[1])) return 1;
    return atoi(s+1);
}

static
int my_getopt(int argc, char **argv)
{
    while(argc-- > 1) {
        char *s = *++argv;
        if (*s == '-' || *s == '/') {
            switch(s[1]) {
                case '?': case 'H': case 'h':
                    optHelp = 1;
                    break;
                case 'A': case 'a': optA = getnum(s+2); break;
                case 'B': case 'b': optB = getnum(s+2); break;
                default:
                    break;
            }
        }
    }
    return 0;
}

static
void usage(void)
{
    const char msg[] =
        "usage: pc98tick [-a-] [-b]"
        "\n"
        "  -a-  do not disp unsupported timer ticks\n"
        "  -b   set PIT via timer BIOS (int 1Ch, ah=2)\n"
        ;
    printf("%s", msg);
}

int main(int argc, char **argv)
{
    int disp_all;
    int init_pit_bios;
    unsigned long ht_base;
    unsigned long ar_base32;

    get98env();
    my_getopt(argc, argv);
    if (optHelp) {
        usage();
        return 0;
    }
    disp_all = optA;
    init_pit_bios = optB;

    printf("sysclk:%dMHz Wait5F:%d ARTIC:%d hitimer:%d\n", is8MHz ? 8:5, hasWait5F, hasARTIC, hasHitimer);

    hook_timer();
    start_timer(init_pit_bios);
    printf("press any key to exit...");
    fflush(stdout);
    printf("\n");
    ht_base = (hasHitimer || disp_all) ? get_ht24() : 0;
    ar_base32 = (hasARTIC || disp_all) ? get_artic32() : 0;
    while(peek_or_getkey() == -1) {
        printf("\ri8253:%lu.%02u", (timer_ticks / 100U), (unsigned)(timer_ticks % 100U));
        if (hasHitimer || disp_all) {
            unsigned long ht = get_ht24() - ht_base;
            printf(" hitimer:%lu.%02u", (ht / 32U), ((ht % 32U) * 100U) / 32U);
        }
        if (hasARTIC || disp_all) {
            unsigned long ar = get_artic32() - ar_base32;
            unsigned long aru = ar % 307200UL;
            printf(" ARTIC:%u.%06lu", (unsigned)(ar / 307200UL), (aru * 10000U) / 3072U);
        }
        printf(" ");
        fflush(stdout);
    }
    printf("\n");
    unhook_timer();
    return 0;
}

