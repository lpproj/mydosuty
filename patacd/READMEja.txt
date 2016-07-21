PATACD: Generic ATAPI CD-ROM driver for DOS (IBM PC and NEC PC-98)

NEC PC-9821 シリーズもしくは PC/AT 互換機用の IDE (ATAPI) CD-ROM
ドライバです。バージョン 3.1 以上の MS-DOS、Windows 95/98/Me の
シングル MS-DOS モードで利用可能です。
（CD-ROM 内のファイルアクセスには、MSCDEX もしくは互換ドライバが
別途必要です）

なお PATACD.SYS 単体で PC-98 と PC/AT 兼用となっています。


インストール
------------

他の DOS 用 CD-ROM ドライバと同様に、DOS の config.sys に device 文で
ドライバのフルパス名を記述し、デバイスドライバとして登録します。

DEVICE=[<ドライブ>:][<パス>]PATACD.SYS [/D:デバイス名]

デバイス名は MSCDEX に渡すドライバのデバイス名を英数字８文字以内で
設定します。
/D オプションを指定しない場合、

  NEC PC-98 シリーズでは /D:CD_101
  IBM PC/AT 互換機では /D:MSCD001

が指定したものとみなされます。


実装状況
--------

・パラレル ATA 接続専用。SATA 接続は未対応です。
・それほど動作は安定していません。
  エミュレータ・仮想マシン上ではそれなりに動作すると思います（思いたい）。
・CD-ROM ドライバの機能をフルサポートしているわけではありません
・オーディオチャンネル（音量）制御はデフォルトで無効となっています。
  必要な場合はソース中の %define SUPPORT_AUDIO_CHANNEL の先頭にある
  コメント（;）を消して再ビルドしてください。
  （240バイトほどサイズが増えます）
・CD 再生は CD-ROM ドライブのアナログ音声出力から音が出力されます。そもそも
  ドライブがアナログ再生コマンドをサポートしていなかったり、アナログ出力が
  どこにも接続されていない場合、CD 再生音は出ません。


ソース
------

  https://github.com/lpproj/mydosuty/tree/master/patacd


