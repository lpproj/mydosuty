DSKRD and BPBDPB
================

DSKRD: PC-98もしくはIBM PCのディスクBIOS、あるいはDOSのabsolute disk read(int 25h, int 21h AX=7305h)を使ってディスクのセクタを読んでみるやつ。

BPBDPB: DOSの指定ドライブのDPBを表示。ついでにデバイスドライバに対して直接BUILD BPBを発行し、結果を表示してみるやつ。


用法(DSKRD)
-----------

DSKRD [-o filename] drive_number LBA_address_in_the_disk [read count]  
DSKRD [-o filename] drive_letter: Sector_address_in_the_partition [read_count]  

drive_number  ディスクBIOSに与えるドライブ番号。10進数、もしくは16進数（先頭に0xをつける）  
LBA_address 読み出したいセクタ番号。ディスク先頭（いわゆるMBR）が0。10進数、もしくは16進数（先頭に0xをつける）  
drive_letter DOSに与えるドライブ文字。ドライブ領域の先頭が0。A:～Z: （コロンは省略不可）  
Sector_address 読み出したいセクタ番号。 10進数、もしくは16進数（先頭に0xをつける）  
read_count 読み出すセクタ数。省略時は1  
filename セクタの内容を出力するファイル。無指定時はダンプリスト表示  

用法(BPBDPB)
------------

BPBDPB drive_letter [media_id]  

drive_letter DOSに与えるドライブ文字。A:～Z: （コロンは省略不可）  
media_id BUILD BPB時にドライバに与えるメディアディスクリプタとFAT IDの値。10進数、もしくは16進数（先頭に0xをつける）  

media_idはたいていの場合不要ですが、PC-98のMS-DOSではフロッピー向けのIDを与えないとエラーを返すことがあります。

ソースについて
--------------

OpenWatcom (1.9)、LSI-C86 3.30c試食版、Turbo C++ 1.01でビルド可能なことを確認しています。また、ビルドにはnasmも必要です。
ライセンスはUnlicenseです。

