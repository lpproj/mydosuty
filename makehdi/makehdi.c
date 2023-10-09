/*
    makehdi: build a disk image file for PC-98 emulator (Anex86/T98Next/NP2)

    to build with gcc:
    gcc -Wall -O2 -s -o makehdi makehdi.c

    with OpenWatcom:
    wcl386 -zq -s -Fr -za99 -DUSE_YA_GETOPT makehdi.c

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

#define RELEASE_DATE_STRING "20201220"

#if defined WIN32 || defined _WIN32
# undef WINVER
# undef _WIN32_WINNT
# define WINVER 0x0500
# define _WIN32_WINNT WINVER
# include <windows.h>
# ifndef FSCTL_SET_SPARSE
#  include <winioctl.h>
# endif
#endif

/* must be compliant with C99 */
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#if __STDC_VERSION__ >= 199901L
# include <inttypes.h>
# include <stdint.h>
#else
# error need C99 (or later)
#endif

#if USE_YA_GETOPT
# define YA_GETOPT_NO_COMPAT_MACRO 1
# include "ya_getopt.c"
# define getopt ya_getopt
# define getopt_long ya_getopt_long
# define getopt_long_only ya_getopt_long_only
# define optarg ya_optarg
# define optind ya_optind
# define opterr ya_opterr
# define optopt ya_optopt
# define no_argument ya_no_argument
# define required_argument ya_required_argument
# define optional_argument ya_optional_argument
#else
# include <getopt.h>
#endif

typedef int_least8_t    S8;
typedef int_least16_t   S16;
typedef int_least32_t   S32;
typedef int_least64_t   S64;
typedef uint_least8_t   U8;
typedef uint_least16_t  U16;
typedef uint_least32_t  U32;
typedef uint_least64_t  U64;
#define P32d PRIdLEAST32
#define P32u PRIuLEAST32
#define P32X PRIXLEAST32
#define P32x PRIxLEAST32
#define P64d PRIdLEAST64
#define P64u PRIuLEAST64
#define P64X PRIXLEAST64
#define P64x PRIxLEAST64
#define STATIC_INLINE static inline

#define U32_MAX  0xffffffffUL
#define U64_MAX (((U64)0xffffffffUL << 32) | 0xffffffffUL)

#ifndef O_BINARY
# define O_BINARY  0
#endif


#if MSDOS || __DOS__ || __OS2__ || (UINT_MAX <= 65535U)
# define SPARSE_FILE_SUPPORTED 0
#else
# define SPARSE_FILE_SUPPORTED 1
#endif

STATIC_INLINE U16 pokeU16(void *mem, U16 v)
{
    *(U8 *)mem = (U8)v & 0xffU;
    *((U8 *)mem + 1) = v >> 8;
    return v;
}

STATIC_INLINE U32 pokeU32(void *mem, U32 v)
{
    *(U8 *)mem = (U8)v & 0xffU;
    *((U8 *)mem + 1) = (v >> 8) & 0xffU;
    *((U8 *)mem + 2) = (v >> 16) & 0xffU;
    *((U8 *)mem + 3) = (v >> 24) & 0xffU;
    return v;
}

#if 0
STATIC_INLINE U16 peekU16(const void * const mem)
{
    U16 v = *(const U8 *)mem
          | ((U16)(*((const U8 *)mem + 1)) << 8)
        ;
    return v;
}

STATIC_INLINE U32 peekU32(const void * const mem)
{
    U32 v = *(const U8 *)mem
          | ((U16)(*((const U8 *)mem + 1)) << 8)
          | ((U32)(*((const U8 *)mem + 2)) << 16)
          | ((U32)(*((const U8 *)mem + 3)) << 24)
        ;
    return v;
}
#endif

#define OFF_HDI_TOP         0
#define OFF_HDI_TYPE        4
#define OFF_HDI_MEGABYTES   4
/*
    hditype (made by anex86.exe)
    0x05    5M (c153 h4 s17 b512)
    0x0a   10M (c320 h4 s17 b512)
    0x14   20M (c615 h4 s17 b512)
    0x28   40M (c615 h8 s17 b512)
    0x00   80M (c1280 h8 s17 b512)

    0x00   user
*/
#define OFF_HDI_HEADERSIZE  8
#define OFF_HDI_HDDSIZE     12
#define OFF_HDI_SECTORSIZE  16
#define OFF_HDI_SECTORS     20
#define OFF_HDI_HEADS       24
#define OFF_HDI_CYLINDERS   28
#define HDI_HEADERSIZE      4096

#define OFF_NHD_TOP         0
#define OFF_NHD_SIGNATURE   0
#define NHD_SIGNATURE       "T98HDDIMAGE.R0\0"
#define NHD_SIGNATURE_SIZE  16
#define OFF_NHD_COMMENT     (NHD_SIGNATURE_SIZE)
#define NHD_COMMENT_SIZE    256
#define OFF_NHD_HEADERSIZE  (NHD_SIGNATURE_SIZE+NHD_COMMENT_SIZE)
#define OFF_NHD_CYLINDERS   (NHD_SIGNATURE_SIZE+NHD_COMMENT_SIZE + 4)
#define OFF_NHD_HEADS       (NHD_SIGNATURE_SIZE+NHD_COMMENT_SIZE + 8)
#define OFF_NHD_SECTORS     (NHD_SIGNATURE_SIZE+NHD_COMMENT_SIZE + 10)
#define OFF_NHD_SECTORSIZE  (NHD_SIGNATURE_SIZE+NHD_COMMENT_SIZE + 12)
#define OFF_NHD_RESERVED    (NHD_SIGNATURE_SIZE+NHD_COMMENT_SIZE + 14)
#define NHD_HEADERSIZE      512

#define OFF_V98_TOP         0
#define OFF_V98_SIGNATURE   0
#define V98_SIGNATURE       "VHD1.00\0"
#define V98_SIGNATURE_SIZE  8
#define V98_SIGNATURE_SIZE_LOOSE    5
#define OFF_V98_SIGVERSION  3
#define OFF_V98_COMMENT     8
#define V98_COMMENT_SIZE    128

#define OFF_V98_HEADER2     (8 + 128)
#define OFF_V98_SIZE_MB     (OFF_V98_HEADER2 + 4)
#define OFF_V98_SECTORSIZE  (OFF_V98_HEADER2 + 6)
#define OFF_V98_SECTORS     (OFF_V98_HEADER2 + 8)
#define OFF_V98_HEADS       (OFF_V98_HEADER2 + 9)
#define OFF_V98_CYLINDERS   (OFF_V98_HEADER2 + 10)
#define OFF_V98_TOTALS      (OFF_V98_HEADER2 + 12)
#define V98_HEADERSIZE      220


static int setupHDHeader_hdi(void *mem, unsigned cylinders, unsigned heads, unsigned sectors, unsigned bytes_per_sector)
{
    U64 total = (U64)cylinders * heads * sectors * bytes_per_sector;
    U8 *m = (U8 *)mem;

    if (total >= 0xffffffffUL) return -1;
    pokeU32(m, 0);
    pokeU32(m + OFF_HDI_TYPE, 0);   /* 0x28 */
    pokeU32(m + OFF_HDI_HEADERSIZE, HDI_HEADERSIZE);
    pokeU32(m + OFF_HDI_HDDSIZE, (U32)total);
    pokeU32(m + OFF_HDI_SECTORSIZE, bytes_per_sector);
    pokeU32(m + OFF_HDI_SECTORS, sectors);
    pokeU32(m + OFF_HDI_HEADS, heads);
    pokeU32(m + OFF_HDI_CYLINDERS, cylinders);
    return 0;
}

static int setupHDHeader_nhd(void *mem, unsigned cylinders, unsigned heads, unsigned sectors, unsigned bytes_per_sector)
{
    U8 *m = (U8 *)mem;

    strcpy((char *)m, NHD_SIGNATURE);
    pokeU32(m + OFF_NHD_HEADERSIZE, NHD_HEADERSIZE);
    pokeU32(m + OFF_NHD_CYLINDERS, cylinders);
    pokeU16(m + OFF_NHD_HEADS, heads);
    pokeU16(m + OFF_NHD_SECTORS, sectors);
    pokeU16(m + OFF_NHD_SECTORSIZE, bytes_per_sector);
    return 0;
}

static int setupHDHeader_v98hdd(void *mem, unsigned cylinders, unsigned heads, unsigned sectors, unsigned bytes_per_sector)
{
    U64 total = (U64)cylinders * heads * sectors * bytes_per_sector;
    U8 *m = (U8 *)mem;
    U32 totalmb = total * bytes_per_sector / (1024UL * 1024UL);

    strcpy((char *)m, V98_SIGNATURE);
    pokeU16(m + OFF_V98_SIZE_MB, (totalmb > 0xffffUL) ? 0xffffU : (U16)totalmb);
    pokeU16(m + OFF_V98_SECTORSIZE, bytes_per_sector);
    m[OFF_V98_SECTORS] = (U8)sectors;
    m[OFF_V98_HEADS] = (U8)heads;
    pokeU16(m + OFF_V98_CYLINDERS, cylinders);
    pokeU32(m + OFF_V98_TOTALS, (U32)total);
    return 0;
}


static void * my_malloc(unsigned bytes)
{
    void *p;
    if (bytes == 0) bytes = 1;
    p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "FATAL: memory allocation failure (%ubytes)\n", bytes);
        exit(-1);
    }
    memset(p, 0, bytes);
    return p;
}


#if ULONG_MAX == U32_MAX
# define my_strtou32(s,e,b)  strtoul(s,e,b)
#else
static U32 my_strtou32(const char *str, char **strend, int base)
{
    unsigned long v = strtoul(str, strend, base);
    return (v == ULONG_MAX || v >= U32_MAX) ? U32_MAX : (U32)v;
}
#endif

static U32 my_getu32(const char *str)
{
    char *s = NULL;
    U32 v = my_strtou32(str, &s, 0);
    return (s && *s) ? U32_MAX : v;
}

static U64 my_getu64(const char *str, int allow_unit)
{
    char *s = NULL;
    unsigned long long base = 1;
    unsigned long long v = strtoull(str, &s, 0);
    if (v != ULLONG_MAX && s && *s) {
        unsigned scale = (toupper(s[1]) == 'I') ? 1024 : 1000;
        switch (toupper(*s)) {
            case 'T': if (allow_unit) base *= scale; /* fallthrough */
            case 'G': if (allow_unit) base *= scale; /* fallthrough */
            case 'M': if (allow_unit) base *= scale; /* fallthrough */
            case 'K':
                if (allow_unit) {
                    base *= scale;
                    break;
                }
                /* fallthrough */ /* if (!allow_unit) */
            default:
                return U64_MAX;
        }
    }
    if ((U64_MAX / base) <= v) return U64_MAX;
    return v == ULLONG_MAX ? U64_MAX : (U64)v * base;
}

static int my_strcasecmp(const char *s1, const char *s2)
{
    int v = 0;
    do {
        if (*s1 == '\0' && *s2 == '\0') break;
        v = toupper(*s1) - toupper(*s2);
        ++s1;
        ++s2;
    } while (v == 0);
    return v;
}

#if defined WIN32 || defined _WIN32
static int win32_qd_ftruncate(int fd, U64 length)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    LONG posh, posl;
    LONG new_posh, new_posl;
    new_posh = posh = (LONG)(length >> 32);
    posl = (LONG)(U32)length;
    new_posl = SetFilePointer(h, posl, &new_posh, FILE_BEGIN);
    if (new_posl != posl || new_posh != posh) return -1;
    return SetEndOfFile(h) ? 0 : -1;
}
# define ftruncate(f,l) win32_qd_ftruncate(f,l)
#endif

enum {
    DISK_DEFAULT = 0,
    DISK_RAW,
    DISK_HDI,
    DISK_NHD,
    DISK_V98
};

int optHelp;
int optQ;
int optV;
U32 optC;
U32 optH;
U32 optS;
U32 optB;
U64 optSize;
int optF = DISK_DEFAULT;
int optHeader;
int optOverwrite;
char *prm_fname;


static int size_to_chs_ide(void)
{
    U64 ts = optSize / optB;
    optH = 8;
    optS = 17;
    optC = ts / (optH * optS);
    if (optC < 0xffff) return 0;
    optH = 16;
    optS = 63;
    optC = ts / (optH * optS);
    if (optC < 0xffff) return 0;
    optH = 16;
    optS = 255;
    optC = ts / (optH * optS);
    if (optC < 0xffff) return 0;
    return -1;
}

static int size_to_chs_scsi(void)
{
    U64 ts = optSize / optB;
    optH = 8;
    optS = 32;
    optC = ts / (optH * optS);
    if (optC < 0xffff) return 0;
    optH = 8;
    optS = 128;
    optC = ts / (optH * optS);
    if (optC < 0xffff) return 0;
    optH = 16;
    optS = 255;
    optC = ts / (optH * optS);
    if (optC < 0xffff) return 0;
    return -1;
}

int checkHDParams(void)
{
    const char s_inv[] = "Invalid parameter: ";
    int rc = 0;
    if (optB == 0) optB = 512;
    if (!(optB == 256 || optB == 512 || optB == 1024 || optB == 2048)) {
        fprintf(stderr, "%sbytes per sector\n", s_inv);
        return -1;
    }
    if (optSize == U64_MAX) {
        fprintf(stderr, "%sdisk size\n", s_inv);
        return -1;
    }
    if (optSize > 0 && optC == 0 && optH == 0 && optS == 0) {
        switch (optF) {
            case DISK_V98:
                if (size_to_chs_scsi() == -1) {
                    fprintf(stderr, "%shdi image too large\n", s_inv);
                    return -1;
                }
                break;
            case DISK_HDI:
            case DISK_NHD:
                if (size_to_chs_ide() == -1) {
                    fprintf(stderr, "%shdi image too large\n", s_inv);
                    return -1;
                }
                break;
        }
    }
    if (optH == 0 || optH > 255U) {
        fprintf(stderr, "%sheads\n", s_inv);
        return -1;
    }
    if (optS == 0 || optS > 255U) {
        fprintf(stderr, "%ssectors per track\n", s_inv);
        return -1;
    }
    if (optC == 0) {
        U32 hsb = optH * optS * optB;
        if (optSize == 0) {
            fprintf(stderr, "%sneed disk size (--size) or cylinders (-c)\n", s_inv);
            return -1;
        }
        optC = optSize / hsb  + ((optSize % hsb) > 0);
    }
    if (optC == 0 || optC > 65535U) {
        fprintf(stderr, "%scylinders\n", s_inv);
        return -1;
    }
    if (optF == DISK_HDI && (U64)optC * optH * optS * optB >= 0xffffffffUL) {
        fprintf(stderr, "%shdi image too large\n", s_inv);
        return -1;
    }
    if (rc < 0) {
        fprintf(stderr, "Parameter error\n");
    }
    return 0;
}


void *header;
unsigned header_size;

int buildHDHeader(int hdtype)
{
    int rc = -1;
    switch(hdtype) {
        case DISK_RAW:
            header_size = 0;
            header = my_malloc(header_size);
            rc = 0;
            break;
        case DISK_DEFAULT:
        case DISK_HDI:
            header_size = HDI_HEADERSIZE;
            header = my_malloc(header_size);
            rc = setupHDHeader_hdi(header, optC, optH, optS, optB);
            break;
        case DISK_NHD:
            header_size = NHD_HEADERSIZE;
            header = my_malloc(header_size);
            rc = setupHDHeader_nhd(header, optC, optH, optS, optB);
            break;
        case DISK_V98:
            header_size = V98_HEADERSIZE;
            header = my_malloc(header_size);
            rc = setupHDHeader_v98hdd(header, optC, optH, optS, optB);
            break;
    }
    return rc;
}


static int my_fw(const void *m, unsigned bytes, int fo)
{
    const char *s = m;
    unsigned r;

    for (r=0; r<bytes;) {
        int r1 = write(fo, s, bytes - r);
        if (r1 == -1) return -1;
        r += (unsigned)r1;
        s += r1;
    }
    return 0;
}

static int my_fw_onecyl(const void *m, unsigned bytes, U32 heads, U32 sectors, int fo)
{
    U32 h, s;
    for (h = 0; h < heads; ++h) {
        for (s = 0; s < sectors; ++s) {
            int rc = my_fw(m, bytes, fo);
            if (rc < 0) return rc;
        }
    }
    return 0;
}

static void print_curcyl(U32 cur_cyl, U32 total_cyl)
{
    printf("\rbuilding cylinder %"P32u"/%"P32u" ", cur_cyl, total_cyl);
    if (total_cyl > 1024) {
        U64 c = ((U64)cur_cyl * 10000U) / total_cyl;
        printf("(%u.%02u%%)", (unsigned)(c / 100U), (unsigned)(c % 100));
    }
    else {
        U64 c = ((U64)cur_cyl * 1000U) / total_cyl;
        printf("(%u.%u%%)", (unsigned)(c / 10U), (unsigned)(c % 10));
    }
    printf(" ... ");
    fflush(stdout);
}


int buildHDImage(const char *filename, int do_overwrite)
{
    int fo;
    U64 total64, totalb;
    int rc;
    char *nullbuff;
    U32 cur_cyl;
    int is_sparse = SPARSE_FILE_SUPPORTED;

    nullbuff = my_malloc(optB);
    rc = buildHDHeader(optF);
    if (rc < 0) return rc;
    total64 = (U64)optC * optH * optS;
    totalb = total64 * optB;
    if (!optQ) {
        printf("cylinder=%"P32u", heads=%"P32u", sectors=%"P32u", bytes per sector=%"P32u"\n", optC, optH, optS, optB);
        printf("total sectors=%"P64u" (0x%"P64x"), size of raw image=", total64, total64);
        if (totalb > 1000UL * 1000UL * 1000UL) {
            U32 totalG100 = totalb / (1000UL * 1000UL * 10UL);
            printf("%"P32u".%02uG", totalG100 / 100, (unsigned)(totalG100 % 100));
        }
        else if (totalb > 1000UL * 1000UL) {
            U32 totalM10 = totalb / (1000UL * 100UL);
            printf("%"P32u".%uM", totalM10 / 10, (unsigned)(totalM10 % 10));
        }
        else {
            printf("%"P32u, (U32)totalb);
        }
        printf("bytes\n");
    }
    fo = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | (do_overwrite ? 0 : O_EXCL), 0644);
    if (fo == -1) {
        if (errno == EEXIST && !do_overwrite) {
            fprintf(stderr, "The file '%s' already exists\n", filename);
        }
        else {
            fprintf(stderr, "Can't open the file '%s' (errno=%d)\n", filename, errno);
        }
        return -1;
    }
    lseek(fo, 0L, SEEK_SET); /* to be safe... */
#if defined WIN32 || defined _WIN32
    {
        DWORD dw = 0;
        is_sparse = (DeviceIoControl((HANDLE)_get_osfhandle(fo), FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dw, NULL) != 0);
    }
#endif
    if (header_size > 0) rc = my_fw(header, header_size, fo);
    if (rc == 0 && !optHeader) {
        if (is_sparse) {
            if (!optQ) print_curcyl(0, optC);
            rc = ftruncate(fo, totalb + header_size);
        }
        else {
            for (cur_cyl=0; cur_cyl < optC; ++cur_cyl) {
                if (!optQ) print_curcyl(cur_cyl, optC);
                rc = my_fw_onecyl(nullbuff, optB, optH, optS, fo);
                if (rc < 0) break;
            }
        }
        if (rc == 0 && !optQ) {
            print_curcyl(optC, optC);
            printf("ok!\n");
        }
    }
    close(fo);
    return rc;
}


static struct option optlong_params[] = {
    { "help",   no_argument, 0, '?' },
    /* without short option */
    { "header-only", required_argument, 0, 0 },
    { "force",   no_argument, 0, 0 },
    { "overwrite",   no_argument, 0, 0 },
    /* with short option */
    { "quiet",   no_argument, 0, 'q' },
    { "verbose",   no_argument, 0, 'v' },
    { "cylinders", required_argument, 0, 'c' },   /* c */
    { "heads", required_argument, 0, 'h' },       /* h */
    { "sectors", required_argument, 0, 's' },     /* s */
    { "bytes", required_argument, 0, 'b' },       /* b */
    { "bytes-per-sector", required_argument, 0, 'b' },       /* b */
    { "sector-size", required_argument, 0, 'b' },       /* b */
    { "format", required_argument, 0, 'f' },      /* f */
    { "total", required_argument, 0, 't' },       /* t */
    { "total-sectors", required_argument, 0, 't' },       /* t */
    { "size", required_argument, 0, 'S' },       /* S */
    { 0, 0, 0, 0 }
};
static char optshort_params[] = "qc:h:s:b:f:S:";

int my_getopt(int argc, char **argv)
{
    int cur_optind;
    while(1) {
        int optlong_index = 0;
        int c;
        cur_optind = optind ? optind : 1;
        c = getopt_long(argc, argv, optshort_params, optlong_params, &optlong_index);
        if (c == -1) break;
        switch (c) {
            case 0:
                switch (optlong_index) {
                    case 1: optHeader = 1; break; /* --header-only */
                    case 2:
                    case 3:
                        optOverwrite = 1; break; /* --force, --overwrite */
                }
                break;
            case 'q': optQ = 1; break;
            case 'v': ++optV; break;
            case 'c':
                optC = my_getu32(optarg);
                break;
            case 'h':
                optH = my_getu32(optarg);
                break;
            case 's':
                optS = my_getu32(optarg);
                break;
            case 'b':
                optB = my_getu32(optarg);
                if (optB != 256 && optB != 512) {
                    fprintf(stderr, "Invalid sector size\n");
                    return -1;
                }
                break;
            case 'S':
                optSize = my_getu64(optarg, 1);
                break;
            case 'f':
                if (my_strcasecmp(optarg, "raw")==0) optF = DISK_RAW;
                else if (my_strcasecmp(optarg, "hdi")==0) optF = DISK_HDI;
                else if (my_strcasecmp(optarg, "nhd")==0) optF = DISK_NHD;
                else if (my_strcasecmp(optarg, "v98")==0) optF = DISK_V98;
                else if (my_strcasecmp(optarg, "hdd")==0) optF = DISK_V98;
                else if (my_strcasecmp(optarg, "mo128")==0) {
                    /* 128MB MO: 9994cyliders * 25sectors - defect_sectors(1024) = 248826 (127398912bytes) */
                    /* workaround: 248826 = 2 * 3 * 113 * 367 */
                    optF = DISK_RAW;
                    optC = 367 * 2 * 3;
                    optH = 1;
                    optS = 113;
                    optB = 512;
                }
                else if (my_strcasecmp(optarg, "mo230")==0) {
                    optF = DISK_RAW;
                    optC = 17853;
                    optH = 1;
                    optS = 25;
                    optB = 512;
                }
                else if (my_strcasecmp(optarg, "mo540")==0) {
                    optF = DISK_RAW;
                    optC = 41660;
                    optH = 1;
                    optS = 25;
                    optB = 512;
                }
                else if (my_strcasecmp(optarg, "mo640")==0) {
                    optF = DISK_RAW;
                    optC = 18256;
                    optH = 1;
                    optS = 17;
                    optB = 2048;
                }
                else if (my_strcasecmp(optarg, "mo650")==0 || my_strcasecmp(optarg, "mo5_650")==0) {
                    optF = DISK_RAW;
                    optC = 4656;
                    optH = 1;
                    optS = 64;
                    optB = 1024;
                }
                else {
                    fprintf(stderr, "Invalid disk format\n");
                    return -1;
                }
                break;
        }
    }
    if (cur_optind < argc) prm_fname = argv[cur_optind];
    return 0;
}


int my_fextcmp(const char *fname, const char *ext)
{
    const char *fn_e = strrchr(fname, '.');
    if (!fn_e) fn_e = fname;
    return my_strcasecmp(fn_e, ext);
}

void usage(void)
{
    const char msg[] = 
        "MAKEHDI - make a blank HDI or other disk image (release " RELEASE_DATE_STRING ")\n"
        "usage : makehdi [--overwrite] [-f disk_type] [--size disk_size] [-c cylinders] [-h heads] [-s sectors_per_track] [-b bytes_per_sector] imagefile\n"
        "disk_type : hdi    Anex86    (*.hdi)\n"
        "            nhd    T98Next   (*.nhd)\n"
        "            v98    Virtual98 (*.hdd)\n"
        "            raw    raw (flat) image\n"
        "            mo128  raw image for 3.5\" 128MB optical disk\n"
        "            mo230  raw image for 3.5\" 230MB optical disk\n"
        "            mo540  raw image for 3.5\" 540MB optical disk\n"
        "            mo640  raw image for 3.5\" 640MB optical disk\n"
        "            mo650  raw image for 5.25\" (130mm) 650MB optical disk\n"
        "                   (300MB single sided image, 1024bytes per sector)\n"
        ;
    printf("%s", msg);
}

int main(int argc, char **argv)
{
    int rc;

    rc = my_getopt(argc, argv);
#if TEST
    printf("getopt rc=%d C:%"P32u" H:%"P32u" S:%"P32u" B:%"P32u" F:%d size:%"P64u" filename:%s\n", rc, optC, optH, optS, optB, optF, optSize, prm_fname ? prm_fname : "(null)");
#endif
    if (rc < 0 || !prm_fname) {
        usage();
        return 1;
    }
    if (optF == DISK_DEFAULT) {
        if (my_fextcmp(prm_fname, ".hdi")==0) optF = DISK_HDI;
        else if (my_fextcmp(prm_fname, ".nhd")==0) optF = DISK_NHD;
        else if (my_fextcmp(prm_fname, ".hdd")==0) optF = DISK_V98;
    }
    rc = checkHDParams();
    if (rc == 0) rc = buildHDImage(prm_fname, optOverwrite);

    return rc;
}

