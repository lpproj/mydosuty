/*
teecon - run a program, with copying stdout/stderr to a file.

platform:
    DOS 3.1 or later

license:
    UNLICENSE

how to build:
    wcl -s -os -bt=dos teecon.c -ldos   (Watcom C/C++)


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
#include <fcntl.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

#define SWITCH_PSP 1

extern void my_enable(void);
#pragma aux my_enable = "sti";
extern void my_disable(void);
#pragma aux my_disable = "cli";

extern unsigned short my_getpsp(void);
#pragma aux my_getpsp = "mov ah,51h" "int 21h" value [bx] modify [ah];
extern void my_setpsp(unsigned);
#pragma aux my_setpsp = "mov ah,50h" "int 21h" parm [bx] modify [ah];

extern long my_doslseek(int, long, char);
#pragma aux my_doslseek = "mov ah,42h" "int 21h" "sbb cx,cx" "or dx,cx" "or ax,cx" parm [bx] [cx dx] [al] value [dx ax] modify [cx];

extern long  my_doswrite_int21(int, const void far *, unsigned);
/* oops, parm [ds dx] will not work correctly... */
#pragma aux my_doswrite_int21 = "mov ah,40h" "push ds" "push es" "pop ds" "int 21h" "pop ds" "sbb dx,dx" parm [bx] [es dx] [cx] value [dx ax];

extern int  my_dosisatty(int);
#pragma aux my_dosisatty = "mov ax,4400h" "int 21h" "cmc" "sbb bx,bx" "and dx, 80h" "and dx,bx" parm [bx] value [dx] modify [ax bx] ;

extern void (__interrupt __far *my_dosgetvect(int))();
#pragma aux my_dosgetvect = "push ds" "xor ax,ax" "mov ds,ax" "add bx,bx" "add bx,bx" "mov ax, [bx]" "mov dx,[bx+2]" "pop ds" parm [bx] value [dx ax] modify [bx];
extern void  my_dossetvect(int, void (__interrupt __far *)());
#pragma aux my_dossetvect = "push ds" "push ax" "xor ax,ax" "mov ds,ax" "pop ax" "add bx,bx" "add bx,bx" "mov [bx],ax" "mov [bx+2],dx" "pop ds" parm [bx] [dx ax] modify [bx];


static void (__interrupt __far *org_int21)();
static void (__interrupt __far *org_int23)();
static void (__interrupt __far *org_int24)();
static unsigned org_psp;
static unsigned my_psp;
static unsigned err_on_write;

typedef int  (*tee_func_handler)(const char far *mem, unsigned length);

static int tee_handle = -1;
static char far *tee_func_buffer;
static unsigned tee_func_length;
static char tee_pchar;

static tee_func_handler  teefunc;
static int tee_check_handle;


static void __interrupt __far private_int23_handler()
{
    /* Ctrl-C: ignore */
}

static void __interrupt __far private_int24_handler(union INTPACK r)
{
    r.h.al = 3; /* Critical error: always "fail" (DOS 3+) */
}

unsigned my_doswrite(int handle, const void far *mem, unsigned length)
{
    long rc;

#if defined(SWITCH_PSP)
    org_psp = my_getpsp();
#endif

    org_int23 = my_dosgetvect(0x23);
    org_int24 = my_dosgetvect(0x24);

#if defined(SWITCH_PSP)
    my_setpsp(my_psp);
#endif

    my_disable();
    my_dossetvect(0x23, private_int23_handler);
    my_dossetvect(0x24, private_int24_handler);
    my_enable();

    rc = my_doswrite_int21(handle, mem, length);
    err_on_write = (rc < 0) ? (unsigned)rc : 0;

#if defined(SWITCH_PSP)
    my_setpsp(org_psp);
#endif

    my_disable();
    my_dossetvect(0x23, org_int23);
    my_dossetvect(0x24, org_int24);
    my_enable();

    return (unsigned)rc;
}


static int  tee_func_write(const char far *mem, unsigned length)
{
    return length ? my_doswrite(tee_handle, mem, length) : 0;
}


void __interrupt __far Int21Handler(union INTPACK r)
{
    teefunc = NULL;
    tee_check_handle = 1;

    if (tee_handle != -1 && !err_on_write) switch(r.h.ah) {
        case 0x06:
            if (r.h.dl == 0xff) break;
            /* fallthrough (to case 0x02) */
        case 0x02:
            tee_pchar = r.h.dl;
            tee_func_length = 1;
            tee_func_buffer = &tee_pchar;
            teefunc = tee_func_write;
            break;

        case 0x09:
            tee_func_length = 0;
            tee_func_buffer = MK_FP(r.w.ds, r.x.dx);
            while(tee_func_buffer[tee_func_length] != '$') {
                if (tee_func_length >= 0x7fffU) break; /* avoid overrun */
                ++tee_func_length;
            }
            teefunc = tee_func_write;
            break;

        case 0x40:
            if (r.x.bx != 1 && r.x.bx != 2) break;
            tee_func_length = r.x.cx;
            tee_func_buffer = MK_FP(r.w.ds, r.x.dx);
            tee_check_handle = r.x.bx;
            teefunc = tee_func_write;
            break;

        default:
            break;
    }

    if (teefunc && my_dosisatty(tee_check_handle)) {
        tee_func_write(tee_func_buffer, tee_func_length);
    }

    _chain_intr(org_int21);
}

int optH;
int optA;
int optO = 1;
int optT;
char *logname;

int mygetopt(int argc, char *argv[])
{
    int i = 0;
    
    while(argc > 0) {
        char *s;

        ++i;
        --argc;
        ++argv;
        s = *argv;
        if ((*s == '-' || *s == '/') && s[1] == '?') optH = 1;
        else if (*s == '-') {
            switch(s[1] | 0x20) { /* poorman's tolower in the ASCII world */
                case 'a': optA = 1; optO = 0; break;
                case 'o': optA = 0; optO = 1; break;
                case 'h': optH = 1; break;
                case 't': optT = 1; break;
            }
        }
        else {
            if (logname) break;
            logname = s;
        }
    }

    return i;
}


void usage(void)
{
    const char msg[] =
        "Usage: teecon [-o] [-a] LOGFILE PROGRAM [...]\n"
        "Execute the PROGRAM with copying stdout/stderr to the LOGFILE.\n"
        "(when stdout/stderr is not redirected to a file)\n"
        "\n"
        "  -o   overwrite the LOGFILE (default)\n"
        "  -a   append the LOGFILE\n"
        "\n"
        "ex) teecon dir_c.txt tree /f c:\\\n";
        ;

    printf("%s", msg);
}


int main(int argc, char *argv[])
{
    int rc = -1;
    int index_cmd;

    index_cmd = mygetopt(argc, argv);
    if (optH) {
        usage();
        return 0;
    }
    if (index_cmd >= argc || !logname) {
        fprintf(stderr, "Type 'TEECON -?' to help.\n");
        return 1;
    }

    if (optA) {
        rc = _dos_open(logname, O_RDWR, &tee_handle);
        if (rc == 0) my_doslseek(tee_handle, 0L, 2);
        else if (rc == 2) optO = 1; /* retry with creat when the file is not found */
        else optO = 0;
    }
    if (optO && tee_handle == -1) rc = _dos_creat(logname, _A_NORMAL, &tee_handle);

    if (rc != 0) {
        fprintf(stderr, "FATAL: can't open the log file '%s'\n", logname);
        return -1;
    }

    my_psp = my_getpsp();
    org_int21 = _dos_getvect(0x21);
    _dos_setvect(0x21, Int21Handler);
    if (argv[index_cmd]) {
        rc = spawnvp(P_WAIT, argv[index_cmd], (char const * const *)(argv + index_cmd));
    }
    _dos_setvect(0x21, org_int21);
    fprintf(stderr, "%s : result=%d\n", argv[index_cmd], rc);
    if (tee_handle != -1) _dos_close(tee_handle);

    return rc;
}

