/*
    chgdate.c

    to build with (Open)Watcom:
      wcl -zq -s -fr -ot -bt=dos chgdate.c
    
    with gcc-ia16:
      ia16-elf-gcc -Wall -Os -s -o chgdate.exe -mcmodel=small chgdate.c -li86
    
    (and you can build with most of other compilers for 16bit DOS...)

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

int safer_date = 0;

void dos_getdatetime(union REGS *rd, union REGS *rt)
{
    rd->h.ah = 0x2a;    /* get system date */
    intdos(rd, rd);
    rt->h.ah = 0x2c;    /* get system time */
    intdos(rt, rt);
    if (safer_date) { /* to be safe */
        /* get date again at 00:00:00.xx (guess just after date-changed) */
        if (rt->x.cx == 0 && rt->x.dx <= 0x100) {
            rd->h.ah = 0x2a;
            intdos(rd, rd);
        }
    }
}

void dos_setdatetime(union REGS *rd, union REGS *rt)
{
#if 0   /* good for paranoids... */
    if (safer_date) {
        /* to avoid date progress situation, set time to 00:00:00 at first */
        union REGS rt_zh;
        rt_zh.x.cx = rt_zh.x.dx = 0;
        rt_zh.h.ah = 0x2d;
        intdos(&rt_zh, &rt_zh);
        /* then, set date and time in order */
        rd->h.ah = 0x2b;    /* set system date */
        intdos(rd, rd);
        rt->h.ah = 0x2d;    /* set system time */
        intdos(rt, rt);
        return;
    }
#endif
    rt->h.ah = 0x2d;    /* set system time */
    intdos(rt, rt);
    rd->h.ah = 0x2b;    /* set system date */
    intdos(rd, rd);
}


int dos_peek_and_getch(void)
{
    union REGS r;

    r.x.ax = 0x0b00;
    intdos(&r, &r);
    if (r.h.al == 0) return -1;
    r.x.ax = 0x0600;
    r.h.dl = 0xff;
    intdos(&r, &r);
    return r.h.al;
}

void dos_flush_key(void)
{
    while(dos_peek_and_getch() != -1)
        ;
}

void print_dosdatetime(const char *pre_msg, const union REGS *pd, const union REGS *pt)
{
    if (pre_msg) printf("%s", pre_msg);
    printf("%04u-%02u-%02u", pd->x.cx, pd->h.dh, pd->h.dl);
    printf(" %02u:%02u:%02u.%02u", pt->h.ch, pt->h.cl, pt->h.dh, pt->h.dl);
    fflush(stdout);
}

unsigned long time2sec100s(const union REGS *pt)
{
    return (unsigned long)(((unsigned)(pt->h.ch) * 60U + pt->h.cl) * 60U + pt->h.dh) * 100U + pt->h.dl;
}


enum {
    ABORTED_BY_USER = -1,
    DATE_PROGRESS_OK = 0,
    DATE_PROGRESS_BAD = 1
};

int check_date_progress(const char *pre_msg)
{
    int rc = DATE_PROGRESS_OK;
    union REGS rd, rd_bak;
    union REGS rt, rt_bak;

    dos_getdatetime(&rd_bak, &rt_bak);
    print_dosdatetime(pre_msg, &rd_bak, &rt_bak);
    while(1) {
        unsigned long ts, ts_bak;
        dos_getdatetime(&rd, &rt);
        if (rt.x.dx == rt_bak.x.dx) {
            continue;
        }
        ts = time2sec100s(&rt);
        ts_bak = time2sec100s(&rt_bak);

        printf("\r");
        print_dosdatetime(pre_msg, &rd, &rt);
        if (ts < ts_bak) { /* does new day come? */
            if (rd.x.dx != rd_bak.x.dx) { /* date changed? */
                printf(" OK\n");
                break;
            }
            else {
                printf(" ** ERROR ** (bad progress)\n");
                rc = DATE_PROGRESS_BAD;
            }
            break;
        }
        rd_bak = rd;
        rt_bak = rt;
        if (dos_peek_and_getch() == 0x1b) {
            printf("\nABORTED BY USER\n");
            rc = ABORTED_BY_USER;
            break;
        }
    }

    return rc;
}


int test_date_progress(long count, long *tested, long *success, long *failed)
{
    int rc = -1;
    long t_tested, t_success, t_failed;
    union REGS rd_org, rt_org;
    union REGS rt_jb; /* 23:59:58.00 */

    rt_jb.h.ch = 23; rt_jb.h.cl = 59; rt_jb.h.dh = 58; rt_jb.h.dl = 0;
    dos_getdatetime(&rd_org, &rt_org);

    t_tested = 0;
    t_success = t_failed = 0;
    while(t_tested < count) {
        char s[64];
        sprintf(s, "TEST %ld ", t_tested + 1);
        dos_setdatetime(&rd_org, &rt_jb);
        rc = check_date_progress(s);
        if (rc < 0) break;
        if (rc == DATE_PROGRESS_OK) ++t_success;
        else ++t_failed;
        ++t_tested;
    }
    printf("test count = %ld\n", t_tested);
    printf("failed = %ld\n", t_failed);
    if (rc != ABORTED_BY_USER) {
        rc = t_failed ? DATE_PROGRESS_BAD : DATE_PROGRESS_OK;
    }
    if (tested) *tested = t_tested;
    if (success) *success = t_success;
    if (failed) *failed = t_failed;

    return rc;
}



int optHelp = 0;
int optS = 0;
long progTest = 0;

int myopt(int argc, char **argv)
{
    int rc = 0;
    while(argc-- > 1) {
        char *s = *++argv;
        char c = *s;
        if (c == '-' || c == '/') {
            ++s;
            c = toupper(*s);
            if (c == '?' || c == 'H') {
                optHelp = 1;
                break;
            }
            else if (c == 'S') {
                optS = 1;
            }
            else if (c == 'P') {
                c = *++s;
                if (c == ':' || c == '=') c = *++s;
                if (c == '\0') {
                    if (argc > 1) {
                        --argc;
                        s = *++argv;
                    }
                    else {
                        rc = -1;
                        break;
                    }
                }
                progTest = strtol(s, NULL, 0);
                if (progTest <= 0) rc = -1;
            }
        }
    }
#if DEBUG
printf("myopt: rc=%d optHelp=%d optS=%d progTest=%ld\n", rc, optHelp, optS, progTest);
#endif
    return rc;
}

void usage(void)
{
    const char msg[] =
        "date progress test for DOS\n"
        "usage:\n"
        "  chgdate [-S] [-P count_for_test]\n"
        "    -S     use safer method for getting system date\n"
        "    -P     test `progress date' mode\n"
        ;
    printf("%s", msg);
}

int main(int argc, char **argv)
{
    int rc;

    rc = myopt(argc, argv);
    if (rc < 0 || optHelp) {
        return rc;
    }
    dos_flush_key();

    safer_date = optS;
    printf("press ESC to exit.\n");
    if (progTest > 0) {
        rc = test_date_progress(progTest, NULL, NULL, NULL);
    }
    else {
        rc = check_date_progress(NULL);
    }

    return rc;
}
