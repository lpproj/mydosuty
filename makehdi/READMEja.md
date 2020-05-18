MAKEHDI
=======

主にPC-98系エミュレータ（Anex86、T98Next、NP2）で使われるハードディスクイメージファイルを作るための、場当たりに作成されたコマンドラインツールです。

CHS情報がマニュアル指定できる点と（NTFSディスク上であれば）イメージ作成がほぼ一瞬である点が、強いて言うならやや便利かもしれません。


書法
----

ディスクイメージの作成方法は主に３通りあります。

1. ディスク容量のみの指定
> makehdi [-f *disk_type*] [-b *bytes_per_sector*] --size *disk_size* IMAGEFILE
2. ヘッド数、トラック当たりのセクタ数、ディスク容量を指定。シリンダ数は自動算出。
> makehdi [-f *disk_type*] [-b *bytes_per_sector*] -h *heads* -s *sectors* --size *disk_size* IMAGEFILE
3. シリンダ数、ヘッド数、トラック当たりのセクタ数を指定。ディスク容量は自動算出。 
> makehdi [-f *disk_type*] [-b *bytes_per_sector*] -c *cylinders* -h *heads* -s *sectors* IMAGEFILE


説明
----

 -f *disk_type*

出力するディスクイメージの種類。     
-fオプションが指定されておらず、IMAGEFILEで指定されたファイル名の最後尾がが以下のデフォルト拡張子と一致する場合、対応するディスクイメージの種類が自動的に指定されます。

| *disk_type* | description        | デフォルト拡張子 |
| ----------- | ------------------ | ---------------- |
| hdi         | HDI形式（Anex86）  | .hdi             |
| nhd         | NHD形式（T98Next） | .nhd             |
| v98         | Virtual98形式      | .hdd             |
| raw         | ベタイメージ       |                  |


 -b *bytes_per_sector*

１セクタ当たりのバイト数。256, 512, 1024, 2048 が指定可能。     
オプション無指定時は 512。


 --size *disk_size*

ディスクイメージの（エミュレータ側から見た）バイト単位の容量。
数字の後に文字を指定することでメガ（ギガ）バイト単位の指定も可能です。

| 文字 | 単位                |
| ---- | ------------------- |
| K    | 1000                |
| Ki   | 1024                |
| M    | 1000000 (1000^2)    |
| Mi   | 1048576 (1024^2)    |
| G    | 1000000000 (1000^3) |
| Gi   | 1073741824 (1024^3) |‬

※実際に作成されるイメージ容量は、１シリンダ分の容量単位で切り上げられます。     
※HDI形式のイメージ容量は4Gi(4096Mi)バイト未満となります     


 -c *cylinders*

ディスクイメージのシリンダ数を指定します（1～65535）。--sizeオプションと同時に指定できません。


 -h *heads*

ディスクイメージのヘッド数を指定します（1～255）。-sオプションの同時指定が必要です。


 -s *sectors*

ディスクイメージの１トラック当たりのセクタ数を指定します（1～255）。-hオプションの同時指定が必要です。


ビルド（コンパイル）
--------------------

C99準拠のCコンパイラが必要です（加えて、getopt_longのライブラリサポートが必須）。
msys2のmingw-w64-*-toolchainとRaspbian上のgccでビルドを確認しています。

```
gcc -s -Wall -O2 -o makehdi makehdi.c
```

OpenWatcom-v2でビルドを行うときは、getopt_long相当のライブラリを別途用意する必要があります。
[ya_getopt](https://github.com/kubo/ya_getopt)を使う場合はya_getopt.cとya_getopt.hをコピーして、コンパイル時にUSE_YA_GETOPTを定義します。

```
wcl386 -zq -s -Fr -za99 -DUSE_YA_GETOPT makehdi.c
```


