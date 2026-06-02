/*
9821L30 - PC-9821 30 lines (text rows) test

Author: lpproj
Target: DOS (PC-9821 only)

Usage: 9821L30 [-m] [20 | 25 | 30]

To build with LSI-C86 3.30:
  lcc 9821L30 -lintlib

with (Open)Watcom:
  wcl -zq -s -os -fr -bt=DOS 9821L30.c -l=dos

with gcc-ia16 + libi86:
  ia16-elf-gcc -Wall -s -mcmodel=small -Os -o 9821L30.exe 9821L30.c -li86


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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) && defined(__ia16__)
#define far __far
#endif


#define bios_b(o) (*(unsigned char far *)MK_FP(0, (o)))
#define bios_w(o) (*(unsigned short far *)MK_FP(0, (o)))
#define iosys_b(o) (*(unsigned char far *)MK_FP(0x60, (o)))
#define iosys_w(o) (*(unsigned short far *)MK_FP(0x60, (o)))

typedef struct CRTINFO {
    int is_98GS;
    int has_21crtbios;
    unsigned char crt_0b_al;
    unsigned char crt_31_al;
    unsigned char crt_31_bh;
    unsigned char bios_flag5;       /* 0000:0458 */
    unsigned char crt_ext_sts;      /* 0000:0459 */
    unsigned char flags_045c;       /* 0000:045C (bit6:9821 ext graphics */
    unsigned char crt_raster;       /* 0000:053B */
    unsigned char crt_sts;          /* 0000:053C */
    unsigned short crt_w_raster;    /* 0000:054A */
    unsigned char prxcrt;           /* 0000:054C */
    unsigned char prxdupd;          /* 0000:054D */
    unsigned char flags_0597;       /* 0000:0597 */
} CRTINFO;

unsigned screen_cols = 80;
int verbose = 0;


#define PAUSE (void)dos_getch()
int dos_getch(void)
{
    union REGS r;
    r.h.ah = 8;
    intdos(&r, &r);
    return r.h.al;
}

void far *xy_to_vram(int x, int y)
{
    static unsigned vram_seg = 0;
    if (vram_seg == 0) {
        vram_seg = iosys_w(0x32);
        if (vram_seg == 0) vram_seg = 0xa000;   /* for EPSON DOS 4.01 */
    }
    return MK_FP(vram_seg, ((y*screen_cols) + x) * 2);
}

void PutANKS(int x, int y, const char *s, int length, unsigned char attr)
{
    unsigned short far *v;

    if (length < 0) length = strlen(s);
    v = xy_to_vram(x, y);
    while(length > 0) {
        *v = (unsigned char)(*s++);
        v[0x1000] = attr;
        ++v;
        --length;
    }
}

int get_crtinfo(CRTINFO *ci)
{
    union REGS r, r0;
    struct SREGS sr;

    ci->bios_flag5 = bios_b(0x0458);
    ci->is_98GS = (ci->bios_flag5 & 0x40) != 0;
    ci->crt_ext_sts = bios_b(0x0459);
    ci->flags_045c = bios_b(0x045c);
    ci->crt_raster = bios_b(0x053b);
    ci->crt_sts = bios_b(0x053c);
    ci->crt_w_raster = bios_w(0x054a);
    ci->prxcrt = bios_b(0x054c);
    ci->prxdupd = bios_b(0x054d);
    ci->flags_0597 = bios_b(0x0597);

    r.x.ax = 0x0b00;
    int86x(0x18, &r, &r, &sr);
    ci->crt_0b_al = r.h.al;

    r0.x.ax = 0x310f;
    r0.h.bh = 0x33;
    int86x(0x18, &r0, &r0, &sr);
    r.x.ax = 0x3100;
    r.h.bh = 0;
    int86x(0x18, &r, &r, &sr);
    ci->has_21crtbios = (r0.h.al == r.h.al && r0.h.bh == r.h.bh);
    ci->crt_31_al = r.h.al;
    ci->crt_31_bh = r.h.bh;

    return ci->has_21crtbios;
}

void
cls_vram(void)
{
    unsigned short far *v = xy_to_vram(0, 0);
    int cnt = 80 * 30;
    unsigned short cc, ca;

    cc = iosys_b(0x119);
    ca = iosys_b(0x114);

    while(cnt > 0) {
        *v = cc;
        v[0x1000] = ca;
        ++v;
        --cnt;
    }
}

int set_screenrows21(CRTINFO *ci, int rows)
{
    union REGS r;
    struct SREGS sr;
    if (!ci->has_21crtbios) return -1;
    r.h.ah = 0x30;
    r.h.al = ci->crt_31_al & 0x0f;
    switch(rows) {
        case 0:
            r.h.bh = ci->crt_31_bh & 0x33;
            break;
        case 20:
            r.h.bh = (ci->crt_31_bh & 0x30);
            break;
        case 25:
            r.h.bh = (ci->crt_31_bh & 0x30) | 0x01;
            break;
        case 30:
            r.h.al |= 0x0c;
            r.h.bh = (ci->crt_31_bh & 0x30) | 0x32;
            break;
        default:
            return -1;
    }
    if (!ci->is_98GS) {
        r.h.al &= 0x0c;
    }
    int86x(0x18, &r, &r, &sr);
    if (r.h.bh != 0) return r.h.bh;
    r.h.ah = 0x0c;
    int86x(0x18, &r, &r, &sr);

    return 0;
}

static const char *b8tos_m(unsigned char b, unsigned char db)
{
#define BTO8S_BUFS 4
    static int ibuf = 0;
    static char bufs[BTO8S_BUFS][10];
    char *s0, *s;
    unsigned char uc;

    s = s0 = bufs[ibuf];
    ibuf = (ibuf + 1) % (BTO8S_BUFS);
    for (uc = 0x80; uc != 0; uc >>= 1, ++s) {
        *s = (db & uc) ? (b & uc) ? '1' : '0'
                       : '-';
    }

    return s0;
}
#define b8tos(b) b8tos_m(b,0xff)

void
put_crtinfo(FILE *fo, const CRTINFO *ci)
{
    fprintf(fo, "[BIOS work memory (for normal CRT)]\n");
    fprintf(fo, "BIOS_FLAG5   0000:0458  %02Xh %s\n", ci->bios_flag5, b8tos_m(ci->bios_flag5, 0x40));
    fprintf(fo, "CRT_EXT_STS  0000:0459  %02Xh %s\n", ci->crt_ext_sts, b8tos(ci->crt_ext_sts));
    fprintf(fo, "FLAGS_045C   0000:045C  %02Xh %s\n", ci->flags_045c, b8tos_m(ci->flags_045c, 0x60));
    fprintf(fo, "CRT_RASTER   0000:053B  %02Xh (%u)\n", ci->crt_raster, ci->crt_raster);
    fprintf(fo, "CRT_STS      0000:053C  %02Xh %s\n", ci->crt_sts, b8tos(ci->crt_sts));
    fprintf(fo, "CRT_W_RASTER 0000:054A  %04Xh (%u)\n", ci->crt_w_raster, ci->crt_w_raster);
    fprintf(fo, "PRXCRT       0000:054C  %02Xh %s\n", ci->prxcrt, b8tos(ci->prxcrt));
    fprintf(fo, "PRXDUPD      0000:054D  %02Xh %s\n", ci->prxdupd, b8tos(ci->prxdupd));
    fprintf(fo, "FLAGS_0597   0000:0597  %02Xh %s\n", ci->flags_0597, b8tos(ci->flags_0597));

    fprintf(fo, "[CRT BIOS]\n");
    fprintf(fo, "int 18h AH=0Bh ->   AL  %02Xh %s\n", ci->crt_0b_al, b8tos(ci->crt_0b_al));
    if (ci->has_21crtbios) {
    fprintf(fo, "int 18h AH=31h ->   AL  %02Xh %s\n", ci->crt_31_al, b8tos_m(ci->crt_31_al, ci->is_98GS ? 0x0f : 0x0c));
    fprintf(fo, "                    BH  %02Xh %s\n", ci->crt_31_bh, b8tos_m(ci->crt_31_bh, 0x33));
    }
    else {
        fprintf(fo, "int 18h AH=0Bh          (not supported)\n");
    }
}

void
disp_sample(int rows, int is_color)
{
    unsigned char attr = 0xe1;
    char s[128];
    int columns = 80;
    int y;

    for(y=0; y<rows; ++y) {
        static char c = 0x20;
        int x, xbase, xend;
        if (is_color) {
            attr = attr + 0x20;
            if ((attr & 0xe0) == 0) attr += 0x20;
        }
        sprintf(s, "%04u:", y);
        xbase = strlen(s);
        xend = xbase + ((rand() / 53) % (columns - xbase - 1)) + 1;
        for(x=xbase; x<xend; ++x) {
            if ((++c) >= 0x7e) c = 0x21;
            s[x] = c;
        }
        PutANKS(0, y, s, xbase, attr ^ 4);
        PutANKS(xbase, y, s + xbase, xend - xbase, attr);
    }
}

void disp_csr(int do_disp)
{
    fprintf(stderr, do_disp ? "\x1b[>5l" : "\x1b[>5h");
}
void disp_sline(int do_disp)
{
    fprintf(stderr, do_disp ? "\x1b[>1l" : "\x1b[>1h");
}

int optHelp;
int optErr;
int optM;
int prmNum;

int main(int argc, char **argv)
{
    int rc;
    int sline_org = iosys_b(0x111);
    int csr_org = iosys_b(0x11b);
    CRTINFO ci, cin;

    while(--argc > 0) {
        char *s = *++argv;
        char c = *s;
        if (c == '-' || c == '/') {
            ++s;
            c = toupper(*s);
            if (c == '?' || c == 'H') optHelp = 1;
            else if (c == 'M') optM = 1;
            else if (c == 'V') ++verbose;
        }
        else {
            char *s2 = s;
            prmNum = (int)strtol(s, &s2, 0);
            if (*s2 != '\0') optErr = 1;
        }
    }

    if (optHelp || optErr) {
        const char msgHelp[] = 
            "Usage (only for PC-9821 series):\n"
            "  9821L30 [-m] [20 | 25 | 30]\n"
            "options:\n"
            "    -m     display sample screen with monochrome\n"
            ;
        if (optErr) fprintf(stderr, "parameter error.\n");
        fprintf(stderr, "%s", msgHelp);
        return optErr;
    }

    get_crtinfo(&ci);
    cin = ci;

    if (prmNum == 0) {
        printf("type \"9821L30 -?\" for usage.\n");
        put_crtinfo(stdout, &ci);
        return 0;
    }

    disp_sline(0);
    disp_csr(0);
    cls_vram();

    rc = set_screenrows21(&ci, prmNum);
    if (rc == 0) {
        get_crtinfo(&cin);
        disp_sample(prmNum, !optM);
        PAUSE;
        cls_vram();
        set_screenrows21(&ci, 0);
    }
    disp_csr(csr_org);
    disp_sline(sline_org);
    if (rc == 0) {
        put_crtinfo(stdout, &cin);
    }
    else {
        if (ci.has_21crtbios) {
            if (rc < 0) fprintf(stderr, "ERROR: bad screen rows\n");
            else fprintf(stderr, "ERROR: int 18h ah=30h failure (BH=%02Xh).\n", rc);
        }
        else {
            fprintf(stderr, "ERROR: int 18h ah=30h not supported.\n");
        }
    }

    return 0;
}
