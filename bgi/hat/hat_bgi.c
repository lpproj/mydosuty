/*
    draw the hat with the Borland Graphics Interface (BGI)

    reference:
    various sources of "draw the hat" : https://togetter.com/li/2390318

    To build with Borland C++ (3.0 to 5.0):
      bcc -1- -O2 -G hat_bgi.c graphics.lib

    When you use Japanese version of Turbo/Boland C++, you can build
    Japanese message version (encoded with CP932/Shift_JIS):
      bcc -1- -O2 -G -DJAPAN=1 hat_bgi.c graphics.lib

    Usage:
      HAT_BGI [-v] [-Ddriver_name,mode_number] [-D@driver_number,mode_number]
        -v  be verbose
        -D  specify BGI driver and mode number:
            to use PC98.BGI for NEC PC-98 640x400 16colors: -DPC98,1
            to use EGA 640x350 16colors: -D@3,1
*/

#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphics.h>

#include <math.h>
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

/* draw the hat */

void draw_hat(int screen_width, int screen_height, int screen_colors)
{
#if TEST_COLOR
    double zmx = 0, zmi = 0;
#endif
    const double DR = M_PI / 180.0;
    const int region_x = 180;
    const int region_y = 180;
    const int org_x = screen_width / 2;
    const int org_y = (screen_height / 2) - (screen_height / 10);
    const double scale_x = (double)screen_width / (double)region_x;
    const double scale_y = 1.8 * (double)screen_height / (double)region_y;
    const int step_x = (region_x * 4 / screen_width) + 2;
    const int step_y = (region_y * 2 / screen_height) + 2;
    const double colorband = (double)(screen_colors - 1) / 194.0;  /* empirical */
    int i, x, y;
    int *d;

    d = malloc(screen_width * sizeof(int));
    for (i=0; i<screen_width; ++i)
        d[i] = screen_height;

    for (y = -region_y; y < region_y; y += step_y) {
        double dy = (double)y;
        for (x = -region_x; x < region_x; x += step_x) {
            double dx = (double)x;
            double r = DR * sqrt(dx*dx + dy*dy);
            double z = 100.0 * cos(r) - 30.0 * cos(r*3);
            int sx = org_x + (int)(scale_x * (dx/3 - dy/6));
            int sy = org_y - (int)(scale_y * (dy/6 + z/4));
#if TEST_COLOR
            if (zmx < z) zmx = z;
            if (zmi > z) zmi = z;
#endif
            if (sx >= 0 && sx < screen_width && sy < d[sx]) {
                int zz = (int)(colorband * (z+100)) + 1;
                putpixel(sx, sy, zz);
                d[sx] = sy;
            }
        }
    }
#if TEST_COLOR
    printf("zmx = %f, zmi = %f\n", zmx, zmi);
#endif

    free(d);
}

/* around BGI */

int bgi_driver = 0;
int bgi_gmode = -1;
int bgi_verbose = 0;

#define bv_printf   if (bgi_verbose <= 0) ; else printf

static char far *bgi_fstrncpy(char far *d, const char far *s, unsigned n)
{
    register unsigned i;
    for (i=0; i<n; ++i) {
        register char c = *((char far *)s + i);
        *((char far *)d + i) = c;
        if (c == '\0') break;
    }
    return d;
}

static int huge bgi_detect_mode_dummy(void)
{
    return bgi_gmode;
}

int init_bgi(const char *driver, int gdriver, int gmode)
{
    int gerror;

    if (driver && *driver && gdriver == 0) {
        bgi_gmode = gmode;
        bv_printf ("loading \"%s\" mode %d...", driver, gmode);
        gdriver = installuserdriver((char far *)driver, bgi_detect_mode_dummy);
        gerror = graphresult();
        bv_printf ("gdriver=%d gerror=%d\n", gdriver, gerror);
    }
    initgraph(&gdriver, &gmode, "");
    gerror = graphresult();
    if (gerror == grOk) {
        bgi_driver = gdriver;
        bgi_gmode = gmode;
        if (bgi_verbose > 0) {
            char dn[65];
            bgi_fstrncpy(dn, getdrivername(), sizeof(dn));
            printf("driver=%d \"%s\"", gdriver, dn);
            printf(", %dx%d %dcolor\n", getmaxx() + 1, getmaxy() + 1, getmaxcolor() + 1);
            bgi_fstrncpy(dn, getmodename(gmode), sizeof(dn));
            printf("mode=%d \"%s\"", gmode, dn);
            printf("\n");
        }
    }
    else {
        bv_printf ("error at initgraph (gerror=%d)\n", gerror);
    }
    return gerror;
}

int exit_bgi(void)
{
    if (bgi_driver != 0) {
        closegraph();
        bgi_driver = 0;
        return grOk;
    }
    return -1;
}

/* getopt & main */

int optHelp;
int optV;
int optD;
char *optDdrv = NULL;
int optDnum = 0;
int optDmode;

static char * getD(const char *s, int *n, int *m)
{
    char *s2 = strchr(s, ',');
    if (s2) {
        if (m) *m = (int)strtol(s2+1, NULL, 0);
        *s2 = '\0';
    }
    if (n) {
        if (*s == '@') *n = (int)strtol(s+1, NULL, 0);
    }
    return (char *)s;
}

void mygetopt(int argc, char **argv)
{
    while(argc-- > 1) {
        char *s = *++argv;
        if (*s == '/' || *s == '-') {
            char c = *++s;
            if (c == '?') optHelp = 1;
            else if (c == 'v' || c == 'V') ++optV;
            else if (c == 'd' || c == 'D') {
                c = *++s;
                if (c == ':' || c == '=') ++s;
                if (*s) optDdrv = getD(s, &optDnum, &optDmode);
                else if (argv[1]) {
                    --argc;
                    ++argv;
                    optDdrv = getD(*argv, &optDnum, &optDmode);
                }
            }
        }
    }
}

void
usage(void)
{
    const char msg[] = 
#ifdef JAPAN
    "用法: HAT_BGI [-v] [-Dドライバ名,モード番号]  [-D@ドライバ内部番号,モード番号]\n"
    "        -v  詳細モード\n"
    "        -D  使用するBGIドライバとモード番号を手動設定\n"
    "            例: PC-98用16色ドライバを使う場合は  -DPC98,1\n"
    "                EGA 640x350 16色ドライバの場合は -D@3,1\n"
#else
    "Usage: HAT_BGI [-v] [-Ddriver,mode_number] [-D@driver_number,mode_number]\n"
    "        -v  be verbose\n"
    "        -D  specify BGI driver and mode number\n"
    "            to use PC98.BGI for NEC PC-98 640x400 16colors: -DPC98,1\n"
    "            to use EGA 640x350 16colors: -D@3,1\n"
#endif
        ;
    printf("%s", msg);
}

int dos_getch(void)
{
    union REGS r;
    r.h.ah = 8;
    intdos(&r, &r);
    return r.h.al;
}

int main(int argc, char **argv)
{
    int rc;

    mygetopt(argc, argv);
    if (optHelp) {
        usage();
        return 0;
    }
    bgi_verbose = optV;

    rc = init_bgi(optDdrv, optDnum, optDmode);
    if (rc == grOk) {
        draw_hat(getmaxx() + 1, getmaxy() + 1, getmaxcolor() + 1);
        dos_getch();
        exit_bgi();
    }

    return rc;
}

/*

BGI memo
========

外部ドライバを使うことにより多様なグラフィックデバイスに対応、と言うことになっているが、正直過度な期待はしないほうがいい…

PC-98とIBM PCの両機種に同時対応させるためにはBorland C++（3.0以上）の日本語版が必要になる。英語版は当然PC-98未対応で、Borland C++ 2.0までのTurbo/Borland C++日本語版はIBM PCに対応していない。

installuserdriverでBGIドライバを読み込む場合、コンパイラ側のライブラリ(graphics.lib)に登録済みのドライバはinitgraphの初期化に失敗するものがある。
手持ちの中ではTurbo C++ 1.01英語版とBorland C++ 2.0Jではダメで、Borland C++ 3.1Jと5.0Jは動作した。
installuserdriverを使わず、ドライバ番号を指定してinitgraphに渡せば所定のドライバを読み込んで初期化は可能。

英語版Turbo C++ 1.01でコンパイルしたhat_bgiに-DPC98,1を指定してPC-98用のドライバを読み込んだ場合、システムは再起動した。
日本語版Borland C++ 2.0でコンパイルしたhat_bgiに-DCGA,0などを指定してIBM PC用のドライバを読み込んだ場合、システムはハングアップした。
ライブラリの機種依存性の問題だと思う。

simtelnetに上がっていたET3000用BGIドライバ(et3bgi11.zip)のドキュメントによると、Turbo C 2.0やTurbo Pascal 5.xでinstalluserdriverを使った場合、その後のinitgraphに与えるドライバ値を+5しておく必要があるらしい（もしくはDETECTを設定して自動検出させる）。
それ以前のTurbo C 1.5やTurbo Pascal 5.0でもそうなんだろうか…？
（なお、Turbo C++ 1.01やTurbo Pascal 6.0ではこの措置は不要になっている）

2024-07-12 lpproj

*/

