/*
truer.c : a TRUENAME function for remote drives

to build with (Open)Watcom:
    wcl -zq -s -fr -DTEST truer.c -l=dos

history:
2025-04-18 lpproj   it seems to be working


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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  DOS function call 60h : "TRUENAME" - CANONICALIZE FILENAME OR PATH

  細かい話: DOSファンクションコール60hはCOMMAND.COMのTRUENAMEコマンドと異なり、
  変換元パス名にドライブ文字と : だけを指定するとエラーを返す。
  たとえば "C:" だとエラーになるので、TRUENAMEコマンドと同じ結果を得たい
  場合は "C:." を指定する必要がある
*/
static int my_dos_truename(const char far *src, char far *dest)
{
    union REGS r;
    struct SREGS sr;

    r.h.ah = 0x60;
    r.x.si = FP_OFF(src);
    sr.ds = FP_SEG(src);
    r.x.di = FP_OFF(dest);
    sr.es = FP_SEG(dest);
    intdosx(&r, &r, &sr);
    return r.x.cflag ? -(int)(r.x.ax) : 0;
}

int truer_name(const char *src, char *dest)
{
    int rc = 0;
    char drive_root[4];
    char true_root[128];
    char tmp_dest[128];
    size_t tr_len;
    size_t td_len;
    int dirsep_in_root;
    int do_fallback;

    /* UNCはそのままtruenameに渡す */
    if ((*src == '\\' || *src == '/') && (src[1] == '\\' && src[1] == '/')) {
        return my_dos_truename(src, dest);
    }

    /*
      リモートドライブがTRUENAMEで返すパス名のプレフィクス部を
      （公開ファンクションのみを使って）得てみる。
      単純にルートディレクトリのTRUENAMEの戻り値を使うだけだが、DOS標準の
      FATドライブと異なり、リモートドライブではルートディレクトリを示す \ が
      つかないことが多い。

      たとえば C:ドライブがMS-DOS標準のFATで、Q:ドライブがCD-ROM(MSCDEX)の
      とき、TRUENAMEコマンドの戻り値は以下のようになる。

      TRUENAME C:\    -> C:\
      TRUENAME C:\foo -> C:\FOO
      TRUENAME Q:\    -> \\Q.\A.
      TRUENAME Q:\foo -> \\Q.\A.\FOO
    */
    drive_root[0] = true_root[0] = '\0';
    do_fallback = 0;
    dirsep_in_root = 1;
    if (isalpha(*src) && src[1] == ':') {
        /* "A:"〜"Z:"のドライブが指定されている */
        *drive_root = toupper(*src);
    }
    else if (*src == '\0' || src[1] != ':') {
        /* ドライブ指定なし（現行ドライブが指定されているとみなす）*/
        union REGS r;
        r.h.ah = 0x17;      /* get current drive */
        intdos(&r, &r);
        *drive_root = 'A' + r.h.al;
    }
    else {
        /* 普通ではないドライブ文字が指定されている（"@:"など） */
        do_fallback = 1;
    }

    if (*drive_root) {
        /* ルートパスのTRUENAMEを得る */
        strcpy(drive_root + 1, ":\\");
        rc = my_dos_truename(drive_root, true_root);
        if (rc >= 0 && *true_root) {
            /* ルートパス名の終端に \ があるか調べる */
            tr_len = strlen(true_root);
            if (true_root[tr_len-1] != '\\' && true_root[tr_len-1] != '/') {
                dirsep_in_root = 0; /* なかった */
            }
        }
        else {
            do_fallback = 1;
        }
    }

    if (do_fallback) {
        return my_dos_truename(src, dest);
    }

    /* truename の本体は、本来ここだけ… */
    rc = my_dos_truename(src, tmp_dest);
    if (rc < 0) return rc;

    /*
      truename後のパス名の先頭部分がプレフィクス部と一致するなら、元の
      ドライブ文字に戻してコピーする
    */
    td_len = strlen(tmp_dest);
    if (tr_len <= td_len && memcmp(tmp_dest, true_root, tr_len) == 0) {
        char *actual_pathname = tmp_dest + tr_len - dirsep_in_root;
        strcpy(dest, drive_root);
        if (*actual_pathname != '\0') { /* if not root-dir */
            strcpy(dest + 2, actual_pathname);
        }
    }
    else {
        strcpy(dest, tmp_dest);
    }
    return rc;
}


#ifdef TEST
int main(int argc, char **argv)
{
    int rc = 0;

    while(argc-- > 1) {
        char s[128];
        ++argv;
        printf("param      : %s\n", *argv);
        rc = my_dos_truename(*argv, s);
        printf("truename   : %s\n", rc >= 0 ? s : "(failed)");
        rc = truer_name(*argv, s);
        printf("truer-name : %s\n", rc >= 0 ? s : "(failed)");
    }
    return rc;
}
#endif
