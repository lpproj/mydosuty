/*
csr.c: set cursor size and visiblity
by sava (lpproj)

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

/*
desctiption (in Japanese):
Windowsコンソールのカーソルサイズ、表示／非表示の設定。
Win10(1809)のコマンドプロンプトでカーソルがちょくちょく勝手に
非表示になるため、応急処置として作成。
おまけでMS-DOS (IBMPC) 用の処理もつけてみた。

*/


#if defined(WIN32) || defined(_WIN32)
# include <windows.h>
# if !defined(_T)
#  if defined(UNICODE) || defined(_UNICODE)
#   define _T(x) L##x
#  else
#   define _T(x) x
#  endif
# endif
# define FOR_WIN
#else
# include <dos.h>
# define FOR_DOS
#endif
#include <limits.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CSR_DO_NOTHING = 0,
    CSR_OFF,
    CSR_ON,
    CSR_RESIZE,
};

int optH;
int cmd = CSR_DO_NOTHING;
int new_csr_size = -1;

typedef struct {
    int visible;
    int size100;
} CSR_STATE;


#if defined(FOR_WIN)
int get_csr_win32(CSR_STATE *csr)
{
    int rc = 0;
    HANDLE h = CreateFile(_T("CON"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cci;
        if (GetConsoleCursorInfo(h, &cci)) {
            csr->visible = (cci.bVisible > 0);
            csr->size100 = (int)(cci.dwSize);
            rc = 1;
        }
        CloseHandle(h);
    }
    return rc;
}
int set_csr_win32(const CSR_STATE *csr)
{
    int rc = 0;
    HANDLE h = CreateFile(_T("CON"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO cci;
        if (GetConsoleCursorInfo(h, &cci)) {
            cci.bVisible = csr->visible ? TRUE : FALSE;
            cci.dwSize = csr->size100;
            rc = !!SetConsoleCursorInfo(h, &cci);
        }
        CloseHandle(h);
    }
    return rc;
}

# define get_csr(c)  get_csr_win32(c)
# define set_csr(c)  set_csr_win32(c)

#elif defined(FOR_DOS)

int get_csr_dos_ibm(CSR_STATE *csr)
{
    unsigned short csh = *(unsigned short far *)MK_FP(0x40, 0x60);
    csr->visible = ((csh >> 8) <= (csh & 0xff)) && !(csh & 0x2000);
    csr->size100 = (7 - ((csh >> 8) & 7)) * 100 / 7;
    return 1;
}

int set_csr_dos_ibm(CSR_STATE *csr)
{
    int rc = 0;
    if (csr->size100 > 0 && csr->size100 <= 100) {
        union REGS r;
        r.h.cl = 7;
        r.h.ch = (700 - csr->size100 * 7) / 100;
        if (!csr->visible) r.h.ch |= 0x20;
        r.h.ah = 0x01;
        int86(0x10, &r, &r);
        rc = 1;
    }
    return rc;
}

# define get_csr(c)  get_csr_dos_ibm(c)
# define set_csr(c)  set_csr_dos_ibm(c)
#endif

int mygetopt(int argc, char *argv[])
{
    int rc = 0;
    while(argc > 0) {
        char *s = *argv;
        if (*s == '-' || *s == '/') switch(toupper(s[1])) {
            case '?': case 'H': optH = 1; break;
        }
        else {
            if (stricmp(s, "on")==0) cmd = CSR_ON;
            else if (stricmp(s, "off")==0) cmd = CSR_OFF;
            else if (isdigit(*s)) {
                char *sn = NULL;
                long l = strtol(s, &sn, 0);
                if (sn && *sn == '\0' && l >= 1 && l <= 100) {
                    if (!cmd) cmd = CSR_RESIZE;
                    new_csr_size = (int)l;
                }
                else {
                    rc = -1;
                    break;
                }
            }
            else {
                rc = -1;
                break;
            }
        }
        
        --argc;
        ++argv;
    }
    return rc;
}

void
usage(void)
{
    const char msg[] = 
        "Set size/visiblity of the cursor.\n"
        "usage: csr [on] [off] [size]\n"
        "  on    show the cursor\n"
        "  off   hide the cursor\n"
        " size   height of the cursor (decimal number of 1..100)\n"
        "exmaples:\n"
        "    csr\n"
        "    csr on 25\n"
        "    csr off\n"
        ;
    printf("%s", msg);
}

int main(int argc, char *argv[])
{
    CSR_STATE csr;
    int do_modify = 0;
    int rc;

    rc = mygetopt(argc - 1, argv + 1);
    if (rc < 0) {
        fprintf(stderr, "invalid parameter(s).\n");
        fprintf(stderr, "type 'csr -?' to help.\n");
        return 1;
    }
    if (optH) {
        usage();
        return 0;
    }

    if (!get_csr(&csr)) {
        fprintf(stderr, "error: can't get cursor information for the console.\n");
        return 1;
    }
    switch(cmd) {
        case CSR_DO_NOTHING:
            printf("Cursor Height:%d%%  Visiblity:%u\n", csr.size100, csr.visible);
            break;
        case CSR_OFF:
        case CSR_ON:
            csr.visible = (cmd == CSR_ON);
            /* fallthrough */
        case CSR_RESIZE:
            do_modify = 1;
            break;
    }
    if (do_modify) {
        if (new_csr_size >= 0) csr.size100 = new_csr_size;
        set_csr(&csr);
    }
    
    return 0;
}

