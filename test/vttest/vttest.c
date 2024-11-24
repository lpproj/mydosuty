/*
    vttest.c
    ANSI的なエスケープシーケンスをサポートした端末（Windows/UNIX/Linux/DOS）の
    ①画面サイズ
    ②行末のカーソル処理
    ③画面末（右下）のカーソル処理
    を調べる

    ・WindowsはデフォルトコンソールかWindowsTerminalのみサポート
    ・DOSはCONデバイスのみサポート（CTTY AUXした場合などは未対応）

    usage:
    vttest [-w]
      -w    行末を超えた範囲への表示で次の行に移行しない（Windows, PC-98 MS-DOS）

    行末／画面末カーソルの状態の説明

    move next
      表示した文字のひとつ右（が画面外の場合は次の行の左端。最下行だった場合はスクロールアップ）
    don't move
      表示した文字と同位置
    move next but don't wrap into new line
      表示した文字のひとつ右（が画面外の場合も画面サイズの範囲を超えて移動し、次の行に行くことはない）
    out of screen columns
      表示した文字のひとつ右であり、画面の桁数を超えた位置（PC-98シリーズのMS-DOS標準コンソールドライバでこのような挙動になる）

    OS/2も1.x時代のコンソールはエスケープシーケンスサポートしてるらしいんですけどね…対応が面倒なので…（32ビットコンソールだと未対応だったりする）
*/

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 _WIN32
#endif
#ifdef _MSC_VER
/* VC++: use POSIX functions without warings */
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#endif

#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__unix__)
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#else
#include <io.h>
#endif
#if defined(MSDOS) || defined(__DOS__) || defined(__TURBOC__) || defined(LSI_C)
#include <dos.h>
#endif

#if (defined(_MSC_VER) || (defined(__TURBOC__) && __TURBOC__ >= 0x0520)) && !defined(snprintf)
#define snprintf _snprintf
#endif

static int fd_conin = 0;
static int fd_conout = 1;
static long conin_timeout_msec = 5000;

#if defined(__unix__)
typedef struct termios  con_attr;
int con_setup(con_attr *prev_attr, int opt_w, int opt_n)
{
    int rc;
    (void)opt_w;
    (void)opt_n;

    memset(prev_attr, 0, sizeof(con_attr));
    if (!isatty(fd_conin) || !isatty(fd_conout)) return -1;
    rc = tcgetattr(fd_conin, prev_attr);
    if (rc >= 0) {
        con_attr new_attr = *prev_attr;
        new_attr.c_lflag &= ~(ECHO | ICANON);
        rc = tcsetattr(fd_conin, TCSANOW, &new_attr);
    }
    return rc;
}
int con_restore(const con_attr *prev_attr)
{
    return tcsetattr(fd_conin, TCSANOW, prev_attr);
}

static
int con_kbhit_with_timeout(long msec)
{
    struct timeval tv;
    fd_set fds;

    tv.tv_sec = msec / 1000;
    tv.tv_usec = (msec % 1000) * 1000;
    FD_ZERO(&fds);
    FD_SET(fd_conin, &fds);
    return (select(fd_conin + 1, &fds, NULL, NULL, (msec >= 0) ? &tv : NULL) > 0);
}

int con_getc_with_timeout(long msec)
{
    unsigned char ch;

    if (con_kbhit_with_timeout(msec) && read(fd_conin, &ch, 1) == 1) return (int)ch;
    return -1;
}
void con_flush(void)
{
    tcflush(fd_conout, TCIFLUSH);
}

#elif defined(WIN32) || defined(WIN64)
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif
#ifndef DISABLE_NEWLINE_AUTO_RETURN
#define DISABLE_NEWLINE_AUTO_RETURN 0x8
#endif
#define hConin  ((HANDLE)_get_osfhandle(fd_conin))
#define hConout ((HANDLE)_get_osfhandle(fd_conout))
typedef struct { DWORD attr_conin; DWORD attr_conout; }  con_attr;
int con_setup(con_attr *prev_attr, int opt_w, int opt_n)
{
    DWORD dwi, dwo;

    if (!GetConsoleMode(hConin, &dwi) || !GetConsoleMode(hConout, &dwo)) return -1;
    prev_attr->attr_conin = dwi;
    prev_attr->attr_conout = dwo;
    dwo &= ~(8UL | 2UL);
    dwo |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (!opt_w) dwo |= ENABLE_WRAP_AT_EOL_OUTPUT;
    if (opt_n) dwo |= DISABLE_NEWLINE_AUTO_RETURN;
    if (!SetConsoleMode(hConout, dwo)) return -1;
    dwi &= ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (!SetConsoleMode(hConin, dwi)) {
        SetConsoleMode(hConout, prev_attr->attr_conout);
        return -1;
    }
    return 0;
}
int con_restore(const con_attr *prev_attr)
{
    BOOL bi, bo;
    bi = SetConsoleMode(hConin, prev_attr->attr_conin);
    bo = SetConsoleMode(hConout, prev_attr->attr_conout);
    return bi && bo;
}
int con_kbhit(void)
{
    int is_hit = 0;
    DWORD dwCnt, i;
    INPUT_RECORD ir_int[64];
    PINPUT_RECORD pir_ext = NULL;
    PINPUT_RECORD pir;

    if (!GetNumberOfConsoleInputEvents(hConin, &dwCnt)) return 0;
    if (dwCnt > sizeof(ir_int)/sizeof(ir_int[0])) {
        pir_ext = malloc(dwCnt * sizeof(ir_int[0]));
        if (!pir_ext) return 0;
        pir = pir_ext;
    }
    else {
        pir = ir_int;
    }
    if (PeekConsoleInputA(hConin, pir, dwCnt, &dwCnt)) {
        for (i=0; i<dwCnt; ++i) {
            CONST PKEY_EVENT_RECORD pk = &(pir[i].Event.KeyEvent);
            if (pir[i].EventType == KEY_EVENT && pk->bKeyDown && pk->uChar.AsciiChar != '\0') {
                is_hit = 1;
                break;
            }
        }
    }
    if (pir_ext) free(pir_ext);
    return is_hit;
}
int con_getc_with_timeout(long msec)
{
    int c;
    DWORD t0;

    t0 = GetTickCount();
    while((c = con_kbhit()) == 0) {
        if (msec >= 0) {
            DWORD t1 = GetTickCount();
            long msec_lapse = (t0 <= t1) ? (t1 - t0) : (t1 + (DWORD)~t0 + 1);
            if (msec_lapse >= msec) break;
            else Sleep(1);
        }
    }
    if (c > 0) {
        unsigned char ch;
        DWORD dw = sizeof(ch);
        if (ReadConsoleA(hConin, &ch, dw, &dw, NULL) && dw > 0) {
            c = (int)ch;
        }
    }
    else {
        c = -1;
    }
    return c;
}

void con_flush(void)
{
    FlushConsoleInputBuffer(hConin);
}

#elif defined(MSDOS) || defined(__DOS__) || defined(__TURBOC__) || defined(LSI_C)
/* MS-DOS: handles stdin/stdout only (todo: support redirection) */
typedef struct { unsigned short con_attr; int mtype; int wstate; } con_attr;
enum { C_M_UNSPECIFIED = 0, C_M_IBMPC, C_M_ANSISYS, C_M_NEC98 };
int con_setup(con_attr *prev_attr, int opt_w, int opt_n)
{
    union REGS r;
    (void)opt_w;
    (void)opt_n;

    memset(prev_attr, 0, sizeof(con_attr));
    if (*(unsigned short far *)MK_FP(0xffffU, 3) == 0xfd80) {
        unsigned char far *FP_WRAP = MK_FP(0x60, 0x117);
        prev_attr->mtype = C_M_NEC98;
        prev_attr->wstate = *FP_WRAP;
        *FP_WRAP = opt_w ? 1 : 0;
    }
    r.x.ax = 0x4400;
    r.x.bx = fd_conout;
    intdos(&r, &r);
    if (r.x.cflag || !(r.h.dl & 0x80)) return -1;
    r.x.ax = 0x4400;
    r.x.bx = fd_conin;
    intdos(&r, &r);
    if (r.x.cflag) return -1;
    prev_attr->con_attr = r.x.dx;
    if ((r.h.dl & 0x81) != 0x81) return -1; /* check stdin is default console */
    r.h.dh = 0;
    r.h.dl |= 0x20;         /* RAW mode (may not be needed with DOS v1.x keyboard input function calls...) */
    r.x.ax = 0x4401;
    r.x.bx = fd_conin;
    intdos(&r, &r);
    return r.x.cflag ? -1 : 0;
}
int con_restore(const con_attr *prev_attr)
{
    union REGS r;
    if (prev_attr->mtype == C_M_NEC98) {
        *(unsigned char far *)MK_FP(0x60, 0x117) = prev_attr->wstate;
    }
    r.x.ax = 0x4401;
    r.x.bx = fd_conin;
    r.x.dx = prev_attr->con_attr & 0xff;
    intdos(&r, &r);
    return r.x.cflag ? -1 : 0;
}

static long con_get_msec(void)
{
    union REGS r;
    r.h.ah = 0x2c;
    intdos(&r, &r);
    return (long)(r.h.dh) * 1000 + (unsigned)(r.h.dl) * 10;
}
int con_getc_with_timeout(long msec)
{
    union REGS r;
    long t0;
    int c;

    if (msec >= 60000L) msec = 59999L;   /* workaround: limit to 60secs */
    t0 = con_get_msec();
    while (1) {
        r.x.ax = 0x0b00;
        intdos(&r, &r);
        c = (r.h.al != 0);
        if (c == 0) {
            long t1 = con_get_msec();
            long msec_lapse = (t0 <= t1) ? (t1 - t0) : (t0 + t1 - 60000L + 1);
            if (msec <= msec_lapse) {
                break;
            }
        }
        else {
            break;
        }
    }
    if (c) {
        r.x.ax = 0x0600;
        r.h.dl = 0xff;
        intdos(&r, &r);
        c = r.h.al;
    }
    else {
        c = -1;
    }
    return c;
}

void con_flush(void)
{
    union REGS r;
    r.x.ax = 0x0c0b;
    intdos(&r, &r);
}

#else
#error need con_setup, con_restore, con_flush and con_getc_with_timeout for your environment.
#endif


#define BS "\x8"
#define ESC "\x1b"
#define CSI ESC "["

#ifdef DONT_HAVE_SNPRINTF
#define mySPRINTF1(s,t,v)       sprintf(s,t,v)
#define mySPRINTF2(s,t,v1,v2)   sprintf(s,t,v1,v2)
#else
#define mySPRINTF1(s,t,v)       snprintf(s,sizeof(s)-1,t,v)
#define mySPRINTF2(s,t,v1,v2)   snprintf(s,sizeof(s)-1,t,v1,v2)
#endif

static
int con_put(const char *s)
{
    int n = strlen(s);
    if (n > 0) n = write(fd_conout, s, n);
    return n;
}

static
void con_gotox1y1(int x1, int y1)
{
    char s[128];

    s[sizeof(s)-1] = '\0';
    mySPRINTF2(s, CSI "%d;%dH", y1, x1);
    con_put(s);
}

/* get ESC[yy;xxR from stdin and parse it  */
static char buf[256];
static
int con_getx1y1(int *x1, int *y1)
{
    enum { S_esc, S_lbrace, S_digitY, S_digitX, S_end };
    int bpos, dpos, todo;
    /* char buf[256]; */
    int c;
    long vl;

    con_put(ESC "[6n");

    for (bpos = 0, todo = S_esc; todo != S_end && bpos < sizeof(buf) - 1;) {
        if ((c = con_getc_with_timeout(conin_timeout_msec)) < 0) return -1;
        buf[bpos++] = c;
        buf[bpos] = '\0';
        switch (todo) {
            case S_esc:
                if (c != 0x1b) return -1;
                todo = S_lbrace;
                break;
            case S_lbrace:
                if (c != '[') return -1;
                todo = S_digitY;
                dpos = bpos;
                break;
            case S_digitY:
                if (c >= '0' && c <= '9') break;
                if (c != ';') return -1;
                vl = strtol(&buf[dpos], (char **)NULL, 10); /* must be 10-based */
                if (vl < 0 || vl >= INT_MAX) return -1;
                if (y1) *y1 = (int)vl;
                dpos = bpos;
                todo = S_digitX;
                break;
            case S_digitX:
                if (c >= '0' && c <= '9') break;
                if (c != 'R') return -1;
                vl = strtol(&buf[dpos], (char **)NULL, 10); /* must be 10-based */
                if (vl < 0 || vl >= INT_MAX) return -1;
                if (x1) *x1 = (int)vl;
                todo = S_end;
                break;
            default:
                return -1;
        }
    }

    return bpos;
}

static int con_get_cols_rows(int get_rows)
{
    int x1 = -1, y1 = -1;
    if (con_getx1y1(&x1, &y1) >= 0) {
        return get_rows ? y1 : x1;
    }
    return -1;
}

/*
    間接的にだが、エスケープシーケンスに食わせる数字の上限をいちおう確認しておく
    （アセンブラとかで組んでた8～16ビット時代の端末ドライバは、そんなに本気で数字の解析とかしてなかったり、内部値をバイト単位で保持していて容易にオーバーフローしたりする）
    可能性としてありえる制限：
    ・数字2桁
    ・INT8_MAX
    ・UINT8_MAX
    ・数字3桁
    ・数字4桁
    ・INT16_MAX
    ・UINT16_MAX
    ・（以下略）

    （ESC[6n が端末が返してくるカーソル位置と同じ範囲の数字なら、同じように端末に食わせることができるだろう、という「類推」的な判定法）
*/
static
int con_check_num_range(int check_rows)
{
#if (INT_MAX) <= 0x7fff
    const int range[] = { 99, 127, 255, 999, 9999, (INT_MAX) - 1, 0 };
#else
    const int range[] = { 99, 127, 255, 999, 9999, 0x7fff, 99999, (INT_MAX) - 1, 0 };
#endif
    const char *s_posxy_template = check_rows ? (CSI "%u;1H") : (CSI "1;%uH");
    const char *s_move_template = check_rows ? (CSI "B") : (CSI "C");
    char s[256];
    unsigned n;
    int max_value = -1;

    s[sizeof(s)-1] = '\0';
    mySPRINTF1(s, s_posxy_template, 1);
    if (con_get_cols_rows(check_rows) != 1) return -1;

    for(n = 0; range[n] > 0; ++n) {
        int pos;
        max_value = range[n];
        mySPRINTF1(s, s_posxy_template, max_value);
        con_put(s);
        con_put(s_move_template);
        pos = con_get_cols_rows(check_rows);
        if (pos != max_value + 1) break;
    }

    return max_value;
}


int optN;
int optW;
int optHelp;

enum { CSR_NEXT = 1, CSR_CURRENT, CSR_NEXT_NONL, CSR_OVER_COLUMNS, CSR_NEXT_AFTER_ROLLUP, CSR_CURRENT_AFTER_ROLLUP };

static
const char *csr_tomsg(int n)
{
    const char *s;

    switch (n) {
        case CSR_NEXT:
            s = "move next";
            break;
        case CSR_CURRENT:
            s = "don't move";
            break;
        case CSR_NEXT_NONL:
            s = "move next but don't wrap into new line";
            break;
        case CSR_OVER_COLUMNS:
            s = "out of screen columns";
            break;
        case CSR_NEXT_AFTER_ROLLUP:
            s = "rollup, then move next";
            break;
        case CSR_CURRENT_AFTER_ROLLUP:
            s = "rollup and stay";
            break;
        case -1:
            s = "---";
            break;
        default:
            s = "(unspecified result)";
            break;
    }

    return s;
}

static
void usage(void)
{
    const char msg[] = \
        "vttest "
        "(built at " __TIME__ ", " __DATE__ ")\n"
        "\n"
        "usage: vttest [-w]\n"
        ;

    printf("%s", msg);
}

int main(int argc, char **argv)
{
    const char s_onechar[] = " "; /* ">" */
    int rc = 1;
    int x, y, columns, rows;
    con_attr prev_attr;
    char *err_str = "";
    char s[1024];
    int csr_eol = -1;
    int csr_over_eol = -1;
    int csr_eos = -1;
    int csr_over_eos = -1;


    s[sizeof(s)-1] = '\0';

    while(--argc > 0) {
        const char *str = *++argv;
        if (*str == '/' || *str == '-') {
            char c = *++str;
            if (c == '?' || c == 'H' || c == 'h') optHelp = 1;
            else if (c == 'n') optN = 1;
            else if (c == 'w') optW = 1;
        }
    }
    if (optHelp) {
        usage();
        return 0;
    }

    if (con_setup(&prev_attr, optW, optN) < 0) {
        fprintf(stderr, "ERROR: terminal/console initiialization failed.\n");
        return 1;
    }
    con_flush();
    con_put(CSI "s");       /* reserve cursor status */
    con_put(CSI "2J");      /* cls */
    con_put(CSI "1;1H");    /* home */

    con_flush();
    x = y = columns = rows = -1;
    con_getx1y1(&x, &y);
    if (x == 1 && y == 1) {
        x = con_check_num_range(0);
        y = con_check_num_range(1);
        con_gotox1y1(1, 1);
        mySPRINTF1(s, CSI "%uC", x);
        con_put(s);
        mySPRINTF1(s, CSI "%uB", y);
        con_put(s);
        con_getx1y1(&columns, &rows);
    }
    if (columns > 0 && rows > 0 && !(columns == 1 && rows == 1)) {
        if (rows > 1 && columns > 1) {
            int nx = -1, ny = -1;
            int nx2 = -1, ny2 = -1;
            con_gotox1y1(columns, 1);
            con_put(s_onechar);
            con_getx1y1(&nx, &ny);
            if (ny == 1) {
                if (nx == columns) csr_eol = CSR_CURRENT;
                else if (nx > columns) csr_eol = CSR_OVER_COLUMNS;
                con_put(s_onechar);
                con_getx1y1(&nx2, &ny2);
                if (ny2 == 1) {
                    if (nx2 == nx) {
                        csr_over_eol = csr_eol;
                    }
                    else if (nx2 > nx) {
                        csr_eol = csr_over_eol = CSR_NEXT_NONL;
                    }
                }
                else if (ny2 > ny) {
                    if (nx2 == 1) csr_over_eol = CSR_CURRENT;
                    else if (nx2 == 2)  csr_over_eol = CSR_NEXT;
                }
            }
            else if (ny == 2) {
                if (nx == 1) csr_eol = CSR_NEXT;
            }
#if 1
            nx = ny = nx2 = ny2 = -1;
            con_gotox1y1(columns, rows);
            con_put(s_onechar);
            con_getx1y1(&nx, &ny);
            if (ny == rows) {
                if (nx == 1) csr_eos = CSR_NEXT;
                else if (nx >= columns) {
                    if (nx == columns) csr_eos = CSR_CURRENT;
                    else if (nx > columns) csr_eos = CSR_OVER_COLUMNS;
                    con_put(s_onechar);
                    con_getx1y1(&nx2, &ny2);
                    if (ny2 == rows) {
                        if (nx2 == 1) csr_over_eos = CSR_CURRENT;
                        else if (nx2 == 2) csr_over_eos = CSR_NEXT;
                        else if (nx2 == columns) csr_over_eos = CSR_CURRENT;
                        else if (nx2 > columns) {
                            if (nx > columns && nx2 == nx) csr_over_eos = CSR_OVER_COLUMNS;
                            else if (nx2 > nx) csr_over_eos = CSR_NEXT_NONL;
                        }
                    }
                }
            }
            if (ny == rows - 1) {
                if (nx == 1) csr_eos = CSR_NEXT_AFTER_ROLLUP;
                else if (nx == columns) csr_eos = CSR_CURRENT_AFTER_ROLLUP;
            }
#endif
        }
        rc = 0;
    }
    else {
        err_str = "can't get screen size of the terminal.";
    }

    con_put(ESC "[u");
    con_restore(&prev_attr);
    if (rc == 0) {
        printf("screen width : (%d,%d)\n", columns, rows);
        printf("csr_eol      : %d (%s)\n", csr_eol, csr_tomsg(csr_eol));
        printf("csr_over_eol : %d (%s)\n", csr_over_eol, csr_tomsg(csr_over_eol));
        printf("csr_eos      : %d (%s)\n", csr_eos, csr_tomsg(csr_eos));
        printf("csr_over_eos : %d (%s)\n", csr_over_eos, csr_tomsg(csr_over_eos));
    }
    else {
        if (err_str && *err_str) fprintf(stderr, "ERROR: %s\n", err_str);
    }

    return rc;
}

