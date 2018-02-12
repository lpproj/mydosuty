/*
setdrive: set boot/current drive to parent's environment
target: 16bit DOS
to build:
    wcl -zq -s setdrive.c -l=dos    (OpenWatcom)
    lcc setdrive.c -lintlib         (LSI-C86 3.30c trial)
latest src at:
    https://github.com/lpproj/mydosuty/setdrive/


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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(CHAR_MAX) && defined(UCHAR_MAX) && (CHAR_MAX == UCHAR_MAX)
# define my_upper(c) toupper(c)
#else
# define my_upper(c) ((c)>=0 ? toupper(c) : (c))
#endif

char far *
get_parent_environ(void)
{
    union REGS r;
    char far *p = 0L;
    unsigned short parent_seg;
    
    r.h.ah = 0x51;  /* or 0x62 */
    intdos(&r, &r);
    parent_seg = *(unsigned short far *)MK_FP(r.x.bx, 0x16);
    if (parent_seg > 8 && parent_seg != 0xffffU) {
        unsigned short env_seg = *(unsigned short far *)MK_FP(parent_seg, 0x2c);
        if (env_seg > 8 && env_seg != 0xffffU) {
            p = MK_FP(env_seg, 0);
        }
    }
    return p;
}


static char far *my_fstrchr(const void far *s, int c)
{
    char far *p = (char far *)s;
    char far *rp = 0L;
    
    while(1) {
        if (*p == c) {
            rp = p;
            break;
        }
        if (*p == '\0')
            break;
        ++p;
    }
    return rp;
}

static int my_fmemicmp(const void far *m0, const void far *m1, size_t n)
{
    int rc = 0;
    
    if (n > 0) {
        const char far *s0 = m0;
        const char far *s1 = m1;
        while(n--) {
            char c0 = my_upper(*s0);
            char c1 = my_upper(*s1);
            if (c0 != c1) {
                
                rc = (int)(unsigned)(unsigned char)c0 - (int)(unsigned)(unsigned char)c1;
                break;
            }
            ++s0;
            ++s1;
        }
    }
    
    return rc;
}


char far *
find_envvar(void far *envblk, const char *name)
{
    char far *pvar = 0L;
    char far *p = envblk;
    size_t nn;

    if (!envblk || !name) return 0L;
    nn = strlen(name);
    if (nn == 0) return 0L;
    
    while(*p) {
        char far *p2 = my_fstrchr(p, '=');
        if (p2) {
            size_t n2 = (p2 - p);
            if (n2 == nn && my_fmemicmp(p, name, nn) == 0) {
                pvar = p2 + 1;
                break;
            }
        }
        p2 = my_fstrchr(p, 0);
        p = p2 + 1;
    }
    return pvar;
}


int get_bootdrive(void)
{
    union REGS r;
    int rc = -1;
    
    /* LoL +43h ... not supported on NTVDM */
    r.x.ax = 0x3305;    /* get boot drive (DOS 4.0+) */
    r.x.dx = 0;
    intdos(&r, &r);
    if (r.h.dl >= 1 || r.h.dl <= 26) {
        rc = r.h.dl - 1;
    }
    
    return rc;
}

int get_curdrive(void)
{
    union REGS r;
    /* SDA + 16h ... do not expect to support DOS-emulators */
    
    r.x.ax = 0x1900;    /* get current drive */
    intdos(&r, &r);
    
    return r.h.al;
}


int optB;
int optC;
int optV;
int optHelp;
char *setenvname = NULL;

int
my_getopt(int argc, char *argv[])
{
    int iserr = 0;
    
    while(argc > 0) {
        char *s = *argv;
        if (*s == '/' || *s == '-') {
            switch(toupper(s[1])) {
                case 'B': optB = 1; break;
                case 'C': optC = 1; break;
                case 'V': optV = 1; break;
                case 'H': case '?':
                    optHelp = 1;
                    break;
                default:
                    iserr = 1;
                    break;
            }
        }
        else {
            if (!setenvname) {
                setenvname = s;
            }
        }
        
        --argc;
        ++argv;
    }
    
    return iserr;
}


void usage(int help)
{
    const char logo[] = 
        "setdrive"
        " (built at " __DATE__ " " __TIME__ ")"
        ;
    
    const char usage[] =
        "set boot drive to specified environment variable.\n"
        "usage: setdrive [-b] [-c] [-v] envname\n"
        "        -b         set boot drive (DOS 4.0+)\n"
        "        -c         set current drive\n"
        "        -v         show boot drive and current drive\n"
        "        envname    name of environment variable.\n"
        "example:\n"
        "    set SYSTEMDRIVE=?:\n"
        "    setdrive -b SYSTEMDRIVE\n"
        ;
    
    printf("%s\n", logo);
    if (help) {
        printf("%s", usage);
    } else {
        printf("type \"setdrive -?\" to help.\n");
    }
}


int main(int argc, char *argv[])
{
    int rc;
    
    rc = my_getopt(argc - 1, argv + 1);
    
    if (rc == 0 && (optB || optC) && setenvname) {
        int d = -1;
        if (optB) d = get_bootdrive();
        if (d == -1 && optC) d = get_curdrive();
        
        if (d == -1) {
            fprintf(stderr, "failure: can't get boot drive.\n");
            rc = 1;
        }
        else {
            char far *p = find_envvar(get_parent_environ(), setenvname);
            if (p && *p != '\0') {
                *p = 'A' + d;
            }
            else {
                fprintf(stderr, "failure: environment variable '%s' not found.\n", setenvname);
                rc = 1;
            }
        }
    
    }
    else {
        usage(optHelp);
        if (rc == 0 && !optHelp && optV) {
            int d;
            d = get_bootdrive();
            printf("Boot drive    = ");
            if (d >= 0) printf("%c:\n", 'A' + d);
            else printf("(unknown)\n");
            d = get_curdrive();
            printf("Current drive = %c:\n", 'A' + d);
        }
    }
    
    return rc;
}

