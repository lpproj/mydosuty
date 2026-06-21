/*
  FPTRDIFF.C - a test of "DO NOT COMPARE FAR POINTERS on 16bit x86 compilers"

  to build with...

  Microsoft C/C++ (up to VC++1.52) : cl fptrdiff.c
  Turbo C++ (up to 5.0x) : tcc fptrdiff.c
  (Open)Watcom C++ : wcl -zq -s -fr fptrdiff.c -l=dos
  LSI-C86 3.30 trial : lcc fptrdiff.c
  gcc-ia16 with libi86 : ia16-elf-gcc -Wall -s -o fptrdiff.exe fptrdiff.c -li86

  to use huge pointers, add -DUSE_HUGE (except of LSI-C86 and gcc-ia16)
*/

#include <dos.h>
#include <stddef.h>     /* ptrdiff_t */
#include <stdio.h>
#include <stdlib.h>

#if defined(USE_HUGE) && !(defined(LSI_C) || (defined(__GNUC__) && defined(__ia16__)))
# if defined(__TURBOC__)
#  define FAR huge
# else
#  define FAR _huge
# endif
#else
# if defined(__GNUC__) && defined(__ia16__)
#  define FAR __far
#  define far __far
# else
#  define FAR far
# endif
#endif

long fp2la(const void FAR *fp)
{
    return 16UL * FP_SEG(fp) + FP_OFF(fp);
}

int cmp_fp(const void far *fp, const void far *fp1)
{
    int sign;
    char csign;
    const char FAR *cp = (const char FAR *)fp;
    const char FAR *cp1 = (const char FAR *)fp1;

    if (cp == cp1) { sign = 0; csign = '='; }
    else if (cp > cp1) { sign = 1; csign = '>'; }
    else if (cp < cp1) { sign = -1; csign = '<'; }
    else {
        fprintf(stderr, "abnormal situation:failed to compare far pointers.\n");
        csign = '!';
        sign = 0;
        /* abort(); */
    }

    printf("fp0=%04X:%04X %c fp1=%04X:%04X ptrdiff=%ld (linear_ptrdiff=%ld)\n", 
           FP_SEG(fp), FP_OFF(fp),
           csign,
           FP_SEG(fp1), FP_OFF(fp1),
           (long)(cp1 - cp),
           fp2la(cp1) - fp2la(cp)
          );

    return sign;
}

unsigned alloc_64k(void)
{
    union REGS r;
    r.h.ah = 0x48;
    r.x.bx = 0x1000; /* 4096 paragraphs */
    intdos(&r, &r);
    if (r.x.cflag) {
        fprintf(stderr, "FATAL: memory allocation failure\n");
        r.x.ax = 0;
    }
    return r.x.ax;
}

int main(void)
{
    unsigned s_base;
    char far *fp_base;
    char far *fp_1000;

    s_base = alloc_64k();
    if (!s_base) return 1;

    fp_base = MK_FP(s_base, 0);
    fp_1000 = MK_FP(s_base + 0x100, 0);

    cmp_fp(fp_base, fp_base);
    printf("\n");
    cmp_fp(fp_base, fp_1000);
    cmp_fp(fp_base, fp_1000 + 1);
    cmp_fp(fp_base + 1, fp_1000);
    cmp_fp(fp_base + 1, fp_1000 + 1);
    cmp_fp(fp_base + 0x1001, fp_1000 + 1);

    return 0;
}
