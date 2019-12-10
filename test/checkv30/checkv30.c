/*
    checkv30 - a test for "quick check CPU type for NEC V20/V30"

    target: 16bit DOS (v2 or later)

    to build with (Open)Watcom:
        wcl -zq -s -bt=dos checkv30.c

    to build with LSI-C86 3.30c(eval):
        lcc checkv30 -lintlib

    to build with ia16-elf-gcc (https://github.com/tkchia/build-ia16):
        ia16-elf-gcc -s -mcmodel=small -march=i8086 -o checkv30.exe checkv30.c


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

#include <limits.h>
#if (INT_MAX) > 32767
# error This code should work with 16bit x86 target...
#endif
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
# define FAR        _far
# define CDECL      _cdecl
#elif defined(LSI_C)
# define FAR        far
# define CDECL
#else
# define FAR        far
# define CDECL      cdecl
#endif


#if defined(LSI_C)
# define DECLARE_CDECL_PARAM_PREFIX  unsigned dummy0, unsigned dummy1, unsigned dummy2, unsigned dummy3,
# define INVOKE_CDECL_PARAM_PREFIX   0,0,0,0,
#else
# define DECLARE_CDECL_PARAM_PREFIX
# define INVOKE_CDECL_PARAM_PREFIX
#endif

#if defined(__GNUC__)
typedef unsigned  __far  (__attribute__((cdecl)) * CDECL_FARFUNC_PARAM1)( DECLARE_CDECL_PARAM_PREFIX unsigned );

#else
typedef unsigned  (FAR CDECL *CDECL_FARFUNC_PARAM1)( DECLARE_CDECL_PARAM_PREFIX unsigned );
#endif


static unsigned char test_aam_cdecl_farcode[] = {
    0x55,                   /* push bp */
    0x89, 0xe5,             /* mov bp, sp */
    0x8b, 0x46, 0x06,       /* mov ax, [bp+6] */
#define TEST_AAM_FAR_OPCODE_OFFSET  0x06
#define TEST_AAM_FAR_BASE_OFFSET    0x07
    0xd4, 0x0a,             /* aam (aam 10) */
    0x5d,                   /* pop bp */
    0xcb                    /* retf */
};

static unsigned char test_setflags_cdecl_farcode[] = {
    0x55,                   /* push bp */
    0x89, 0xe5,             /* mov bp, sp */
    0x8b, 0x46, 0x06,       /* mov ax, [bp+6] */
    0x9c,                   /* pushf */
    0x50, 0x9d,             /* mov flags, ax (push ax; popf) */
    0x9c, 0x58,             /* mov ax, flags (pushf; pop ax) */
    0x9d,                   /* popf */
    0x5d,                   /* pop bp */
    0xcb                    /* retf */
};

static unsigned char test_shrcl_cdecl_farcode[] = {
    0x55,                   /* push bp */
    0x89, 0xe5,             /* mov bp, sp */
    0x8b, 0x46, 0x06,       /* mov ax, [bp+6] */
    0x51,                   /* push cx */
    0x89, 0xc1,             /* mov cx, ax */
    0x31, 0xc0,             /* xor ax, ax */
    0x48,                   /* dec ax (ax=-1) */
    0xd3, 0xe8,             /* shr ax, cl */
    0x59,                   /* pop cx */
    0x5d,                   /* pop bp */
    0xcb                    /* retf */
};


unsigned do_aamaad(unsigned char opcode, unsigned bcdvalue, unsigned char base)
{
    if (opcode != 0xd4 && opcode != 0xd5) return (unsigned)(-1);
    
    if (base == 0) base = 10;   /* to be safe (for some emulators...) */
    test_aam_cdecl_farcode[TEST_AAM_FAR_OPCODE_OFFSET] = opcode;
    test_aam_cdecl_farcode[TEST_AAM_FAR_BASE_OFFSET] = base;
    return ((CDECL_FARFUNC_PARAM1)test_aam_cdecl_farcode)( INVOKE_CDECL_PARAM_PREFIX bcdvalue);
}
#define do_aam(bcd,base)    do_aamaad(0xd4,bcd,base)
#define do_aad(bcd,base)    do_aamaad(0xd5,bcd,base)

unsigned do_setflags(unsigned value)
{
    return ((CDECL_FARFUNC_PARAM1)test_setflags_cdecl_farcode)( INVOKE_CDECL_PARAM_PREFIX value);
}

unsigned do_shr_cl(unsigned shiftcnt)
{
    return ((CDECL_FARFUNC_PARAM1)test_shrcl_cdecl_farcode)( INVOKE_CDECL_PARAM_PREFIX shiftcnt);
}



int print_shifttest(void)
{
    unsigned rv = do_shr_cl(128);
    
    printf("SHR with CL test (check 8086 or 80186+)\n");
    printf("---------------------------------------\n");
    printf("SHR 0x%X,128 -> 0x%X\n", (unsigned)(-1), rv);
    printf("CPU...guess %s\n", (rv == 0) ? "8086" : "80186 or later");
    printf("\n");
    return rv != 0;
}


int print_flagtest(void)
{
    int rc = -1;
    unsigned flags0000, flagsf000;
    
    flags0000 = do_setflags(0x0000);
    flagsf000 = do_setflags(0xf000U);
    printf("set FLAGS test (check 8086, 286 or 386+)\n");
    printf("----------------------------------------\n");
    printf("flags = 0x0000 -> 0x%04X\n", flags0000);
    printf("flags = 0xf000 -> 0x%04X\n", flagsf000);

    flags0000 &= 0xf000U;
    flagsf000 &= 0xf000U;
    printf("CPU...");
    if (flags0000 == flagsf000) {
        if (flagsf000 == 0xf000) {
            rc = 0;
            printf("guess 8086, NEC V20/V30 or 80186");
        }
        else if (flags0000 == 0x0000) {
            rc = 2;
            printf("guess 80286");
        }
        else printf("oops, unspecified");
    }
    else {
        if ((flags0000 & 0x8000U) || (flagsf000 & 0x8000U))
            printf("oops, unspecified");
        else {
            rc = 3;
            printf("guess i386 or later");
        }
    }
    printf("\n");

    printf("\n");
    return rc;
}


static int aamaad_sub(int is_aad)
{
    unsigned param;
    unsigned a10, a16;
    char opcharMD;
    unsigned char opcodeMD;

    if (is_aad) {
        opcharMD = 'D';
        opcodeMD = 0xd5;
        param = 0x0100;
    } else {
        opcharMD = 'M';
        opcodeMD = 0xd4;
        param = 16;
    }
    a10 = do_aamaad(opcodeMD, param, 10);
    a16 = do_aamaad(opcodeMD, param, 16);

    printf("AX=%u (0x%X); AA%c 10 -> AX=0x%04X", param, param, opcharMD, a10);
    printf("\n");
    printf("AX=%u (0x%X); AA%c 16 -> AX=0x%04X", param, param, opcharMD, a16);
    printf(" (guess %sNEC V20/V30)\n", a10==a16 ? "" : "not ");

    return a10==a16;
}

int print_aamaadtest_necv30(void)
{
    int rcAAM, rcAAD;

    printf("AAD/AAM test (check 8086 or NEC V30)\n");
    printf("------------------------------------\n");
    rcAAM = aamaad_sub(0);
    rcAAD = aamaad_sub(1);
    if (rcAAM != rcAAD) {
        printf("*** bad AAD/AAM emulation ***\n\n");
        return 0;
    }
    printf("CPU...guess %sNEC V20/V30\n", rcAAD ? "" : "not ");
    printf("\n");
    return rcAAD;
}



int main(void)
{
    int is_v30, is_186later, major_cpu;

    is_v30 = print_aamaadtest_necv30();
    is_186later = print_shifttest();
    major_cpu = print_flagtest();
    if (major_cpu == 0) {
        if (is_186later > 0) major_cpu = 1;
#if 0
        /* not recommended: treat NEC V30/V20 as 80186/188
           (Misdetecting is often occured on software emulators...) */
        if (is_v30 > 0) major_cpu = 1;
#endif
    }

    return major_cpu;
}

