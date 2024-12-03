/*
    isvzstay - check the VZ editor is staying in the memory.

    to build with...
    (Open)Watcom: wcl -zq -s -fr -bt=dos isvzstay.c
    LSIC86: lcc isvzstay.c

    license: the Unlicense (http://unlicense.org/)
*/

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
unsigned peekw(const void far *m, unsigned off)
{
    return *(unsigned short far *)((char far *)(m) + off);
}

int isvzseg(unsigned memseg)
{
    const unsigned char far *mcb = MK_FP(memseg - 1, 0);
    const char far *m = MK_FP(memseg, 0);

    if (*mcb != 'M' && *mcb != 'Z') return 0;   /* memory arena corrupt */
    if (peekw(mcb, 1) != memseg) return 0;      /* not a PSP block */
    if (peekw(mcb, 3) < 0x12) return 0;     /* too small for VZ signature */
    if (*m != '\xcd' || m[1] != 0x20) return 0;   /* PSP quick check */

    /* check VZ signature */
    return m[0x102] == 'V' && m[0x103] == 'Z' && m[0x109] == '\0' && m[0x10a] == 0x1a;
}


int main(int argc, char **argv)
{
    int rc = 0;
    int opt_z = 0;
    union REGS r;
    struct SREGS sr;
    unsigned mcb_seg;
    const char far *m;
    char c;

    while(argc-- > 1) {
        char *s = *++argv;
        c = *s;
        if (c == '/' || c == '-') c = *++s;
        if (c == 'z' || c == 'Z') opt_z = 1;
    }

    /*
      MCBチェーンを辿ってVZの存在を確認する 
      DOS5以上の場合はUMBも調べるべきだが、サイズ的にUMBに直接ロードすることは 
      難しいと思われるので省略している 
    */
    r.h.ah = 0x52;
    intdosx(&r, &r, &sr);
    mcb_seg = *(unsigned short far *)MK_FP(sr.es, r.x.bx - 2);
    while(1) {
        m = MK_FP(mcb_seg, 0);
        c = *m;
        if (c == 'M' || c == 'Z') {
            rc = isvzseg(mcb_seg + 1);
            if (rc > 0 || c == 'Z') break;
            mcb_seg += peekw(m, 3) + 1;
        }
        else {
            fprintf(stderr, "ERROR: corrupt MCB\n");
            return 0;
        }
    }

    if (rc > 0) {
        int n;
        m = MK_FP(mcb_seg + 1, 0);
        printf("VZ signature : \"");
        for(n = 0x102; n <= 0x10a; ++n) {
            c = m[n];
            if ((unsigned char)c < 0x20) break;
            printf("%c", c);
        }
        printf("\"\n");
        n = m[0x122];
        printf("Is VZ a TSR? : %s", n ? "yes" : "no");
        if (n != 0) printf(" (tsrflag=%d)", n);
        printf("\n");
        if (opt_z) rc = n;
    }
    else {
        printf("VZ is not loaded.\n");
    }

    return rc;
}
