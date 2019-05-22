/*

    pc98io5f: get actual time of "out 5Fh, al".

target:
    NEC PC-9821
    MS-DOS

to build:
    wcl -zq -s pc98io5f.c       (with OpenWatcom)
    tcc pc98io5f.c              (with Turbo C++ 1.01)
    lcc pc98io5f.c              (with LSI-C86 3.30 eval)

license: (so-called) The Public Domain

description (in Japanese, encoded with CP932):
    out 5Fh, al の実行時間をおおまかに測定する。
    メモリ上に out 5Fh, al の命令を1000個並べて実行し、処理前後の
    タイムスタンプの経過を調べる。

    手持ちの PC-9821Xc13/S5 で測定したところ、タイムスタンプ単位で
    535tickとなり、１命令当たりの処理速度は約1744nsとなった。
    （1tick=3260ns換算）

*/

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char code_header[] = {
    0xfa,           /* cli */
    0xe5, 0x5c,     /* in ax, 5ch ; low word of ARTIC */
    0x89, 0xc2      /* mov dx, ax */
};

static const char code_body[] = {
    0xe6, 0x5f      /* out 5fh, al */
};

static const char code_footer[] = {
    0xe5, 0x5c,     /* in ax, 5ch ; low word of ARTIC */
    0x2b, 0xc2,     /* sub ax, dx */
    0xfb,           /* sti */
    0xcb            /* retf */
};


static unsigned (far *myfarfunc)(void);

static
int
has_artic(void)
{
    return (*(unsigned char far *)MK_FP(0,0x45b) & 0x84) || (*(unsigned char far *)MK_FP(0,0x458)&0x80);
}


int main(void)
{
    unsigned codecnt = 1000;
    unsigned artic_ns = 3260;   /* ARTIC time-stamper: 3.26us per tick */
    unsigned code_total_size;
    unsigned t;
    unsigned long tns;
    char *mycode;
    char *m;
    unsigned n;

    if (!has_artic()) {
        fprintf(stderr, "This machine does not have ARTIC time stamper.\n");
        exit(1);
    }
    code_total_size = sizeof(code_header) + codecnt * sizeof(code_body) + sizeof(code_footer);
    mycode = malloc(code_total_size);
    if (mycode == NULL) {
        fprintf(stderr, "fatal: memory allocation failure.\n");
        exit(-1);
    }
    m = mycode;
    memcpy(m, code_header, sizeof(code_header));
    m += sizeof(code_header);
    for(n=0; n<codecnt; ++n) {
        memcpy(m, code_body, sizeof(code_body));
        m += sizeof(code_body);
    }
    memcpy(m, code_footer, sizeof(code_footer));
    myfarfunc = (void far *)mycode;

    t = myfarfunc();
    tns = (unsigned long)t * artic_ns;

    printf("\"out 5Fh,al\" x %u : %uticks in ARTIC\n", codecnt, t);
    printf("%uns * %u ... approx %luns (%luus)\n", artic_ns, t, tns, tns / 1000U);
    printf("per a \"out 5Fh,al\" ... %luns\n", tns / codecnt);

    return 0;
}

