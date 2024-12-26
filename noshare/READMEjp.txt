NOSHARE.COM
===========

MS-DOS 4.0の起動時に出る、

「警告！ ラージメディアが存在します. SHAREをロードしてください.」
（英語版は "WARNING! SHARE should be loaded for large media"）

というメッセージを表示しないようにする常駐プログラムです。
（厳密に言うと「常駐」はしません。役目を終えると自動的に常駐解除されます）

マイクロソフトから公開されているオープンソース版 MS-DOS 4.0（自前ビルド）および
エプソン版 MS-DOS 4.01にて動作確認を行っています。


用法
----

MS-DOS 4.0のCONFIG.SYSを編集し、INSTALL文でNOSHARE.COMを常駐させます。

    INSTALL=A:\NOSHARE.COM

CONFIG.SYSの処理が終了後、NOSHARE.COMは自動的に常駐解除されます。
他にINSTALL文で常駐させるプログラムがある場合、NOSHAREのINSTALL文は
それらよりも後に書いてください。


ソース
------

https://github.com/lpproj/mydosuty/tree/master/noshare

