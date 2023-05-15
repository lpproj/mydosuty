/*
    atapi_1b    PC-98 IDE BIOS test
                (int 1Bh AH=02h,DL=FFh: send ATAPI packet and receive data)
                reference: Undocumented 9801/9821 Vol.1 (ISBN 4-8443-4642-3)

    to build (with LSI C-86 3.30c trial):
        lcc atapi_1b.c -lintlib

    usage:
        IDE BIOS経由でCD-ROMドライブにATAPIコマンドを発行し、INQUIRYデータを
        取得する。

        atapi_1b [-f] DA/UA

          -f    IDE BIOS内のパラメータテーブルを一時的に変更し、
                ファンクションを受け付けるようにする。
                （通常のIDE CD-ROM環境ではおそらく必須）

          DA/UA CDドライブのDA/UAの数字。16進数指定のときは先頭に0xをつける。

    revision:
    20230514 lpproj initial
    20230515 lpproj scan BIOS to fetch the paramtable address
*/

/*

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

#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    ERROR_ANYWAY = -1,
    NOERROR = 0
};

int check_daua_for_atapi(int daua)
{
    /*
        ワークエリアを覗いて、DA/UAがIDE CD-ROMか調べる（調べたい）

        IDE BIOSにおけるハードディスクとCD-ROMドライブのDA/UAは微妙に面倒な
        ことになっており、スレーブにデバイスが接続されると、

        1. スレーブ接続されたCDはHDと誤認され、0000:055C〜D(DISK_EQUIP),
           0000:05BA(IDE_DISK_EQUIP), F8E8:0010の当該ビットがセットされる。
           （うかつにHD BIOSから読み出すとハングアップの可能性大）

        2. IDEBIOS上のDA/UAの番号順が、認識されたハードディスクを優先して
           若い番号に詰められることがある。たとえば内蔵IDEの接続状況が

             プライマリMaster HD
             プライマリSlave  なし
             セカンダリMaster CD-ROM
             セカンダリSlave  HD

           となっている場合、DA/UAは

             0x80    HD（プライマリMaster）
             0x81    HD（セカンダリSlave）
             0x82    CD-ROM（セカンダリMaster）

           となる。

        というわけでBIOS的に想定外の事態に陥りやすい。
        （IDE BIOS内のパラメータテーブルを真面目に確認したほうがいいかも）
    */
    unsigned da, ua;

    da = (daua & (-1 ^ 0x8f));
    ua = daua & 0x0f;
#if 0
    if (da == 0 && ua < 8) {    /* IDE HD EQUIP (IDEBIOS) */
        if (((1<<ua) & *(unsigned char far *)MK_FP(0, 0x05ba)) == 0)
#else
    if (da == 0 && ua < 4) {    /* SASI/ESDI/IDE HD EQUIP */
        if (((1<<ua) & *(unsigned char far *)MK_FP(0, 0x055d)) == 0)
#endif
            return NOERROR;
    }
    return ERROR_ANYWAY;
}


typedef struct {
    unsigned char cdb[12];
    void far *buffer;
} BIOSATAPI;

int send_atapi_cmd_and_receive(int daua, const unsigned char *cdb, unsigned cdb_length, void *receive_buffer, unsigned buffer_length)
{
    union REGS r;
    struct SREGS sr;
    BIOSATAPI pkt;

    /*
        int 1Bh AH=02h,DL=FFh   ATAPIデバイス(IDE CD-ROM)へのコマンド送出
        ES:BP=パケットへのポインタ
        BX   =データ読み出しバイト数（下記参照）
        用法に関してはundoc1（Undocumented 9801/9821 vol.1）の
        「ディスクBIOS（その他）」の記述による。

        以下undoc1未記載情報:
        o IDE BIOS内ワークエリアのパラメータテーブル(D800:2100から、1ドライブ
          あたり32バイト）のオフセット+10hのbit0が1の場合に処理が実行される。
          （bit0がセットされていない場合はNot Readyのエラーを返す）
          通常このフラグはセットされておらず、設定するBIOSファンクションも
          おそらく存在しない。ITFが事前にセットしていない場合、外部から
          ワークエリアを操作する必要がある。
        o 呼び出し後に結果を返す可能性のあるコマンドを発行した場合、実際に
          デバイスが返してくるデータサイズ分のバッファを「必ず」確保しておく
          必要がある。
          たとえばCDB内でAllocation Lengthを指定するコマンド（INQUIRYなど）を
          発行する場合、BXの指定値にかかわらずAllocation Lengthに指定した
          サイズの情報がバッファに書き込まれる。（Allocation Lengthより
          小さい値をBXに指定した場合も、データがBXのサイズにクリップされる
          ことはないようだ…）
        o Allocation Lengthに奇数を指定するとTime Outエラー（90h）が出る
          ことがある。

        **注意**
        ATAPIコマンドパケットの送出はHD BIOSのWRITE(AH=02h)に割り当てられて
        おり、IDE接続CD-ROMドライブ以外へのコマンド発行は致命的なデータ喪失を
        もたらす恐れがある。
        ある程度の安全策として、
        o DISK_EQUIPをチェックし、SASI/IDEのHDが存在する場合は呼び出さない
        ようにする
        o DHやCXにもHD領域外となる値を念のため設定しておく
    */
    if (cdb_length != 6 && cdb_length != 10 && cdb_length != 12)
        return ERROR_ANYWAY;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.cdb, cdb, cdb_length);
    pkt.buffer = receive_buffer;

    /* int 1bh, ah=02h, dl=0ffh: send atapi cmd and receive data */
    r.h.ah = 0x02;
    r.h.al = daua;
    r.h.dl = 0xff;
    r.x.bx = buffer_length;
    r.x.bp = FP_OFF(&pkt);  /* thanks to LSI C-86's libc */
    sr.es = FP_SEG(&pkt);
    /* not mandatory for this function: just to be safe... */
    r.h.dh = 0xff;
    r.x.cx = 0xffff;
    int86x(0x1b, &r, &r, &sr);
    return r.h.ah;
}

unsigned idebios_romseg(void)
{
    const unsigned mtype = *(unsigned char far *)MK_FP(0, 0x501) & 0x38;
    const unsigned char far *r;
    unsigned xrom_id_begin, xrom_id_end, xrom_seg;
    unsigned n;

    /* get XROM_ID */
    if (mtype & 0x08) {
        if ((mtype & 0x30) == 0) {
            /* PC-98XA */
            xrom_id_begin = 0x04e0; /* 0x04e6 (see undoc2 memsys.txt XROM_ID) */
            xrom_id_end = 0x04f3;
            xrom_seg = 0xe000U;
        }
        else {
            /* common Hi-res */
            xrom_id_begin = 0x04c0;
            xrom_id_end = 0x04cf;   /* 0x04c9 (see undoc2 memsys.txt XROM_ID) */
            xrom_seg = 0xe600U;
        }
    }
    else {
        if ((mtype & 0x30) == 0x30) return 0;   /* PC-98LT/HA */
        /* normal */
        xrom_id_begin = 0x04d0;
        xrom_id_end = 0x04df;
        xrom_seg = 0xd000U;
    }

    n = (xrom_id_end - 4) - xrom_id_begin + 1;
    r = MK_FP(0, xrom_id_begin);
    while(n--) {
        if (*r == 0xea && r[1] == 0xaa && r[2] == 0xaa && r[3] == 0xaa)
            return xrom_seg;
        ++r;
        xrom_seg += 0x0100;
    }

    return 0;
}

unsigned idebios_seg(void)
{
    unsigned ideseg = idebios_romseg();

    /*
      メモリマネージャがIDE BIOSをリロケートしている可能性があるので、実際の
      アドレスはDISK_XROMのほうから拾う
    */
    if (ideseg) {
        const unsigned char far *DISK_XROM = MK_FP(0, 0x04b0);
        unsigned da = DISK_XROM[0x8];
        if (da != 0 && da == DISK_XROM[0x0])
            ideseg = da << 8;
    }
    return ideseg;
}

void far *scan_paramtable_base(unsigned ideseg)
{
    /*
      IDE BIOS内のパラメータテーブルのアドレスを調べる。
      2100h固定じゃなくて、存在しない場合もあるらしいので、BIOS内の
      アドレス算出コードっぽいものの存在をスキャンして取り出すように
      してみた。（有効性がどの程度なのか未確認）
    */
    static unsigned param_off = 0xffffU;
    const unsigned char sig_code[] = {
        /* 0xbb, 0x00, 0x00, */ /* mov bx, offset IDEBIOSSEG: paramtable */
        0x33, 0xc9,             /* xor cx, cx */
        0x8a, 0x6e, 0x00,       /* mov ch, [bp] */
        0xb1, 0x05,             /* mov cl, 5 */
        0xd2, 0xe5,             /* shl ch, cl */
        0x32, 0xc9,             /* xor cl, cl */
        0x86, 0xe9,             /* xchg ch, cl */
        0x03, 0xd9              /* add bx, cx */
    };

    if (param_off == 0xffffU && ideseg) {
        const unsigned char far *m = MK_FP(ideseg, 0);
        const unsigned s_begin = 0x0003;
        const unsigned s_end = 0x2000 - sizeof(sig_code);
        unsigned s_cur;
        for(s_cur = s_begin; s_cur < s_end; ++s_cur) {
            unsigned n;
            for(n=0; n<sizeof(sig_code); ++n) {
                if (m[s_cur + n] != sig_code[n]) break;
            }
            if (n == sizeof(sig_code)) {
                param_off = *(unsigned short far *)(m + s_cur - 2);
            }
        }
        if (param_off == 0xffffU) param_off = 0;
    }
    if (param_off == 0 || ideseg == 0) return NULL;
    return MK_FP(ideseg, param_off);
}

void far *get_ide_paramtable(int daua)
{
    if ((daua & (-1 ^ 0x87)) == 0) {
        unsigned ideseg = idebios_seg();
        if (ideseg) {
            char far *p = scan_paramtable_base(ideseg);
            if (p) return (void far *)(p + ((daua&7)<<5));
        }
    }

    return NULL;
}

void dumpmem(const void far *mem, unsigned length)
{
#define DUMP_PER_LINE 16
    const unsigned char far *m = mem;
    unsigned n, nl;
    unsigned char s[DUMP_PER_LINE+1];

    for(n=0, nl=0; n<length;) {
        unsigned char c = *m;
        if (nl == 0) printf("%04X ", n);
        s[nl] = (c >= ' ') ? c : '.';
        printf(" %02X", c);
        ++m;
        ++n;
        ++nl;
        if (nl >= DUMP_PER_LINE) {
            s[nl] = '\0';
            printf("  %s\n", s);
            nl = 0;
        }
    }
    if (nl != 0) {
        s[nl] = '\0';
        while(nl++ < DUMP_PER_LINE)
            printf("   ");
        printf("  %s\n", s);
    }
}

int main(int argc, char **argv)
{
    unsigned char cdb_inquiry[] = {
        0x12,   /* +0  operation code */
        0x00,   /* +1  LUN (bit7~5) */
        0x00,   /* +2  page code */
        0x00,   /* +3  alloxation length (higher) */
        0x00,   /* +4  allocation length */
        0x00    /* +5  control field */
    };
    char buf[256];
    const unsigned inqsize = 36;
    int daua;
    unsigned ideseg;
    int optF = 0;
    unsigned char flag_bak;
    unsigned char far *ide_param;
    int rc;

    --argc;
    ++argv;
    if (argc >= 1) {
        if (strcmp("-f", *argv)==0) {
            optF = 1;
            --argc;
            ++argv;
        }
    }
    if (argc < 1 || strcmp("-?", *argv)==0) {
        printf("usage (exsample): atapi_1b [-f] 0x81\n");
        return 0;
    }
    daua = strtol(*argv, NULL, 0);
    if (check_daua_for_atapi(daua) != NOERROR) {
        fprintf(stderr, "not accepted DA/UA (0x%x)\n", daua);
        return 1;
    }
    ideseg = idebios_seg();
    printf("IDEBIOS ROM:%04X Actual:%04X\n", idebios_romseg(), ideseg);
    if (ideseg == 0) {
        fprintf(stderr, "not found IDE BIOS\n");
        return 1;
    }
    ide_param = get_ide_paramtable(daua);
    printf("IDEBIOS parameter table ");
    if (ide_param) {
        void far *p_base = get_ide_paramtable(0);
        printf("%04X:%04X", FP_SEG(ide_param), FP_OFF(ide_param));
        printf(" (base %04X:%04X)\n", FP_SEG(p_base), FP_OFF(p_base));
    }
    else
        printf("(not found)\n");

    memset(buf, 0xff, sizeof(buf));
    cdb_inquiry[4] = inqsize;
    if (optF) {
        if (ide_param) {
            flag_bak = ide_param[0x10];
            ide_param[0x10] |= 1;   /* enable IDEBIOS ah=02,dl=ffh and ah=16h */
        }
        else {
            fprintf(stderr, "warning: ignore option -F\n");
        }
    }
    rc = send_atapi_cmd_and_receive(daua, cdb_inquiry, sizeof(cdb_inquiry), buf, inqsize);
    if (optF && ide_param) {
        ide_param[0x10] = flag_bak;
    }
    printf("result=0x%x\n", rc);
    if (rc == 0 || rc == 0x90) dumpmem(buf, inqsize);

    return rc != 0;
}

