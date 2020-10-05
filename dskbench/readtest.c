/*
    readtest: a simple disk test/benchmark tool for DOS

    to build with OpenWatcom (1.9 or 2.0beta):
    wcl -zq -s -za99 -zp1 -Fr -ot -ml readtest.c -ldos

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
#include <time.h>
#if defined __STDC_VERSION__ && (__STDC_VERSION__) >= 199901L
#include <stdint.h>
typedef uint_least64_t  U64;
#else
typedef unsigned __int64  U64;
#endif


#if !(defined __TURBOC__ || defined LSI_C)
#pragma pack(1)
#endif


static char dskr_do_write;
static unsigned dskr_ax;
static unsigned dskr_bx;
static unsigned dskr_cx;
static unsigned dskr_dx;
static unsigned dskr_ds;

int my_absrw_body()
{
    int rc = -1;
    _asm {
        pushf
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        
        push bp
        push ds
        push es
        mov ax, [dskr_ax]
        mov bx, [dskr_bx]
        mov cx, [dskr_cx]
        mov dx, [dskr_dx]
        cmp [dskr_do_write], 0
        mov ds, [dskr_ds]
        je absrw_read
        int 26h
        jmp short absrw_after_rw
    absrw_read:
        int 25h
    absrw_after_rw:
        pop dx      ; drop resident flags
        pop es
        pop ds
        pop bp
        sbb bx, bx
        and ax, bx
        mov [rc], ax
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        popf
    }
    return rc;
}


union REGS r;
struct SREGS sr;
unsigned my_doserrno;

int my_dos_getver2(void)
{
    unsigned tv;
    r.x.ax = 0x3306;
    r.x.bx = 0;
    intdos(&r, &r);
    tv = r.x.bx;
    r.x.ax = 0x3000;
    intdos(&r, &r);
    if (tv) r.x.ax = tv;
    if (r.h.al == 10) return 0x031e;  /* OS/2 1.x as DOS 3.3 */
    if (r.h.al == 20) return 0x0500;  /* OS/2 2.x or Warp as DOS 5.0 */
    if (r.x.ax == 7 && r.h.bh == 0) return 0x061e;  /* PC DOS 7/2000 as DOS 6.3 */
    return ((unsigned)(r.h.al) << 8) | r.h.ah;
}

int my_dos_check_key(const char *keytbl, size_t tbl_len)
{
    while(1) {
        int i;
        r.x.ax = 0x0b00;
        intdos(&r, &r);
        if (!r.h.al) break;
        r.x.ax = 0x0700;
        intdos(&r, &r);
        for(i=0; i<tbl_len; ++i) {
            if ((unsigned char)(keytbl[i]) == r.h.al) {
                return i;
            }
        }
    }
    return -1;
}

int my_dos_ext_open(const char *fn, int action, int mode, int attr)
{
    r.x.ax = 0x6c00;
    r.x.si = FP_OFF(fn);
    sr.ds = FP_SEG(fn);
    r.x.bx = mode;
    r.x.cx = attr;
    r.x.dx = action;
    intdosx(&r, &r, &sr);
    if (r.x.cflag) {
        my_doserrno = r.x.ax;
        return -1;
    }
    return r.x.ax;
}

int my_dos_open2(const char *fn, int mode)
{
    int h;
    if (my_dos_getver2() >= 0x070a) {
        h = my_dos_ext_open(fn, mode | 0x1000, 0x01, 0); /* ~4G file in FAT32 */
        if (h != -1) return h;
    }
    r.h.ah = 0x3d;
    r.h.al = (unsigned char)mode;
    r.x.dx = FP_OFF(fn);
    sr.ds = FP_SEG(fn);
    intdosx(&r, &r, &sr);
    if (r.x.cflag) {
        my_doserrno = r.x.ax;
        return -1;
    }
    return r.x.ax;
}

void far *my_dos_get_dta(void)
{
    r.h.ah = 0x2f;
    intdosx(&r, &r, &sr);
    return MK_FP(sr.es, r.x.bx);
}

void far *my_dos_set_dta(void far *dta)
{
    union REGS r0;
    struct SREGS sr0;
    void far *prev_dta;
    r0.h.ah = 0x2f;
    intdosx(&r0, &r0, &sr0);
    prev_dta = MK_FP(sr0.es, r0.x.bx);
    r0.h.ah = 0x1a;
    r0.x.dx = FP_OFF(dta);
    sr0.ds = FP_SEG(dta);
    intdosx(&r0, &r0, &sr0);
    return prev_dta;
}


struct my_dos_find_t {
    unsigned char reserved[21];
    unsigned char attrib;
    unsigned short wr_time;
    unsigned short wr_date;
    unsigned long size;
    char name[13];
};
typedef struct my_dos_find_t  my_dos_find_t;

int my_dos_findfirst(const char *fn, unsigned attr, my_dos_find_t far *ff)
{
    void far *prev_dta = my_dos_set_dta(ff);
    r.x.ax = 0x4e00;
    r.x.cx = attr;
    r.x.dx = FP_OFF(fn);
    sr.ds = FP_SEG(fn);
    intdosx(&r, &r, &sr);
    my_dos_set_dta(prev_dta);
    if (r.x.cflag) {
        my_doserrno = r.x.ax;
        return r.x.ax;
    }
    return 0;
}

int my_dos_findnext(my_dos_find_t far *ff)
{
    void far *prev_dta = my_dos_set_dta(ff);
    r.x.ax = 0x4f00;
    intdosx(&r, &r, &sr);
    my_dos_set_dta(prev_dta);
    if (r.x.cflag) {
        my_doserrno = r.x.ax;
        return r.x.ax;
    }
    return 0;
}

int my_dos_rw(int do_write, int h, void far *mem, int length)
{
    r.h.ah = do_write ? 0x40: 0x3f;
    r.x.bx = h;
    r.x.dx = FP_OFF(mem);
    sr.ds = FP_SEG(mem);
    r.x.cx = length;
    intdosx(&r, &r, &sr);
    if (r.x.cflag) {
        my_doserrno = r.x.ax;
        return -1;
    }
    return r.x.ax;
}

int my_dos_read(int h, void far *mem, int length)
{
    return my_dos_rw(0, h, mem, length);
}

int my_dos_write(int h, const void far *mem, int length)
{
    if (length == 0) return 0;
    return my_dos_rw(1, h, (void far *)mem, length);
}

int my_dos_close(int h)
{
    r.h.ah = 0x3e;
    r.x.bx = h;
    intdos(&r, &r);
    if (r.x.cflag) {
        my_doserrno = r.x.ax;
        return -1;
    }
    return 0;
}


enum {
    RFUNC_TYPE_UNKNOWN = 0,
    RFUNC_TYPE_DOS3,
    RFUNC_TYPE_DOS4,
    RFUNC_TYPE_FAT32
};

int my_dos_get_total_sectors(int drive0, unsigned long *total_sectors, unsigned *bytes_per_sector, int *drive_type)
{
    unsigned dosver = my_dos_getver2();
    unsigned char far *dpb;
    unsigned long user_data_offset;
    unsigned sectors_per_cluster;
    unsigned total_clusters;

    if (dosver >= 0x070a) {
        /* for FAT32 partition */
        static char drvroot[] = "@:\\";
        char buf[44];

        drvroot[0] = 'A' + drive0;
        r.x.ax = 0x7303;
        memset(buf, 0, sizeof(buf));
        r.x.dx = FP_OFF(drvroot);
        sr.ds = FP_SEG(drvroot);
        r.x.cx = 0x28 + 8;
        r.x.di = FP_OFF(buf);
        sr.es = FP_SEG(buf);
        intdosx(&r, &r, &sr);
        if (!r.x.cflag) {
            *bytes_per_sector = *(unsigned short *)(buf + 0x08);
            *total_sectors = *(unsigned long *)(buf + 0x18);
            if (drive_type) *drive_type = RFUNC_TYPE_FAT32;
            return 0;
        }
    }
    r.h.ah = 0x32;
    r.h.dl = drive0 + 1;
    intdosx(&r, &r, &sr);
    if (r.h.al != 0) return -1;
    dpb = MK_FP(sr.ds, r.x.bx);
    *bytes_per_sector = *(unsigned short far *)(dpb + 2);
    user_data_offset = *(unsigned short far *)(dpb + 0x0b);
    sectors_per_cluster = 1U << dpb[5];
    total_clusters = *(unsigned short far *)(dpb + 0x0d) - 1;
    *total_sectors = user_data_offset + (unsigned long)total_clusters * sectors_per_cluster;
    if (drive_type) *drive_type = (dosver <= 0x031e) ? RFUNC_TYPE_DOS3 : RFUNC_TYPE_DOS4;

    return 0;
}

int read_drive_sector(int drive0, int drive_type, void far *buffer, unsigned long lba, unsigned count_of_sectors)
{
    struct absrwpkt {
        unsigned long lba;
        unsigned short count;
        void far *address;
    } pkt;
    pkt.lba = lba;
    pkt.count = count_of_sectors;
    pkt.address = buffer;
    if (drive_type == RFUNC_TYPE_FAT32) {
        r.x.ax = 0x7305;
        r.x.cx = 0xffff;
        r.h.dl = drive0 + 1;
        r.x.si = 0;
        r.x.bx = FP_OFF(&pkt);
        sr.ds = FP_SEG(&pkt);
        intdosx(&r, &r, &sr);
        if (r.x.cflag) {
            return my_doserrno = r.x.ax;
        }
        return 0;
    }
    if (drive_type == RFUNC_TYPE_DOS3 || drive_type == RFUNC_TYPE_DOS4) {
        dskr_do_write = 0;
        dskr_ax = drive0;
        if (drive_type == RFUNC_TYPE_DOS3) {
            dskr_bx = FP_OFF(buffer);
            dskr_ds = FP_SEG(buffer);
            dskr_cx = count_of_sectors;
        }
        else {
            dskr_bx = FP_OFF(&pkt);
            dskr_ds = FP_SEG(&pkt);
            dskr_cx = 0xffff;
        }
        return my_absrw_body();
    }
    return -1;
}

void *my_malloc(size_t n)
{
    void *p;
    if (n == 0) n = 1;
    p = malloc(n);
    if (!p) {
        fprintf(stderr, "FATAL: memory allocation failure\n");
        exit(-1);
    }
    memset(p, 0, n);
    return p;
}

void *my_realloc(void *p, size_t n)
{
    if (n == 0) n = 1;
    p = realloc(p, n);
    if (!p) {
        fprintf(stderr, "FATAL: memory re-allocation failure (size=%u)\n", n);
        exit(-1);
    }
    return p;
}

void far *my_dos_fmalloc(size_t n)
{
    unsigned long nn = n;
    r.h.ah = 0x48;
    r.x.bx = (unsigned)((nn + 15UL) >> 4);
    intdos(&r, &r);
    return r.x.cflag ? (void far *)0L : MK_FP(r.x.ax, 0);
}


void far *buffer;
unsigned buffer_size;


struct pnlst_t;
typedef struct pnlst_t {
    struct pnlst_t *next;
    char *pathname;
    unsigned char attrib;
    unsigned short wr_time;
    unsigned short wr_date;
    unsigned long size;
} pnlst_t;



enum {
    DR_NOERROR = 0,
    DR_ERROR,
    DR_ABORT
};

void print_bpseconds(U64 bytes, long seconds)
{
#define C1M 1000000UL
#define C1K 1000UL
#define C100 100U
    U64 bps = bytes / (unsigned long)seconds;

    if (bps > (C1M * 10U)) {
        printf("%lu.%02uM", (unsigned long)(bps / C1M), (unsigned long)(bps % C1M) / (C1M / C100));
    }
    else if (bps > C1K * C100) {
        printf("%lu.%02uK", (unsigned long)(bps / C1K), (unsigned long)(bps % C1K) / (C1K / C100));
    }
    else {
        printf("%lu", (unsigned long)bps);
    }
    printf("byte%s", (bps > 1)?"s":"");
}

int dummy_read_file(const char *fn, unsigned long length, const char *header, U64 *total_bytes)
{
    const long BUFCNT = 65536L / buffer_size;
    int nrdblk = 0;
    int rc = DR_NOERROR;
    unsigned long rcnt = 0;
    int h;
    time_t t0;
    long td;

    my_doserrno = 0;
    if (!header) header = "";
    printf("\r%s'%s'", header, fn);

    h = my_dos_open2(fn, 0);
    if (h == -1) {
        printf(" can't open\n");
        return DR_ERROR;
    }

    t0 = time(NULL);
    while(1) {
        int rcnt0 = my_dos_read(h, buffer, buffer_size);
        if (rcnt0 == -1) {
            printf("read error\n");
            rc = DR_ERROR;
            break;
        }
        rcnt += (unsigned)rcnt0;
        if (rcnt0 == 0 || ++nrdblk >= BUFCNT) {
            printf("\r%s'%s'", header, fn);
            printf(" %lu/%lu...", rcnt, length);
            fflush(stdout);
            if (rcnt0 == 0) break;
            if (my_dos_check_key("\x1b", 1) >= 0) {
                rc = DR_ABORT;
                break;
            }
            nrdblk = 0;
        }
    }
    td = (long)(time(NULL) - t0);
    my_dos_close(h);

    if (rc == DR_ABORT) printf(" *aborted*");
    if (rc != DR_ERROR) {
        printf(" (%lusec%s", td, (td>1)?"s":"");
        if (td) {
            printf(", ");
            print_bpseconds(rcnt, td);
            printf("/sec");
        }
        printf(")");
    }
    printf("\n");

    if (total_bytes) *total_bytes += rcnt;
    return rc;
}

int dummy_read_files(const pnlst_t *pntop)
{
    U64 total_bytes;
    int rc;
    const pnlst_t *pn;
    time_t t0;
    long td;
    unsigned i, n_pn;
    char header[128];

    for(n_pn=0, pn=pntop; pn; ++n_pn) {
        pn = pn->next;
    }
    if (n_pn == 0) {
        printf("*no file read*\n");
        return DR_ERROR;
    }
    total_bytes = 0;
    printf("Reading %u file%s\n", n_pn, (n_pn>1)?"s":"");
    t0 = time(NULL);
    for(i=0, pn=pntop; pn;) {
        ++i;
        sprintf(header, "(%u/%u) ", i, n_pn);
        rc = dummy_read_file(pn->pathname, pn->size, header, &total_bytes);
        if (rc != DR_NOERROR) break;
        pn = pn->next;
    }
    td = (long)(time(NULL) - t0);
    printf("%u file%s, time=%lusec%s", i, (i>1)?"s":"", td, (td>1)?"s":"");
    printf(", ");
    print_bpseconds(total_bytes, 1);
    if (td) {
        printf(" (");
        print_bpseconds(total_bytes, td);
        printf("/sec)");
    }
    printf("\n");
    return rc;
}


int dummy_read_drive(int drive0, const char *header, U64 *total_bytes)
{
    const long BUFCNT = 65536L / buffer_size;
    int nrdblk = 0;
    int rc = DR_NOERROR;
    unsigned long totals;
    int drive_type;
    unsigned bps;
    unsigned long rcnt = 0;
    char dletter;
    time_t t0;
    long td;
    U64 rbytes;

    my_doserrno = 0;
    if (!header) header = "";
    dletter = drive0 + 'A';
    if (my_dos_get_total_sectors(drive0, &totals, &bps, &drive_type) != 0) {
        printf("%sdrive %c not ready (or not a FAT disk)\n", header, dletter);
        return DR_ERROR;
    }
    printf("Reading drive %c (%lu sectors, ", dletter, totals);
    print_bpseconds((U64)totals*bps, 1);
    printf(")\n");

    printf("%sdrive %c:", header, 'A'+drive0);
    t0 = time(NULL);
    while(1) {
        unsigned long csec = totals - rcnt;
        if ((buffer_size / bps) < csec) csec = (buffer_size / bps);
        if (read_drive_sector(drive0, drive_type, buffer, rcnt, (unsigned)csec) != 0) {
            printf(" read error\n");
            rc = DR_ERROR;
            break;
        }
        rcnt += csec;
        if (rcnt >= totals || ++nrdblk >= BUFCNT) {
            printf("\r%sdrive %c:", header, dletter);
            printf(" %lu/%lu...", rcnt, totals);
            fflush(stdout);
            if (rcnt >= totals) break;
            if (my_dos_check_key("\x1b", 1) >= 0) {
                rc = DR_ABORT;
                break;
            }
            nrdblk = 0;
        }
    }
    td = (long)(time(NULL) - t0);

    rbytes = (U64)rcnt * bps;
    if (rc == DR_ABORT) printf(" *aborted*");
    if (rc != DR_ERROR) {
        printf(" (%lusec%s", td, (td>1)?"s":"");
        if (td) {
            printf(", ");
            print_bpseconds(rbytes, td);
            printf("/sec");
        }
        printf(")");
    }
    printf("\n");

    if (total_bytes) *total_bytes += rbytes;
    return rc;
}


static const char *last_sep_next(const char *s)
{
    const char *p_sl, *p_bs;
    p_sl = strrchr(s, '/');
    p_bs = strrchr(s, '\\');
    if (!p_sl && !p_bs) {
        return (isalpha(*s) && s[1] == ':') ? (s + 2) : s;
    }
    p_sl = p_sl ? p_sl + 1 : s;
    p_bs = p_bs ? p_bs + 1 : s;
    return p_sl > p_bs ? p_sl : p_bs;
}


pnlst_t * get_pathnames(const char *filepattern, pnlst_t *pnprev, int *rc_cnt)
{
    char tmp_dta[128];
    my_dos_find_t *ff = (my_dos_find_t *)tmp_dta;
    int rc;
    size_t plen;
    const char *fp_sl;
    pnlst_t *p_first = NULL;
    pnlst_t *p = NULL;

    fp_sl = last_sep_next(filepattern);
    plen = fp_sl - filepattern;

    if (pnprev) {
        p_first = pnprev;
        while(pnprev->next) pnprev = pnprev->next;
    }

    ff->name[0] = '\0';
    rc = my_dos_findfirst(filepattern, 0x27, ff);
    while(rc == 0) {
        if (ff->name[0] == '\0') break;
        p = my_malloc(sizeof(pnlst_t));
        if (!p_first) p_first = p;
        p->pathname = my_malloc(plen + 13);
        if (plen) memcpy(p->pathname, filepattern, plen);
        strcpy(p->pathname + plen, ff->name);
        p->attrib = ff->attrib;
        p->wr_time = ff->wr_time;
        p->wr_date = ff->wr_date;
        p->size = ff->size;
        if (pnprev) pnprev->next = p;
        pnprev = p;
        if (rc_cnt) ++(*rc_cnt);
        ff->name[0] = '\0';
        rc = my_dos_findnext(ff);
    }
    return p_first;
}


int optHelp = 0;
int prmIndex = -1;
long optN = 0;
#if defined __COMPACT__ || defined __LARGE__ || defined __HUGE__
unsigned optB = 32768U;
#else
unsigned optB = 4096;
#endif
int optD = 0;
int optF = 0;

int mygetopt(int argc, char **argv)
{
    int i = 0;
    while(--argc > 0) {
        char *s = *++argv;
        ++i;
        if (*s == '-' || *s == '/') {
            char c = *++s;
            if (c) ++s;
            c = toupper(c);
            switch(c) {
                case '?': case 'H': optHelp = 1; break;
                case 'D': optD = 1; break;
                case 'F': optF = 1; break;
                case 'N': case 'B':
                    if (*s == '=' || *s == ':') ++s;
                    if (!*s && argv[1]) {
                        s = *++argv;
                        --argc;
                    }
                    if (c == 'N') {
                        optN = strtol(s, NULL, 0);
                        if (optN < 0) {
                            fprintf(stderr, "option -n:parameter error\n");
                            exit(1);
                        }
                    }
                    else if (c == 'B') {
                        long n = strtol(s, NULL, 0);
                        if (n == 2048 || n == 4096 || n == 8192 || n == 16384 || n == 32768) {
                            optB = (unsigned)n;
                        }
                        else {
                            fprintf(stderr, "option -b:parameter error\n");
                            exit(1);
                        }
                    }
                    break;
            }
        }
        else {
            prmIndex = i;
            return prmIndex;
        }
    }
    return 0;
}

void usage(const char *progname)
{
    const char msg[] =
        "usage: %s [-b buffer_size] files...\n"
        " Read file(s) and display speed\n"
        "   or: %s [-b buffer_size] drive:\n"
        " Read all sectors in the specified drive (A: to Z:) and display speed\n"
        ;
    if (!progname) progname = "(MYPROG)";
    printf(msg, progname, progname);
}


time_t wait_timetick(void)
{
    time_t t, t0;
    t0 = time(NULL);
    while(t0 == (t = time(NULL))) {
        my_dos_check_key(NULL, 0); /* just for droping keystrokes */
    }
    return t;
}

int main(int argc, char **argv)
{
    int rc = -1;
    int i, files;
    pnlst_t *pnbase = NULL;
    int drive = -1;

    rc = mygetopt(argc, argv);
    if (rc <= 0 || optHelp) {
        usage("READTEST");
        return !optHelp;
    }
    if (prmIndex < argc && isalpha(argv[prmIndex][0]) && argv[prmIndex][1] == ':' && argv[prmIndex][2] == '\0') {
        drive = toupper(argv[prmIndex][0]) - 'A';
    }
    else {
        for (i=prmIndex, pnbase = NULL, files = 0; i<argc; ++i) {
            pnbase = get_pathnames(argv[i], pnbase, &files);
        }
        if (!pnbase) {
            fprintf(stderr, "No file matched\n");
            exit(1);
        }
    }
    if (optN == 0) optN = 1;
    buffer_size = optB;
    buffer = my_dos_fmalloc(buffer_size);
    if (!buffer) {
        fprintf(stderr, "FATAL: memory not enough\n");
        exit(-1);
    }

    wait_timetick();

    if (pnbase) rc = dummy_read_files(pnbase);
    else if (drive >= 0) rc = dummy_read_drive(drive, NULL, NULL);

    return rc;
}

