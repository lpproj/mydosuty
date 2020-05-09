/*
    test1b06: NEC PC-98 HD BIOS TEST (read, sense)
    (public domain)

    to build (with OpenWatcom):
    wcl -q -zq -Fr -s -ms test1b06.c call1b.asm

*/

#include <ctype.h>
#include <dos.h>
#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMP_ASMVER _cdecl _far
#define IMP_ASMFUNC _cdecl

/* we are in the segmented 16bit world... */
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

typedef struct RPACK {
    U16 r_ax, r_bx, r_cx, r_dx, r_si, r_di, r_bp, r_flags;
    U16 r_ds, r_es;
} RPACK;

extern RPACK IMP_ASMVER r_pack0;
extern RPACK IMP_ASMVER r_pack1;

extern U16 IMP_ASMVER r_ax0;
extern U16 IMP_ASMVER r_bx0;
extern U16 IMP_ASMVER r_cx0;
extern U16 IMP_ASMVER r_dx0;
extern U16 IMP_ASMVER r_si0;
extern U16 IMP_ASMVER r_di0;
extern U16 IMP_ASMVER r_bp0;
extern U16 IMP_ASMVER r_flags0;
extern U16 IMP_ASMVER r_ds0;
extern U16 IMP_ASMVER r_es0;
extern U16 IMP_ASMVER r_ax1;
extern U16 IMP_ASMVER r_bx1;
extern U16 IMP_ASMVER r_cx1;
extern U16 IMP_ASMVER r_dx1;
extern U16 IMP_ASMVER r_si1;
extern U16 IMP_ASMVER r_di1;
extern U16 IMP_ASMVER r_bp1;
extern U16 IMP_ASMVER r_flags1;
extern U16 IMP_ASMVER r_ds1;
extern U16 IMP_ASMVER r_es1;
extern U16 IMP_ASMVER private_stack[256];

extern U8  IMP_ASMVER intxx_value;
extern void IMP_ASMFUNC CallIntXX(void);

#define MAX_BUFFERSIZE  16384

void far *buffer_top;
void far *buffer;   /* aligned by 64k */

unsigned wasted_stack_words;


static int do_alloc_64k(int length) {
    unsigned long la, la2;
    unsigned alen = length ? (unsigned)length * 2U : 2;
    char far *m = _fmalloc(alen);
    if (!m) {
        fprintf(stderr, "FATAL: memory allocation failure\n");
        exit(-1);
    }
    buffer_top = m;
    la = (unsigned long)FP_SEG(m) * 16U + FP_OFF(m);
    la2 = la + (unsigned)length;
    if ((la2 >> 16) != (la >> 16)) {
        la = (la + 0x10000UL) & 0xffff0000UL;
        buffer = MK_FP((unsigned)(la >> 4), 0);
    }
    else {
        buffer = m;
    }
    return 0;
}


void disp_rpack(RPACK far *rp)
{
    printf("AX=%04X BX=%04X CX=%04X DX=%04X SI=%04X DI=%04X BP=%04X\nDS=%04X ES=%04X FLAGS=%04X\n"
        , rp->r_ax, rp->r_bx, rp->r_cx, rp->r_dx
        , rp->r_si, rp->r_di
        , rp->r_bp
        , rp->r_ds, rp->r_es
        , rp->r_flags
    );
}

void clear_rpack0(U16 v)
{
    r_ax0 = r_bx0 = r_cx0 = r_dx0 = r_si0 = r_di0 = r_bp0 = v;
    r_ds0 = r_es0 = v;
    r_flags0 = 0x200; /* sti, cld, clc */
}

int CallInt1B(void)
{
    unsigned n;
    intxx_value = 0x1b;
    for(n=0; n<256; ++n) private_stack[n] = 0xffff;
    CallIntXX();
    for(n=0; n<256 && private_stack[n] == 0xffff; ++n)
        ;
    wasted_stack_words = 256 - n;
    return r_flags1 & 1;
}

int int1b_sense(U8 daua, int newsense)
{
    clear_rpack0(0);
    r_ax0 = (newsense ? 0x8400 : 0x0400) | daua;
    return CallInt1B();
}


int do_sense(U8 daua, int newsense, int disp)
{
    int rc;
    if (disp) printf("%sSENSE daua=0x%02X...", newsense ? "NEW " : "", daua);
    rc = int1b_sense(daua, newsense);
    if (disp) {
        if (rc) printf("failure (result=0x%02X)\n", r_ax1 >> 8);
        else {
            const char *s = "unknown";
            printf("success: AH=0x%02X", r_ax1 >> 8);
            switch((r_ax1 >> 8) & 0x0f) {
                case 0: s = "5M"; break;
                case 1: s = "10M"; break;
                case 2: s = "15M"; break;
                case 3: s = "20M"; break;
                case 4: s = "25M"; break;
                case 5: s = newsense ? "25M" : "40M"; break;
                case 7: s = "40M"; break; /* newsense only */
                case 9: s = "ESDI 100M+"; break;
                case 15: s = "IDE 80M+"; break;
            }
            printf(" (%s)\n", s);
            if (newsense) {
                printf("Cylinders=%u Heads=%u Sectors=%u BytesPerSector=%u\n", r_cx1, r_dx1 >> 8, r_dx1 & 0xff, r_bx1);
            }
        }
        printf("used stack(least)=%uwords (%ubytes)\n", wasted_stack_words, wasted_stack_words * 2);
    }
    return rc;
}

static U32 my_getnum(const char *str, int allow_unit)
{
    char *s = NULL;
    U32 base = 1;
    U32 v;

    v = strtoul(str, &s, 0);
    if (v != ULONG_MAX && s && *s) {
        unsigned scale = (toupper(s[1]) == 'I') ? 1024 : 1000;
        switch (toupper(*s)) {
            case 'G': if (allow_unit) base *= scale; /* fallthrough */
            case 'M': if (allow_unit) base *= scale; /* fallthrough */
            case 'K':
                if (allow_unit) {
                    base *= scale;
                    break;
                }
                /* fallthrough */
            default:
                return ULONG_MAX;
        }
    }
    if ((ULONG_MAX / base) <= v) return ULONG_MAX;
    return v * base;
}


U32 Cylinder, Head, Sector, BPS;
int optHelp;
int optN;
int optInvalid;
int prmInvalid;
U8 optCHS;
char *prm_daua;
char *prm_lba;
U16 v_daua;
U32 v_lba = ULONG_MAX;

int my_getopt(int argc, char **argv)
{
    int params = 0;
    while (--argc > 0) {
        char *s = *++argv;
        if (*s == '/' || *s == '-') {
            switch (toupper(s[1])) {
                case '?': optHelp = 1; break;
                case 'N': optN = 1; break;
                case 'C': Cylinder = my_getnum(s+2, 0); optCHS |= 1; break;
                case 'H': Head = my_getnum(s+2, 0); optCHS |= 2; break;
                case 'S': Sector = my_getnum(s+2, 0); optCHS |= 4;  break;
                case 'B': BPS = my_getnum(s+2, 0); break;
                default:
                    optInvalid = 1;
            }
        }
        else {
            if (!prm_daua) {
                prm_daua = s;
                ++params;
                v_daua = my_getnum(s, 0);
                if (v_daua > 0xff) {
                    prmInvalid = 1;
                    return -1;
                }
            }
            else if (!prm_lba) {
                prm_lba = s;
                ++params;
                v_lba = my_getnum(s, 0);
                if (v_lba == ULONG_MAX) {
                    prmInvalid = 1;
                    return -1;
                }
            }
        }
    }
    return params;
}


int do_read(void far *mem, int disp)
{
    int rc;

    if (optCHS) v_daua |= 0x80;
    else v_daua &= 0x7f;

    if (disp) {
        printf("READ daua=0x%02X ", v_daua);
        if (optCHS) {
            printf("C=%lu H=%u S=%u ", (unsigned long)Cylinder, (unsigned)Head, (unsigned)Sector);
        }
        else {
            printf("LBA=0x%lX ", (unsigned long)v_lba);
        }
        printf("%ubytes...", BPS);
    }
    clear_rpack0(0);
    r_ax0 = 0x0600 | v_daua;
    r_es0 = FP_SEG(mem);
    r_bp0 = FP_OFF(mem);
    r_bx0 = BPS;
    if (optCHS) {
        r_cx0 = Cylinder;
        r_dx0 = (Head << 8) | (U8)Sector;
    }
    else {
        r_cx0 = (U16)v_lba;
        r_dx0 = v_lba >> 16;
    }
    rc = CallInt1B();
    if (disp) {
        printf("%s (AH=0x%02X)\n", rc ? "failure" : "ok", r_ax1 >> 8);
        printf("used stack(least)=%uwords (%ubytes)\n", wasted_stack_words, wasted_stack_words * 2);
    }
    return rc;
}


void usage(void)
{
    const char msg[] = 
        "usage: test1b06 [-Bnnn] daua lba_address\n"
        "    or test1b06 -Cnnnn -Hnn -Snn [-Bnnn] daua\n"
        "    or test1b06 [-n] daua\n"
        ;
    printf("%s", msg);
}

int main(int argc, char *argv[])
{
    int rc;

    rc = my_getopt(argc, argv);
    if (optHelp || rc < 1) {
        usage();
        return !!(prmInvalid);
    }
    if (BPS > MAX_BUFFERSIZE) {
        fprintf(stderr, "Sector size parameter error\n");
        return 1;
    }
    do_alloc_64k(BPS);
    if (optCHS) {
        if ((optCHS & 7) != 7) {
            fprintf(stderr, "need all options: -C -H and -S\n");
            return 1;
        }
        if (Cylinder > 65535U || Head > 255U || Sector > 255U) {
            fprintf(stderr, "CHS parameter error\n");
            return 1;
        }
    }
    if (optCHS || prm_lba) {
        if (BPS == 0) {
            int1b_sense(v_daua, 1);
            BPS = r_bx1 ? r_bx1 : 256;
        }
        rc = do_read(buffer, 1);
    }
    else {
        do_sense(v_daua, optN, 1);
    }
    return 0;
}

