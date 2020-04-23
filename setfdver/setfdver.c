/*
setfdver: Fake FreeDOS kernel version (like VERSION directive in CONFIG.SYS)

to build
  with OpenWatcom:  wcl -zq -Fr -s -bt=dos setfdver.exe -l=dos
  with Turbo C++:   tcc setfdver.c
  with LSI-C86:     lcc setfdver.c -lintlib -ltinymain.obj
  with gcc-ia16:    ia16-elf-gcc -s -Os -o setfdver.exe setfdver.c -li86


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

#if defined __GNUC__
/* ia16-elf-gcc */
# define far __far
#endif

/* version information of various DOS */

union REGS r;
struct SREGS sr;

static unsigned char oem_id;
static unsigned char v_maj, v_min;
static unsigned char true_maj, true_min, true_flags, true_rev;
static unsigned serial_h, serial_l;
static unsigned rev_drdos;
static unsigned v_romdos;
static const char far *s_romdos;
static const char far *s_fdkernel;

void query_and_disp_dosver(int disp)
{
    r.x.ax = 0x3306;
    r.x.bx = 0;
    r.x.dx = 0xffff;
    intdos(&r, &r);
    if (r.h.dl != 0xff) {
        true_maj = r.h.bl;
        true_min = r.h.bh;
        true_flags = r.h.dh;
        true_rev = r.h.dl & 7;
    }
    r.x.ax = 0x3000;
    r.x.bx = 0;
    intdos(&r, &r);
    oem_id = r.h.bh;
    v_maj = r.h.al;
    v_min = r.h.ah;
    serial_h = r.h.cl;
    serial_l = r.x.cx;
    /* DR DOS revision */
    r.x.ax = 0x4452;
    r.x.dx = 0x0080;
    intdos(&r, &r);
    if (!r.x.cflag && r.x.dx != 0x0080) {
        rev_drdos = r.x.ax;
    }
    /* FreeDOS kernel revision */
    if (oem_id == 0xfd) {
        r.x.ax = 0x33ff;
        r.x.dx = 0;
        intdos(&r, &r);
        if (!r.x.cflag && r.x.dx != 0) {
            s_fdkernel = MK_FP(r.x.dx, r.x.ax);
        }
    }
    /* Datalight ROM-DOS revision */
    r.x.ax = 0x30db;
    r.x.si = 0xb2d2;
    r.x.di = 3;
    r.x.bx = r.x.cx = r.x.dx = 0;
    sr.es = 0;
    intdosx(&r, &r, &sr);
    if (!r.x.cflag && r.x.cx == 0xb2d2) {
        v_romdos = r.x.dx;
        s_romdos = MK_FP(sr.es, r.x.bx);
    }

    if (disp) {
        char *s;
        printf("DOS Version      %d.%02d", v_maj, v_min);
        if (v_maj == 10) printf(" (OS/2 1.x)");
        else if (v_maj == 20) printf(" (OS/2 2.x or later)");
        printf("\n");
        if (true_maj) {
            printf("True Version     %d.%02d", true_maj, true_min);
            if (true_maj == 5 && true_min == 50) printf(" (NTVDM)");
            printf("\n");
        }
        if (rev_drdos) {
            switch(rev_drdos) {
                case 0x1041: s = "DOS Plus 1.2"; break;
                case 0x1060: s = "DOS Plus 2.x"; break;
                case 0x1063: s = "DR DOS 3.41"; break;
                case 0x1064: s = "DR DOS 3.42"; break;
                case 0x1065: s = "DR DOS 5"; break;
                case 0x1067: s = "DR DOS 6"; break;
                case 0x1070: s = "PalmOS"; break;
                case 0x1071: s = "DR DOS 6.0 Mar 1993"; break;
                case 0x1072: s = "Novell DOS 7.0, OpenDOS 7.01"; break;
                case 0x1073: s = "DR-DOS 7.02, 7.03"; break;
                case 0x1432: s = "Concurrent PC DOS 3.2"; break;
                case 0x1441: s = "Concurrent DOS 4.1"; break;
                case 0x1450: s = "Concurrent DOS/XM 5.0 or Concurrent DOS/386 1.1"; break;
                case 0x1460: s = "Concurrent DOS/XM 6.0 or Concurrent DOS/386 2.0"; break;
                case 0x1462: s = "Concurrent DOS/XM 6.2 or Concurrent DOS/386 3.0"; break;
                case 0x1466: s = "DR Multiuser DOS 5.1, CCT Multiuser DOS 7.x"; break;
                case 0x1467: s = "Concurrent DOS 5.1"; break;
                default:
                    s = "unknown";
            }
            printf("DR-DOS revision  0x%04X (%s)\n", rev_drdos, s);
        }
        if (v_romdos) {
            int i;
            printf("ROM-DOS version  %d.%02d (", v_romdos >> 8, v_romdos & 0xff);
            for (i=0; s_romdos[i] && i<256; ++i) printf("%c", s_romdos[i]);
            printf(")\n");
        }
        printf("OEM ID           0x%02X", oem_id);
        switch(oem_id) {
            case 0x00: s = "IBM"; break;
            case 0x01: s = "Compaq"; break;
            case 0x04: s = "AT&T"; break;
            case 0x05: s = "Zenith DOS 3.3"; break;
            case 0x06:
            case 0x4d:
                s = "HP"; break;
            case 0x07: s = "Zenith"; break;
            case 0x08: s = "Tandon"; break;
            case 0x09: s = "AST Europe"; break;
            case 0x0a: s = "Asem"; break;
            case 0x0b: s = "Hantarex"; break;
            case 0x0c: s = "SystemLine"; break;
            case 0x0d: s = "Packard-Bell"; break;
            case 0x0e: s = "Intercomp"; break;
            case 0x0f: s = "Unibit"; break;
            case 0x10: s = "Unidata"; break;
            case 0x16: s = "DEC"; break;
            case 0x17:
            case 0x23:
                s = "Olivetti"; break;
            case 0x28: s = "TI"; break;
            case 0x29: s = "Toshiba"; break;
            case 0x5e: s = "RxDOS"; break;
            case 0x66: s = "PTS-DOS"; break;
            case 0x99: s = "General Software's Embedded DOS"; break;
            case 0xcd: s = "Paragon S/DOS"; break;
            case 0xed:
            case 0xee:
            case 0xef:
                s = "DR-DOS"; break;
            case 0xfd: s = "FreeDOS"; break;
            case 0xff: s = "Microsoft"; break;

            default:
                s = "unknown";
        }
        printf(" (%s)\n", s);
        printf("Serial No.       %02X-%04X\n", serial_h, serial_l);
        if (oem_id == 0xfd && s_fdkernel) {
            int i;
            char c;
            printf("FDKernel rev.    \"");
            for (i=0; (c = s_fdkernel[i]) != '\0' && c != '\n' && i<256; ++i)
                printf("%c", c);
            printf("\"\n");
        }
    }
}



int optHelp;
int optV;
int optQ;
int optR;
int opt_maj, opt_min;

int mygetopt(int argc, char **argv)
{
    int rc = 0;

    while(argc > 0) {
        char *s = *argv;
        if (*s == '/' || *s == '-') {
            char c = *++s;
            switch(toupper(c)) {
                case '?': case 'H': optHelp = 1; break;
                case 'V': ++optV; break;
                case 'Q': optQ = 1; break;
                case 'R': optR = 1; break;
            }
        }
        else {
            if (opt_maj == 0 && isdigit(*s)) {
                unsigned long n;
                char *s2 = NULL;
                n = strtoul(s, &s2, 10);
                if (n < 2 || n > 99 || (s2 && *s2 != '.')) {
                    rc = -1;
                    break;
                }
                opt_maj = (int)n;
                s = s2 + 1;
                s2 = NULL;
                n = strtoul(s, &s2, 10);
                if (n > 99 || (s2 && *s2 != '\0')) {
                    rc = -1;
                    break;
                }
                opt_min = (int)n;
                if (opt_min <= 9 && *s != '0') opt_min *= 10;
            }
            else {
                rc = -1;
            }
        }

        --argc;
        ++argv;
    }
    return rc;
}

static void banner(void)
{
    const char msg_banner[] = 
        "SETFDVER version 0.00 (built at "__DATE__ " " __TIME__ ")"
        ;
    printf("%s\n", msg_banner);
}

static void usage(void)
{
    const char msg_usage[] = 
        "usage: setfdver [-q]\n"
        "       setfdver fake_version\n"
        "       setfdver -r\n"
        "\n"
        " -q            quiet mode\n"
        " fake_version  reported FreeDOS version (from 2.00 to 99.99)\n"
        " -r            revert fake version\n"
        "\n"
        "examples:\n"
        "  setfdver             display DOS version info.\n"
        "  setfdver 5.0         assume DOS 5.0\n"
        "  setfdver 6.2         assume DOS 6.2 (6.20)\n"
        "  setfdver 6.22        assume DOS 6.22\n"
        "  setfdver -r          set DOS version to the defalt\n"
        ;
        banner();
        printf("%s", msg_usage);
}

int main(int argc, char *argv[])
{
    int rc;

    rc = mygetopt(argc - 1, argv + 1);
    if (rc < 0) {
        fprintf(stderr, "SETFDVER: parameter error.\n(Type setfdver -? to help)");
        return 1;
    }
    if (optHelp) {
        usage();
        return 0;
    }

    query_and_disp_dosver(!optQ && !optR && opt_maj == 0);

    if (optR || opt_maj > 0) {
        if (oem_id != 0xfd) {
            fprintf(stderr, "SETFDVER: not FreeDOS kernel\n");
            return 1;
        }
        if (optR) {
            opt_maj = true_maj;
            opt_min = true_min;
        }
        r.x.ax = 0x33fc;
        r.h.bl = opt_maj;
        r.h.bh = opt_min;
        intdos(&r, &r);
        if (!optQ) {
            printf("set DOS version to %d.%02d\n", opt_maj, opt_min);
        }
    }

    return 0;
}
