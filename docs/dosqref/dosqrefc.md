DOSファンクションクイックリファレンス(Interrupt List用)
=========================================================

(2018-06-17)

[Ralf Brown's Interrput List](http://www.cs.cmu.edu/~ralf/files.html)の中にあるDOSファンクションコール関係項目の一覧（全部とは言ってない）に、オンライン版へのリンクを加えたものです。オンライン版は複数存在しますが、ここで表示される広告が（比較的）穏便な[delorie.comのほう](http://www.delorie.com/djgpp/doc/rbinter/)にリンクを張っています。


int 21h DOSファンクションコール一覧
--------------------------------

- int 21h AH=00h プログラム終了(COMプログラム用) rbint:[D-2100](http://www.delorie.com/djgpp/doc/rbinter/id/62/25.html)
- int 21h AH=01h 標準入力から１文字入力（エコー付） rbint:[D-2101](http://www.delorie.com/djgpp/doc/rbinter/id/63/25.html)
- int 21h AH=02h 標準出力に１文字出力 rbint:[D-2102](http://www.delorie.com/djgpp/doc/rbinter/id/65/25.html)
- int 21h AH=03h 標準補助入力(AUX)から１文字入力 rbint:[D-2103](http://www.delorie.com/djgpp/doc/rbinter/id/66/25.html)
- int 21h AH=04h 標準補助出力(AUX)に１文字出力 rbint:[D-2104](http://www.delorie.com/djgpp/doc/rbinter/id/67/25.html)
- int 21h AH=05h プリンタ(PRN)に１文字出力 rbint:[D-2105](http://www.delorie.com/djgpp/doc/rbinter/id/68/25.html)
- int 21h AH=06h 直接コンソール出力 rbint:[D-2106](http://www.delorie.com/djgpp/doc/rbinter/id/69/25.html)
- int 21h AH=06h DL=FFh 直接コンソール入力 rbint:[D-2106--DLFF](http://www.delorie.com/djgpp/doc/rbinter/id/70/25.html)
- int 21h AH=07h 直接文字入力（エコーなし） rbint:[D-2107](http://www.delorie.com/djgpp/doc/rbinter/id/71/25.html)
- int 21h AH=08h 文字入力（エコーなし） rbint:[D-2108](http://www.delorie.com/djgpp/doc/rbinter/id/72/25.html)
- int 21h AH=09h 文字列出力 rbint:[D-2109](http://www.delorie.com/djgpp/doc/rbinter/id/73/25.html)
- int 21h AH=0Ah 行入力 rbint:[D-210A](http://www.delorie.com/djgpp/doc/rbinter/id/74/25.html)
- int 21h AH=0Bh 標準入力ステータス取得 rbint:[D-210B](http://www.delorie.com/djgpp/doc/rbinter/id/76/25.html)
- int 21h AH=0Ch 標準入力バッファ消去（後に入力） rbint:[D-210C](http://www.delorie.com/djgpp/doc/rbinter/id/78/25.html)
- int 21h AH=0Dh ディスクリセット rbint:[D-210D](http://www.delorie.com/djgpp/doc/rbinter/id/79/25.html)
- int 21h AH=0Eh 現行ドライブ設定 rbint:[D-210E](http://www.delorie.com/djgpp/doc/rbinter/id/81/25.html)
- int 21h AH=0Fh FCBによるファイルオープン rbint:[D-210F](http://www.delorie.com/djgpp/doc/rbinter/id/85/25.html)
- int 21h AH=10h FCBによるファイルクローズ rbint:[D-2110](http://www.delorie.com/djgpp/doc/rbinter/id/86/25.html)
- int 21h AH=11h FCBによるファイル検索(findfirst) rbint:[D-2111](http://www.delorie.com/djgpp/doc/rbinter/id/87/25.html)
- int 21h AH=12h FCBによるファイル検索(findnext) rbint:[D-2112](http://www.delorie.com/djgpp/doc/rbinter/id/88/25.html)
- int 21h AH=13h FCBによるファイル消去 rbint:[D-2113](http://www.delorie.com/djgpp/doc/rbinter/id/89/25.html)
- int 21h AH=14h FCBによるファイル読み込み(シーケンシャル) rbint:[D-2114](http://www.delorie.com/djgpp/doc/rbinter/id/90/25.html)
- int 21h AH=17h FCBによるファイルリネーム rbint:[D-2117](http://www.delorie.com/djgpp/doc/rbinter/id/93/25.html)
- int 21h AH=15h FCBによるファイル書き込み(シーケンシャル) rbint:[D-2115](http://www.delorie.com/djgpp/doc/rbinter/id/91/25.html)
- int 21h AH=16h FCBによるファイル作成 rbint:[D-2116](http://www.delorie.com/djgpp/doc/rbinter/id/92/25.html)
- int 21h AH=19h 現行ドライブ取得 rbint:[D-2119](http://www.delorie.com/djgpp/doc/rbinter/id/99/25.html)
- int 21h AH=1Ah DTA(ディスク転送エリア)設定 rbint:[D-211A](http://www.delorie.com/djgpp/doc/rbinter/id/00/26.html)
- int 21h AH=1Bh 現行ドライブのFAT情報を得る rbint:[D-211B](http://www.delorie.com/djgpp/doc/rbinter/id/01/26.html)
- int 21h AH=1Ch 指定ドライブのFAT情報を得る rbint:[D-211C](http://www.delorie.com/djgpp/doc/rbinter/id/02/26.html)
- int 21H AH=1Fh 現行ドライブのDPBを得る rbint:[D-211F](http://www.delorie.com/djgpp/doc/rbinter/id/05/26.html)
- int 21h AH=21h FCBによるファイル読み込み(ランダムレコード、1ブロック) rbint:[D-2121](http://www.delorie.com/djgpp/doc/rbinter/id/09/26.html)
- int 21h AH=22h FCBによるファイル書き込み(ランダムレコード、1ブロック) rbint:[D-2122](http://www.delorie.com/djgpp/doc/rbinter/id/10/26.html)
- int 21h AH=23h FCBによるファイルサイズ(レコード数)取得 rbint:[D-2123](http://www.delorie.com/djgpp/doc/rbinter/id/11/26.html)
- int 21h AH=24h FCBによるファイルレコード位置設定(lseek?) rbint:[D-2124](http://www.delorie.com/djgpp/doc/rbinter/id/12/26.html)
- int 21h AH=25h 割り込みベクタ設定 rbint:[D-2125](http://www.delorie.com/djgpp/doc/rbinter/id/13/26.html)
- int 21h AH=26h PSP作成(COMプログラム用) rbint:[D-2126](http://www.delorie.com/djgpp/doc/rbinter/id/93/26.html)
- int 21h AH=27h FCBによるファイル読み込み(ランダムレコード、複数ブロック) rbint:[D-2127](http://www.delorie.com/djgpp/doc/rbinter/id/94/26.html)
- int 21h AH=28h FCBによるファイル書き込み(ランダムレコード、複数ブロック) rbint:[D-2128](http://www.delorie.com/djgpp/doc/rbinter/id/95/26.html)
- int 21h AH=29h ファイル名をFCB形式に変換 rbint:[D-2129](http://www.delorie.com/djgpp/doc/rbinter/id/96/26.html)
- int 21h AH=2Ah 現在の日付を取得 rbint:[D-212A](http://www.delorie.com/djgpp/doc/rbinter/id/97/26.html)
- int 21h AH=2Bh 現在の日付を設定 rbint:[D-212B](http://www.delorie.com/djgpp/doc/rbinter/id/98/26.html)
- int 21h AH=2Ch 現在の時刻を取得 rbint:[D-212C](http://www.delorie.com/djgpp/doc/rbinter/id/14/27.html)
- int 21h AH=2Dh 現在の時刻を設定 rbint:[D-212D](http://www.delorie.com/djgpp/doc/rbinter/id/18/27.html)
- int 21h AH=2Eh (DL=00h) ベリファイフラグ設定 rbint:[D-212E--DL00](http://www.delorie.com/djgpp/doc/rbinter/id/20/27.html)
- int 21h AH=2Fh DTA(ディスク転送エリア)アドレス取得 rbint:[D-212F](http://www.delorie.com/djgpp/doc/rbinter/id/21/27.html)
- int 21h AH=30h DOSバージョン取得 rbint:[D-2130](http://www.delorie.com/djgpp/doc/rbinter/id/22/27.html)
- int 21h AH=31h プログラムの常駐終了 (TSR:Terminate and Stay Resident) rbint:[D-2131](http://www.delorie.com/djgpp/doc/rbinter/id/34/27.html)
- int 21H AH=32h 指定ドライブのDPBを得る rbint:[D-2132](http://www.delorie.com/djgpp/doc/rbinter/id/35/27.html)
- int 21h AH=33h (AL=00h/01h) BREAKチェックの状態取得／設定 rbint:[D-2133](http://www.delorie.com/djgpp/doc/rbinter/id/36/27.html)
- int 21h AX=3305h (DOS 4+) 起動ドライブ取得 rbint:[D-213305](http://www.delorie.com/djgpp/doc/rbinter/id/40/27.html)
- int 21h AX=3306h (DOS 5+) 真のバージョン取得 rbint:[D-213306](http://www.delorie.com/djgpp/doc/rbinter/id/41/27.html)
- int 21h AH=34h InDOSフラグのアドレス取得 rbint:[D-2134](http://www.delorie.com/djgpp/doc/rbinter/id/50/27.html)
- int 21h AH=35h 割り込みベクタアドレス取得 rbint:[D-2135](http://www.delorie.com/djgpp/doc/rbinter/id/51/27.html)
- int 21h AH=36h 指定ドライブの容量取得 rbint:[D-2136](http://www.delorie.com/djgpp/doc/rbinter/id/62/27.html)
- int 21h AX=3700h オプションスイッチ用の文字(SWITCHAR)取得 rbint:[D-213700](http://www.delorie.com/djgpp/doc/rbinter/id/63/27.html)
- int 21h AX=3701h オプションスイッチ用の文字(SWITCHAR)設定 rbint:[D-213701](http://www.delorie.com/djgpp/doc/rbinter/id/64/27.html)
- int 21h AH=38h 国別情報の取得／設定 rbint:[D-2138](http://www.delorie.com/djgpp/doc/rbinter/id/84/27.html)
- int 21h AH=38h DX=FFFFh 国番号の設定 rbint:[D-2138--DXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/85/27.html)
- int 21h AH=39h ディレクトリ作成(mkdir) rbint:[D-2139](http://www.delorie.com/djgpp/doc/rbinter/id/86/27.html)
- int 21h AH=3Ah ディレクトリ削除(rmdir) rbint:[D-213A](http://www.delorie.com/djgpp/doc/rbinter/id/87/27.html)
- int 21h AH=3Bh 現行ディレクトリ設定(chdir) rbint:[D-213B](http://www.delorie.com/djgpp/doc/rbinter/id/88/27.html)
- int 21h AH=3Ch ファイル作成(creat) rbint:[D-213C](http://www.delorie.com/djgpp/doc/rbinter/id/89/27.html)
- int 21h AH=3Dh ファイルを開く(open) rbint:[D-213D](http://www.delorie.com/djgpp/doc/rbinter/id/90/27.html)
- int 21h AH=3Eh ファイルを閉じる(close) rbint:[D-213E](http://www.delorie.com/djgpp/doc/rbinter/id/93/27.html)
- int 21h AH=3Fh ファイル読み込み(read) rbint:[D-213F](http://www.delorie.com/djgpp/doc/rbinter/id/94/27.html)
- int 21h AH=40h ファイル書き込み(write)もしくはファイル長設定 rbint:[D-2140](http://www.delorie.com/djgpp/doc/rbinter/id/02/28.html)
- int 21h AH=41h ファイル削除(unlink) rbint:[D-2141](http://www.delorie.com/djgpp/doc/rbinter/id/08/28.html)
- int 21h AH=42h ファイルポインタ設定(lseek) rbint:[D-2142](http://www.delorie.com/djgpp/doc/rbinter/id/10/28.html)
- int 21h AH=43h ファイル属性取得／設定(attrib/chmod)
  - int 21h AX=4300h ファイル属性取得 rbint:[D-214300](http://www.delorie.com/djgpp/doc/rbinter/id/13/28.html)
  - int 21h AX=4301h ファイル属性設定 rbint:[D-214301](http://www.delorie.com/djgpp/doc/rbinter/id/14/28.html)
- int 21h AH=44h IOCTL  → [index](http://www.delorie.com/djgpp/doc/rbinter/ix/21/44.html)
  - int 21h AX=4400h ファイルハンドルのデバイス情報取得(isatty) rbint:[D-214400](http://www.delorie.com/djgpp/doc/rbinter/id/32/28.html)
  - int 21h AX=4401h ファイルハンドルのデバイス設定(キャラクターデバイス) rbint:[D-214401](http://www.delorie.com/djgpp/doc/rbinter/id/33/28.html)
  - int 21h AX=4402h IOCTLデータ読み込み(ファイルハンドル) rbint:[D-214402](http://www.delorie.com/djgpp/doc/rbinter/id/34/28.html)
  - int 21h AX=4403h IOCTLデータ書き込み(ファイルハンドル) rbint:[D-214403](http://www.delorie.com/djgpp/doc/rbinter/id/66/28.html)
  - int 21h AX=4404h IOCTLデータ読み込み(ブロックデバイス) rbint:[D-214404](http://www.delorie.com/djgpp/doc/rbinter/id/83/28.html)
  - int 21h AX=4405h IOCTLデータ書き込み(ブロックデバイス) rbint:[D-214405](http://www.delorie.com/djgpp/doc/rbinter/id/93/28.html)
  - int 21h AX=4406h 入力ステータス取得 rbint:[D-214406](http://www.delorie.com/djgpp/doc/rbinter/id/00/29.html)
  - int 21h AX=4407h 出力ステータス取得 rbint:[D-214407](http://www.delorie.com/djgpp/doc/rbinter/id/01/29.html)
  - int 21h AX=4408h 指定ドライブがリムーバブルかどうかの確認(DOS 3+) rbint:[D-214408](http://www.delorie.com/djgpp/doc/rbinter/id/02/29.html)
  - int 21h AX=4409h 指定ドライブがネットワーク(リモート)かどうかの確認(DOS 3.1+) rbint:[D-214409](http://www.delorie.com/djgpp/doc/rbinter/id/03/29.html)
  - int 21h AX=440Ah 指定ハンドルがネットワーク(リモート)上のものかどうか確認(DOS 3.1+) rbint:[D-21440A](http://www.delorie.com/djgpp/doc/rbinter/id/04/29.html)
  - int 21h AX=440Bh 共有リトライ回数と間隔の設定(DOS 3.1+) rbint:[D-21440B](http://www.delorie.com/djgpp/doc/rbinter/id/05/29.html)
  - int 21h AX=440Ch 汎用IOCTL(ファイルハンドル)(DOS 3.2+) rbint:[D-21440C](http://www.delorie.com/djgpp/doc/rbinter/id/06/29.html)
  - int 21h AX=440Dh 汎用IOCTL(ブロックデバイス)(DOS 3.2+) rbint:[D-21440D](http://www.delorie.com/djgpp/doc/rbinter/id/08/29.html)
  - int 21h AX=440Eh 論理ドライブマップ取得(DOS 3.2+) rbint:[D-21440E](http://www.delorie.com/djgpp/doc/rbinter/id/19/29.html)
  - int 21h AX=440Fh 論理ドライブマップ設定(DOS 3.2+) rbint:[D-21440F](http://www.delorie.com/djgpp/doc/rbinter/id/20/29.html)
  - int 21h AX=4410h 汎用IOCTL機能サポートチェック(ファイルハンドル)(DOS 5+) rbint:[D-214410](http://www.delorie.com/djgpp/doc/rbinter/id/21/29.html)
  - int 21h AX=4411h 汎用IOCTL機能サポートチェック(ブロックデバイス)(DOS 5+) rbint:[D-214411](http://www.delorie.com/djgpp/doc/rbinter/id/23/29.html)
- int 21h AX=4452h DR-DOSバージョン取得(DR-DOS3.41+) rbint:[D-214452](http://www.delorie.com/djgpp/doc/rbinter/id/32/29.html)
- int 21h AH=45h ファイルハンドルの複製(dup) rbint:[D-2145](http://www.delorie.com/djgpp/doc/rbinter/id/43/29.html)
- int 21h AH=46h ファイルハンドルの再利用(dup2) rbint:[D-2146](http://www.delorie.com/djgpp/doc/rbinter/id/44/29.html)
- int 21h AH=47h 現行ディレクトリの取得(getcwd) rbint:[D-2147](http://www.delorie.com/djgpp/doc/rbinter/id/45/29.html)
- int 21h AH=48h メモリ確保 rbint:[D-2148](http://www.delorie.com/djgpp/doc/rbinter/id/46/29.html)
- int 21h AH=49h メモリ開放 rbint:[D-2149](http://www.delorie.com/djgpp/doc/rbinter/id/47/29.html)
- int 21h AH=4Ah メモリブロックサイズ変更 rbint:[D-214A](http://www.delorie.com/djgpp/doc/rbinter/id/48/29.html)
- int 21h AH=4Bh プログラムのロード、実行 rbint:[D-214B](http://www.delorie.com/djgpp/doc/rbinter/id/51/29.html)
- int 21h AH=4Ch プログラムの終了 rbint:[D-214C](http://www.delorie.com/djgpp/doc/rbinter/id/86/29.html)
- int 21h AH=4Dh プログラム(子プロセス)の終了コード取得 rbint:[D-214D](http://www.delorie.com/djgpp/doc/rbinter/id/88/29.html)
- int 21h AH=4Eh ファイル検索(findfirst) rbint:[D-214E](http://www.delorie.com/djgpp/doc/rbinter/id/89/29.html)
- int 21h AH=4Fh ファイル検索(findnext) rbint:[D-214F](http://www.delorie.com/djgpp/doc/rbinter/id/91/29.html)
- int 21h AH=50h PSPアドレス設定(現行プロセスID変更) rbint:[D-2150](http://www.delorie.com/djgpp/doc/rbinter/id/92/29.html)
- int 21h AH=51h PSPアドレス取得 rbint:[D-2151](http://www.delorie.com/djgpp/doc/rbinter/id/94/29.html)
- int 21h AH=52h List of Lists(SYSVARS)取得 rbint:[D-2152](http://www.delorie.com/djgpp/doc/rbinter/id/95/29.html)
- int 21h AH=53h BPB(BIOSパラメータブロック)からDPBへの変換 rbint:[D-2153](http://www.delorie.com/djgpp/doc/rbinter/id/97/29.html)
- int 21h AH=54h ベリファイフラグ取得 rbint:[D-2154](http://www.delorie.com/djgpp/doc/rbinter/id/98/29.html)
- int 21h AH=55h 新PSP(子プロセス)作成 rbint:[D-2155](http://www.delorie.com/djgpp/doc/rbinter/id/01/30.html)
- int 21h AH=56h ファイルのリネーム rbint:[D-2156](http://www.delorie.com/djgpp/doc/rbinter/id/02/30.html)
- int 21h AH=57h ファイル日時の設定と取得 →[index](http://www.delorie.com/djgpp/doc/rbinter/ix/21/57.html)
  - int 21h AX=5700h ファイル更新日時を取得(fstat) rbint:[D-215700](http://www.delorie.com/djgpp/doc/rbinter/id/04/30.html)
  - int 21h AX=5701h ファイル日時を設定(futimes) rbint:[D-215701](http://www.delorie.com/djgpp/doc/rbinter/id/05/30.html)
- int 21h AH=58h メモリアロケーションストラテジの設定／取得 rbint:[D-2158](http://www.delorie.com/djgpp/doc/rbinter/id/20/30.html)
- int 21h AH=58h UMBリンク状態の設定／取得 rbint:[D-2158](http://www.delorie.com/djgpp/doc/rbinter/id/21/30.html)
- int 21h AH=59h BX=0000h 拡張エラー情報の取得 rbint:[D-2159--BX0000](http://www.delorie.com/djgpp/doc/rbinter/id/24/30.html)
- int 21h AH=5Ah 一時ファイル作成(mkstemp) rbint:[D-215A](http://www.delorie.com/djgpp/doc/rbinter/id/26/30.html)
- int 21h AH=5Bh 新規ファイル作成(creat O_EXCL) rbint:[D-215B](http://www.delorie.com/djgpp/doc/rbinter/id/27/30.html)
- int 21h AH=5Ch ファイル領域のロック(SHARE,fcntl F_GETLK/F_SETLK) rbint:[D-215C](http://www.delorie.com/djgpp/doc/rbinter/id/28/30.html)
- int 21h AX=5D00h サーバファンクションコール(RPC) rbint:[D-215D00](http://www.delorie.com/djgpp/doc/rbinter/id/29/30.html)
- int 21h AX=5D06h SDA(スワッパブルデータエリア)アドレス取得 rbint:[D-215D06](http://www.delorie.com/djgpp/doc/rbinter/id/35/30.html)
- int 21h AX=5D0Ah 拡張エラーコード設定 rbint:[D-215D0A](http://www.delorie.com/djgpp/doc/rbinter/id/39/30.html)
- int 21h AH=5Eh ネットワーク(DOS 3.1+)：マシン名、プリンター →[index](http://www.delorie.com/djgpp/doc/rbinter/ix/21/5F.html)
  - int 21h AX=5E00h マシン名の取得 rbint:[D-215E00](http://www.delorie.com/djgpp/doc/rbinter/id/42/30.html)
  - int 21h AX=5E01h マシン名の設定 rbint:[D-215E01CH00](http://www.delorie.com/djgpp/doc/rbinter/id/44/30.html)
- int 21h AH=5Fh ネットワーク(DOS 3.1+)：リダイレクト →[index](http://www.delorie.com/djgpp/doc/rbinter/ix/21/5F.html)
  - int 21h AX=5F00h リダイレクションモードの取得 rbint:[D-215F00](http://www.delorie.com/djgpp/doc/rbinter/id/53/30.html)
  - int 21h AX=5F01h リダイレクションモードの設定 rbint:[D-215F01](http://www.delorie.com/djgpp/doc/rbinter/id/54/30.html)
  - int 21h AX=5F02h リダイレクション・リスト・エントリーの取得 rbint:[D-215F02](http://www.delorie.com/djgpp/doc/rbinter/id/55/30.html)
  - int 21h AX=5F03h デバイスのリダイレクト rbint:[D-215F03](http://www.delorie.com/djgpp/doc/rbinter/id/56/30.html)
  - int 21h AX=5F04h リダイレクトのキャンセル rbint:[D-215F04](http://www.delorie.com/djgpp/doc/rbinter/id/57/30.html)
  - int 21h AX=5F05h 拡張リダイレクト・リスト・エントリーの取得(DOS 4+) rbint:[D-215F05](http://www.delorie.com/djgpp/doc/rbinter/id/58/30.html)
- int 21h AX=5F07h ドライブ有効化(DOS 5+) rbint:[D-215F07](http://www.delorie.com/djgpp/doc/rbinter/id/62/30.html)
- int 21h AX=5F08h ドライブ無効化(DOS 5+) rbint:[D-215F08](http://www.delorie.com/djgpp/doc/rbinter/id/64/30.html)
- int 21h AH=60h パス名の正規化(truename,realpath) rbint:[D-2160](http://www.delorie.com/djgpp/doc/rbinter/id/50/31.html)
- int 21h AH=62h PSPアドレス取得(DOS 3+) rbint:[D-2162](http://www.delorie.com/djgpp/doc/rbinter/id/53/31.html)
- int 21h AX=6300h DBCSベクター情報の取得 rbint:[D-216300](http://www.delorie.com/djgpp/doc/rbinter/id/56/31.html)
- int 21h AH=65h 拡張国別情報(DOS 3.3+)
  - int 21h AX=6501h～6507h 拡張国別情報の取得 rbint:[D-2165](http://www.delorie.com/djgpp/doc/rbinter/id/76/31.html)
  - int 21h AX=6520h～6522h 文字(列)の大文字化(DOS 4+) rbint:[D-2165](http://www.delorie.com/djgpp/doc/rbinter/id/77/31.html)
  - int 21h AX=6523h Yes/No文字の判定(DOS 4+) rbint:[D-216523](http://www.delorie.com/djgpp/doc/rbinter/id/78/31.html)
  - int 21h AX=65A0h～65A2h ファイル名文字(列)の大文字化(DOS 4+) rbint:[D-2165](http://www.delorie.com/djgpp/doc/rbinter/id/79/31.html)
- int 21h AX=6601h グローバルコードページの取得 rbint:[D-216601](http://www.delorie.com/djgpp/doc/rbinter/id/80/31.html)
- int 21h AX=6602h グローバルコードページの設定 rbint:[D-216602](http://www.delorie.com/djgpp/doc/rbinter/id/81/31.html)
- int 21h AH=67h ハンドル数の設定 rbint:[D-2167](http://www.delorie.com/djgpp/doc/rbinter/id/82/31.html)
- int 21h AH=68h ファイルのコミット(fsync) rbint:[D-2168](http://www.delorie.com/djgpp/doc/rbinter/id/83/31.html)
- int 21h AH=69h ディスクシリアル番号の取得／設定 rbint:[D-2169](http://www.delorie.com/djgpp/doc/rbinter/id/84/31.html)
- int 21h AH=6Ch 拡張オープン(DOS 4+) rbint:[D-216C](http://www.delorie.com/djgpp/doc/rbinter/id/92/31.html)
- int 21h AX=7302h 拡張DPB取得(DOS 7.1+) rbint:[D-217302](http://www.delorie.com/djgpp/doc/rbinter/id/39/32.html)
- int 21h AX=7303h ドライブ空き容量取得(DOS 7.1+) rbint:[D-217303](http://www.delorie.com/djgpp/doc/rbinter/id/40/32.html)
- int 21h AX=7304h 初期化用DPB設定(DOS 7.1+) rbint:[D-217304](http://www.delorie.com/djgpp/doc/rbinter/id/41/32.html)
- int 21h AX=7305h CX=FFFFh 絶対ディスク読み込み／書き込み(FAT32領域のみ) rbint:[D-217305CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/42/32.html)


int 21h以外のDOS機能・コールバックハンドラ
-------------------------

- int 22h プログラム終了アドレス rbint:[D-22](http://www.delorie.com/djgpp/doc/rbinter/id/23/41.html)

- int 23h Ctrl-Cハンドラ rbint:[D-23](http://www.delorie.com/djgpp/doc/rbinter/id/25/41.html)

- int 24h 致命的エラーハンドラ rbint:[D-24](http://www.delorie.com/djgpp/doc/rbinter/id/27/41.html)

- int 25h 絶対ディスク読み込み
  - int 25h 絶対ディスク読み込み(DOS 1～3.30) rbint:[D-25](http://www.delorie.com/djgpp/doc/rbinter/id/28/41.html)
  - int 25h CX=FFFFh 絶対ディスク読み込み(DOS 3.31+) rbint:[D-25----CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/29/41.html)
- int 26h 絶対ディスク書き込み
  - int 26h 絶対ディスク書き込み(DOS 1～3.30) rbint:[D-26](http://www.delorie.com/djgpp/doc/rbinter/id/33/41.html)
  - int 26h CX=FFFFh 絶対ディスク書き込み(DOS 3.31+) rbint:[D-26----CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/34/41.html)

- int 28h DOSアイドル割り込み rbint:[D-28](http://www.delorie.com/djgpp/doc/rbinter/id/38/41.html)

- int 29h 高速文字出力 rbint:[D-29](http://www.delorie.com/djgpp/doc/rbinter/id/40/41.html)

- int 2Eh コマンド実行 \(COMMAND.COM) rbint:[l-2E](http://www.delorie.com/djgpp/doc/rbinter/id/64/42.html)


表・データ構造
-------------


- FCB(ファイル制御ブロック) rbint:[Table 01345](http://www.delorie.com/djgpp/doc/rbinter/it/45/13.html)
- 拡張FCB(XFCB) rbint:[Table 01346](http://www.delorie.com/djgpp/doc/rbinter/it/46/13.html)
- FAT内ディレクトリエントリ rbint:[Table 01352](http://www.delorie.com/djgpp/doc/rbinter/it/52/13.html)
- メディアID(メディアディスクリプタバイト) rbint:[Table 01356](http://www.delorie.com/djgpp/doc/rbinter/it/56/13.html)
- BPB(BIOSパラメータブロック) rbint:[Table 01663](http://www.delorie.com/djgpp/doc/rbinter/id/97/29.html)
- DPB(ドライブパラメータブロック)
  - DOS 1.x rbint:[Table 01357](http://www.delorie.com/djgpp/doc/rbinter/it/57/13.html)
  - DOS 2+ rbint:[Table 01395](http://www.delorie.com/djgpp/doc/rbinter/it/95/13.html)
- PSP(プログラムセグメントプレフィックス) rbint:[Table 01378](http://www.delorie.com/djgpp/doc/rbinter/it/78/13.html)
- DOS拡張エラーコード rbint:[Table 01680](http://www.delorie.com/djgpp/doc/rbinter/it/80/16.html)
- ファイル検索データブロック(findfirst data block) rbint:[Table 01626](http://www.delorie.com/djgpp/doc/rbinter/it/26/16.html)
- ファイル属性値 rbint:[Table 01420](http://www.delorie.com/djgpp/doc/rbinter/it/20/14.html)
- EXEヘッダ rbint:[Table 01594](http://www.delorie.com/djgpp/doc/rbinter/it/94/15.html)
- List of Lists (SYSVARS) rbint:[Table 01627](http://www.delorie.com/djgpp/doc/rbinter/it/27/16.html)
- MCB(メモリ制御ブロック) rbint:[Table 01628](http://www.delorie.com/djgpp/doc/rbinter/it/28/16.html)
- SFT(システムファイルテーブル)
  - DOS 2.x rbint:[Table 01639](http://www.delorie.com/djgpp/doc/rbinter/it/39/16.html)
  - DOS 3.0 rbint:[Table 01640](http://www.delorie.com/djgpp/doc/rbinter/it/40/16.html)
  - DOS 3.1～3.31 rbint:[Table 01641](http://www.delorie.com/djgpp/doc/rbinter/it/41/16.html)
  - DOS 4+ rbint:[Table 01642](http://www.delorie.com/djgpp/doc/rbinter/it/42/16.html)
- デバイスドライバ
  - ドライバヘッダ rbint:[Table 01646](http://www.delorie.com/djgpp/doc/rbinter/it/46/16.html)
  - コマンドリスト rbint:[Table 02595](http://www.delorie.com/djgpp/doc/rbinter/it/95/25.html)
  - リクエストヘッダ rbint:[Table 02597](http://www.delorie.com/djgpp/doc/rbinter/it/97/25.html)
  - ステータスコード rbint:[Table 02596](http://www.delorie.com/djgpp/doc/rbinter/it/96/25.html)
  - エラーコード rbint:[Table 02598](http://www.delorie.com/djgpp/doc/rbinter/it/98/25.html)
- CDS(カレントディレクトリストラクチャー) rbint:[Table 01643](http://www.delorie.com/djgpp/doc/rbinter/it/43/16.html)
- ディスクバッファ
  - DOS 2.x rbint:[Table 01649](http://www.delorie.com/djgpp/doc/rbinter/it/49/16.html)
  - DOS 3.x rbint:[Table 01650](http://www.delorie.com/djgpp/doc/rbinter/it/50/16.html)
  - DOS 4.00 rbint:[Table 01652](http://www.delorie.com/djgpp/doc/rbinter/it/52/16.html)
  - DOS 4.01+ rbint:[Table 01653](http://www.delorie.com/djgpp/doc/rbinter/it/53/16.html)
- DOSファイル時刻フォーマット
  - 時間(時分秒) rbint:[Table 01665](http://www.delorie.com/djgpp/doc/rbinter/it/65/16.html)
  - 日付(年月日) rbint:[Table 01666](http://www.delorie.com/djgpp/doc/rbinter/it/66/16.html)
- SDA(スワッパブルデータエリア)
  - DOS 3.1～3.30 rbint:[Table 01687](http://www.delorie.com/djgpp/doc/rbinter/it/87/16.html)
  - DOS 4+ rbint:[Table 01690](http://www.delorie.com/djgpp/doc/rbinter/it/90/16.html)
- 国別情報テーブル rbint:[Table 01399](http://www.delorie.com/djgpp/doc/rbinter/it/99/13.html)
- 拡張国別情報テーブル rbint:[Table 01750](http://www.delorie.com/djgpp/doc/rbinter/it/50/17.html)


***


カテゴリ別：文字表示とキー入力
------------------------------

要はハンドル 0（標準入力）、1（標準出力）、3（標準補助出力）、4（プリンタ）への入出力。  
CONデバイスがCOOKEDモード（デフォルト）の場合、CONデバイスに対するファンクション3Fhの読み込みは行入力モードとなる。行入力したくない場合はファンクション4401hを使って標準入力をRAW(BINARY)モードに設定するか、4401hや0Ah以外の入力ファンクションを使う。
int 29hはコンソールドライバ内部の文字表示ルーチンを直接呼び出すため。DOSによるリダイレクトの影響を受けない。

- int 21h AH=01h 標準入力から１文字入力（エコー付） rbint:[D-2101](http://www.delorie.com/djgpp/doc/rbinter/id/63/25.html)
- int 21h AH=02h 標準出力に１文字出力 rbint:[D-2102](http://www.delorie.com/djgpp/doc/rbinter/id/65/25.html)
- int 21h AH=03h 標準補助入力(AUX)から１文字入力 rbint:[D-2103](http://www.delorie.com/djgpp/doc/rbinter/id/66/25.html)
- int 21h AH=04h 標準補助出力(AUX)に１文字出力 rbint:[D-2104](http://www.delorie.com/djgpp/doc/rbinter/id/67/25.html)
- int 21h AH=05h プリンタ(PRN)に１文字出力 rbint:[D-2105](http://www.delorie.com/djgpp/doc/rbinter/id/68/25.html)
- int 21h AH=06h 直接コンソール出力 rbint:[D-2106](http://www.delorie.com/djgpp/doc/rbinter/id/69/25.html)
- int 21h AH=06h DL=FFh 直接コンソール入力 rbint:[D-2106--DLFF](http://www.delorie.com/djgpp/doc/rbinter/id/70/25.html)
- int 21h AH=07h 直接文字入力（エコーなし） rbint:[D-2107](http://www.delorie.com/djgpp/doc/rbinter/id/71/25.html)
- int 21h AH=08h 文字入力（エコーなし） rbint:[D-2108](http://www.delorie.com/djgpp/doc/rbinter/id/72/25.html)
- int 21h AH=09h 文字列出力 rbint:[D-2109](http://www.delorie.com/djgpp/doc/rbinter/id/73/25.html)
- int 21h AH=0Ah 行入力 rbint:[D-210A](http://www.delorie.com/djgpp/doc/rbinter/id/74/25.html)
- int 21h AH=0Bh 標準入力ステータス取得 rbint:[D-210B](http://www.delorie.com/djgpp/doc/rbinter/id/76/25.html)
- int 21h AH=0Ch 標準入力バッファ消去（後に入力） rbint:[D-210C](http://www.delorie.com/djgpp/doc/rbinter/id/78/25.html)
- int 21h AH=3Fh ファイル読み込み(read) rbint:[D-213F](http://www.delorie.com/djgpp/doc/rbinter/id/94/27.html)
- int 21h AH=40h ファイル書き込み(write)もしくはファイル長設定 rbint:[D-2140](http://www.delorie.com/djgpp/doc/rbinter/id/02/28.html)
- int 21h AX=4400h ファイルハンドルのデバイス情報取得(isatty) rbint:[D-214400](http://www.delorie.com/djgpp/doc/rbinter/id/32/28.html)
- int 21h AX=4401h ファイルハンドルのデバイス設定(キャラクターデバイス) rbint:[D-214401](http://www.delorie.com/djgpp/doc/rbinter/id/33/28.html)
- int 21h AH=33h (AL=00h/01h) BREAKチェックの状態取得／設定 rbint:[D-2133](http://www.delorie.com/djgpp/doc/rbinter/id/36/27.html)

- int 29h 高速文字出力 rbint:[D-29](http://www.delorie.com/djgpp/doc/rbinter/id/40/41.html)


カテゴリ別：ファイル操作（ハンドル）
------------------------------------

ファイル作成ファンクションが３つある。3Chは「ファイルが存在する場合は0バイトに初期化」、5Bhは存在時にエラーを返す。6Chの拡張オープンは3Ch/5Bh/3Dhのオープンをひとつで処理できるがDOS 3.x未対応。

ファイル書き込みファンクションで、書き込みサイズを 0 バイトに設定した場合、ファイルは現在のシークポイントに切り詰め、もしくは伸張される(ftruncate)。


- int 21h AH=3Ch ファイル作成(creat) rbint:[D-213C](http://www.delorie.com/djgpp/doc/rbinter/id/89/27.html)
- int 21h AH=3Dh ファイルを開く(open) rbint:[D-213D](http://www.delorie.com/djgpp/doc/rbinter/id/90/27.html)
- int 21h AH=3Eh ファイルを閉じる(close) rbint:[D-213E](http://www.delorie.com/djgpp/doc/rbinter/id/93/27.html)
- int 21h AH=3Fh ファイル読み込み(read) rbint:[D-213F](http://www.delorie.com/djgpp/doc/rbinter/id/94/27.html)
- int 21h AH=40h ファイル書き込み(write)もしくはファイル長設定 rbint:[D-2140](http://www.delorie.com/djgpp/doc/rbinter/id/02/28.html)
- int 21h AH=41h ファイル削除(unlink) rbint:[D-2141](http://www.delorie.com/djgpp/doc/rbinter/id/08/28.html)
- int 21h AH=42h ファイルポインタ設定(lseek) rbint:[D-2142](http://www.delorie.com/djgpp/doc/rbinter/id/10/28.html)
- int 21h AH=45h ファイルハンドルの複製(dup) rbint:[D-2145](http://www.delorie.com/djgpp/doc/rbinter/id/43/29.html)
- int 21h AH=46h ファイルハンドルの再利用(dup2) rbint:[D-2146](http://www.delorie.com/djgpp/doc/rbinter/id/44/29.html)
- int 21h AX=5700h ファイル更新日時を取得(fstat) rbint:[D-215700](http://www.delorie.com/djgpp/doc/rbinter/id/04/30.html)
- int 21h AX=5701h ファイル日時を設定(futimes) rbint:[D-215701](http://www.delorie.com/djgpp/doc/rbinter/id/05/30.html)
- int 21h AH=5Ah 一時ファイル作成(mkstemp) rbint:[D-215A](http://www.delorie.com/djgpp/doc/rbinter/id/26/30.html)
- int 21h AH=5Bh 新規ファイル作成(creat O_EXCL) rbint:[D-215B](http://www.delorie.com/djgpp/doc/rbinter/id/27/30.html)
- int 21h AH=5Ch ファイル領域のロック(SHARE,fcntl F_GETLK/F_SETLK) rbint:[D-215C](http://www.delorie.com/djgpp/doc/rbinter/id/28/30.html)
- int 21h AH=67h ハンドル数の設定 rbint:[D-2167](http://www.delorie.com/djgpp/doc/rbinter/id/82/31.html)
- int 21h AH=68h ファイルのコミット(fsync) rbint:[D-2168](http://www.delorie.com/djgpp/doc/rbinter/id/83/31.html)
- int 21h AH=6Ch 拡張オープン(DOS 4+) rbint:[D-216C](http://www.delorie.com/djgpp/doc/rbinter/id/92/31.html)


カテゴリ別：ディレクトリ、ドライブ
----------------------------------

- int 21h AH=0Dh ディスクリセット rbint:[D-210D](http://www.delorie.com/djgpp/doc/rbinter/id/79/25.html)
- int 21h AH=0Eh 現行ドライブ設定 rbint:[D-210E](http://www.delorie.com/djgpp/doc/rbinter/id/81/25.html)
- int 21h AH=19h 現行ドライブ取得 rbint:[D-2119](http://www.delorie.com/djgpp/doc/rbinter/id/99/25.html)
- int 21h AH=39h ディレクトリ作成(mkdir) rbint:[D-2139](http://www.delorie.com/djgpp/doc/rbinter/id/86/27.html)
- int 21h AH=3Ah ディレクトリ削除(rmdir) rbint:[D-213A](http://www.delorie.com/djgpp/doc/rbinter/id/87/27.html)
- int 21h AH=3Bh 現行ディレクトリ設定(chdir) rbint:[D-213B](http://www.delorie.com/djgpp/doc/rbinter/id/88/27.html)
- int 21h AH=47h 現行ディレクトリの取得(getcwd) rbint:[D-2147](http://www.delorie.com/djgpp/doc/rbinter/id/45/29.html)
- int 21h AX=3305h (DOS 4+) 起動ドライブ取得 rbint:[D-213305](http://www.delorie.com/djgpp/doc/rbinter/id/40/27.html)


カテゴリ別：ファイル検索、属性
------------------------------

ファイル名を指定してファイル時刻を変更するファンクション（lstat相当）は存在しないようだ。
ファイル更新日時を特定の時刻に変更したい場合は、当該ファイルを **リードオンリーモードで** オープンし、Func 5701hで時刻を設定したのち、ファイルをクローズする必要がある。
逆にファイル属性の取得や変更はファイル名指定で行い、ハンドル経由のファンクションコールは存在しない。


- int 21h AX=4300h ファイル属性取得 rbint:[D-214300](http://www.delorie.com/djgpp/doc/rbinter/id/13/28.html)
- int 21h AX=4301h ファイル属性設定(chmod) rbint:[D-214301](http://www.delorie.com/djgpp/doc/rbinter/id/14/28.html)
- int 21h AX=5700h ファイル更新日時を取得(fstat) rbint:[D-215700](http://www.delorie.com/djgpp/doc/rbinter/id/04/30.html)
- int 21h AX=5701h ファイル日時を設定(futimes) rbint:[D-215701](http://www.delorie.com/djgpp/doc/rbinter/id/05/30.html)
- int 21h AH=4Eh ファイル検索(findfirst) rbint:[D-214E](http://www.delorie.com/djgpp/doc/rbinter/id/89/29.html)
- int 21h AH=4Fh ファイル検索(findnext) rbint:[D-214F](http://www.delorie.com/djgpp/doc/rbinter/id/91/29.html)
- int 21h AH=60h パス名の正規化(truename,realpath) rbint:[D-2160](http://www.delorie.com/djgpp/doc/rbinter/id/50/31.html)
- int 21h AH=11h FCBによるファイル検索(findfirst) rbint:[D-2111](http://www.delorie.com/djgpp/doc/rbinter/id/87/25.html)
- int 21h AH=12h FCBによるファイル検索(findnext) rbint:[D-2112](http://www.delorie.com/djgpp/doc/rbinter/id/88/25.html)
- int 21h AH=1Ah DTA(ディスク転送エリア)設定 rbint:[D-211A](http://www.delorie.com/djgpp/doc/rbinter/id/00/26.html)
- int 21h AH=2Fh DTA(ディスク転送エリア)アドレス取得 rbint:[D-212F](http://www.delorie.com/djgpp/doc/rbinter/id/21/27.html)


カテゴリ別：ファイル操作（FCB）
-------------------------------

DOS 1.x時代のファイルアクセス法であり、DOS 3以上のネットワークドライブでは疑似的にサポートされる。
サブディレクトリが使えず、ファイルへの読み書きは「レコード」と呼ばれる単位で行われる。1レコードはデフォルト設定で128バイトになる（設定はオープン後に変更可能だが、128バイトより大きな値にした場合はDTAのサイズもその分だけ必要になる）。

どうしてもDOS 1.xで動作するプログラムを作りたい場合以外で使うメリットはないが、ボリュームラベルの作成と削除はDOS 2.0以降でもFCB経由で行う必要がある。

- int 21h AH=0Fh FCBによるファイルオープン rbint:[D-210F](http://www.delorie.com/djgpp/doc/rbinter/id/85/25.html)
- int 21h AH=16h FCBによるファイル作成
rbint:[D-211F](http://www.delorie.com/djgpp/doc/rbinter/id/05/26.html)
- int 21h AH=10h FCBによるファイルクローズ rbint:[D-2110](http://www.delorie.com/djgpp/doc/rbinter/id/86/25.html)
- int 21h AH=11h FCBによるファイル検索(findfirst) rbint:[D-2111](http://www.delorie.com/djgpp/doc/rbinter/id/87/25.html)
- int 21h AH=12h FCBによるファイル検索(findnext) rbint:[D-2112](http://www.delorie.com/djgpp/doc/rbinter/id/88/25.html)
- int 21h AH=13h FCBによるファイル消去 rbint:[D-2113](http://www.delorie.com/djgpp/doc/rbinter/id/89/25.html)
- int 21h AH=14h FCBによるファイル読み込み(シーケンシャル) rbint:[D-2114](http://www.delorie.com/djgpp/doc/rbinter/id/90/25.html)
- int 21h AH=15h FCBによるファイル書き込み(シーケンシャル) rbint:[D-2115](http://www.delorie.com/djgpp/doc/rbinter/id/91/25.html)
- int 21h AH=17h FCBによるファイルリネーム rbint:[D-2117](http://www.delorie.com/djgpp/doc/rbinter/id/93/25.html)
- int 21h AH=21h FCBによるファイル読み込み(ランダムレコード、1ブロック) rbint:[D-2121](http://www.delorie.com/djgpp/doc/rbinter/id/09/26.html)
- int 21h AH=22h FCBによるファイル書き込み(ランダムレコード、1ブロック) rbint:[D-2122](http://www.delorie.com/djgpp/doc/rbinter/id/10/26.html)
- int 21h AH=23h FCBによるファイルサイズ(レコード数)取得 rbint:[D-2123](http://www.delorie.com/djgpp/doc/rbinter/id/11/26.html)
- int 21h AH=24h FCBによるファイルレコード位置設定(lseek?) rbint:[D-2124](http://www.delorie.com/djgpp/doc/rbinter/id/12/26.html)
- int 21h AH=27h FCBによるファイル読み込み(ランダムレコード、複数ブロック) rbint:[D-2127](http://www.delorie.com/djgpp/doc/rbinter/id/94/26.html)
- int 21h AH=28h FCBによるファイル書き込み(ランダムレコード、複数ブロック) rbint:[D-2128](http://www.delorie.com/djgpp/doc/rbinter/id/95/26.html)
- int 21h AH=29h ファイル名をFCB形式に変換 rbint:[D-2129](http://www.delorie.com/djgpp/doc/rbinter/id/96/26.html)
- int 21h AH=2Fh DTA(ディスク転送エリア)アドレス取得 rbint:[D-212F](http://www.delorie.com/djgpp/doc/rbinter/id/21/27.html)
- int 21h AH=1Ah DTA(ディスク転送エリア)設定 rbint:[D-211A](http://www.delorie.com/djgpp/doc/rbinter/id/00/26.html)


カテゴリ別：ディスク読み込み／書き込み
--------------------------------------

- int 25h 絶対ディスク読み込み(DOS 1～3.30) rbint:[D-25](http://www.delorie.com/djgpp/doc/rbinter/id/28/41.html)
- int 25h CX=FFFFh 絶対ディスク読み込み(DOS 3.31+) rbint:[D-25----CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/29/41.html)
- int 26h 絶対ディスク書き込み(DOS 1～3.30) rbint:[D-26](http://www.delorie.com/djgpp/doc/rbinter/id/33/41.html)
- int 26h CX=FFFFh 絶対ディスク書き込み(DOS 3.31+) rbint:[D-26----CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/34/41.html)
- int 21h AX=7305h CX=FFFFh 絶対ディスク読み込み／書き込み(FAT32領域のみ) rbint:[D-217305CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/42/32.html)
- int 21h AX=440Dh 汎用IOCTL(ブロックデバイス) rbint:[D-21440D](http://www.delorie.com/djgpp/doc/rbinter/id/08/29.html)
  - int 21h AX=440Dh CL=41h 論理ドライブへのトラック書き込み
  - int 21h AX=440Dh CL=60h ドライブパラメータ取得
  - int 21h AX=004Dh CL=61h 論理ドライブからのトラック読み込み


カテゴリ別：ディスク情報
------------------------

- int 21h AH=1Bh 現行ドライブのFAT情報を得る rbint:[D-211B](http://www.delorie.com/djgpp/doc/rbinter/id/01/26.html)
- int 21h AH=1Ch 指定ドライブのFAT情報を得る rbint:[D-211C](http://www.delorie.com/djgpp/doc/rbinter/id/02/26.html)
- int 21H AH=1Fh 現行ドライブのDPBを得る rbint:[D-211F](http://www.delorie.com/djgpp/doc/rbinter/id/05/26.html)
- int 21H AH=32h 指定ドライブのDPBを得る rbint:[D-2132](http://www.delorie.com/djgpp/doc/rbinter/id/35/27.html)
- int 21h AH=36h 指定ドライブの容量取得 rbint:[D-2136](http://www.delorie.com/djgpp/doc/rbinter/id/62/27.html)
- int 21h AX=4408h 指定ドライブがリムーバブルかどうかの確認(DOS 3+) rbint:[D-214408](http://www.delorie.com/djgpp/doc/rbinter/id/02/29.html)
- int 21h AX=4409h 指定ドライブがネットワーク(リモート)かどうかの確認(DOS 3.1+) rbint:[D-214409](http://www.delorie.com/djgpp/doc/rbinter/id/03/29.html)
- int 21h AX=440Dh 汎用IOCTL(ブロックデバイス) rbint:[D-21440D](http://www.delorie.com/djgpp/doc/rbinter/id/08/29.html)
  - int 21h AX=440Dh CL=40h ドライブパラメータ設定
  - int 21h AX=440Dh CL=60h ドライブパラメータ取得
- int 21h AX=440Eh 論理ドライブマップ取得(DOS 3.2+) rbint:[D-21440E](http://www.delorie.com/djgpp/doc/rbinter/id/19/29.html)
- int 21h AX=440Fh 論理ドライブマップ設定(DOS 3.2+) rbint:[D-21440F](http://www.delorie.com/djgpp/doc/rbinter/id/20/29.html)
- int 21h AH=53h BPB(BIOSパラメータブロック)からDPBへの変換 rbint:[D-2153](http://www.delorie.com/djgpp/doc/rbinter/id/97/29.html)
- int 21h AH=69h ディスクシリアル番号の取得／設定 rbint:[D-2169](http://www.delorie.com/djgpp/doc/rbinter/id/84/31.html)
- int 21h AX=7302h 拡張DPB取得(DOS 7.1+) rbint:[D-217302](http://www.delorie.com/djgpp/doc/rbinter/id/39/32.html)
- int 21h AX=7303h ドライブ空き容量取得(DOS 7.1+) rbint:[D-217303](http://www.delorie.com/djgpp/doc/rbinter/id/40/32.html)
- int 21h AX=7304h 初期化用DPB設定(DOS 7.1+)
rbint:[D-217305CXFFFF](http://www.delorie.com/djgpp/doc/rbinter/id/42/32.html)


カテゴリ別：メモリ
------------------

- int 21h AH=48h メモリ確保 rbint:[D-2148](http://www.delorie.com/djgpp/doc/rbinter/id/46/29.html)
- int 21h AH=49h メモリ開放 rbint:[D-2149](http://www.delorie.com/djgpp/doc/rbinter/id/47/29.html)
- int 21h AH=4Ah メモリブロックサイズ変更 rbint:[D-214A](http://www.delorie.com/djgpp/doc/rbinter/id/48/29.html)
- int 21h AH=58h メモリアロケーションストラテジの設定／取得 rbint:[D-2158](http://www.delorie.com/djgpp/doc/rbinter/id/20/30.html)
- int 21h AH=58h UMBリンク状態の設定／取得(DOS 5+) rbint:[D-2158](http://www.delorie.com/djgpp/doc/rbinter/id/21/30.html)


カテゴリ別：プロセス（プログラム）
--------------------------------

- int 21h AH=00h プログラム終了(COMプログラム用) rbint:[D-2100](http://www.delorie.com/djgpp/doc/rbinter/id/62/25.html)
- int 21h AH=26h PSP作成(COMプログラム用) rbint:[D-2126](http://www.delorie.com/djgpp/doc/rbinter/id/93/26.html)
- int 21h AH=31h プログラムの常駐終了 (TSR:Terminate and Stay Resident) rbint:[D-2131](http://www.delorie.com/djgpp/doc/rbinter/id/34/27.html)
- int 21h AH=4Bh プログラムのロード、実行 rbint:[D-214B](http://www.delorie.com/djgpp/doc/rbinter/id/51/29.html)
- int 21h AH=4Ch プログラムの終了 rbint:[D-214C](http://www.delorie.com/djgpp/doc/rbinter/id/86/29.html)
- int 21h AH=4Dh プログラム(子プロセス)の終了コード取得 rbint:[D-214D](http://www.delorie.com/djgpp/doc/rbinter/id/88/29.html)
- int 21h AH=50h PSPアドレス設定(現行プロセスID変更) rbint:[D-2150](http://www.delorie.com/djgpp/doc/rbinter/id/92/29.html)
- int 21h AH=51h PSPアドレス取得 rbint:[D-2151](http://www.delorie.com/djgpp/doc/rbinter/id/94/29.html)
- int 21h AH=55h 新PSP(子プロセス)作成 rbint:[D-2155](http://www.delorie.com/djgpp/doc/rbinter/id/01/30.html)
- int 21h AH=62h PSPアドレス取得(DOS 3+) rbint:[D-2162](http://www.delorie.com/djgpp/doc/rbinter/id/53/31.html)


カテゴリ別：エラー
------------------

- int 21h AH=59h BX=0000h 拡張エラー情報の取得 rbint:[D-2159--BX0000](http://www.delorie.com/djgpp/doc/rbinter/id/24/30.html)
- int 21h AX=5D0Ah 拡張エラーコード設定 rbint:[D-215D0A](http://www.delorie.com/djgpp/doc/rbinter/id/39/30.html)
- DOS拡張エラーコード rbint:[Table 01680](http://www.delorie.com/djgpp/doc/rbinter/it/80/16.html)

- デバイスドライバ ステータスコード rbint:[Table 02596](http://www.delorie.com/djgpp/doc/rbinter/it/96/25.html) とエラーコード rbint:[Table 02598](http://www.delorie.com/djgpp/doc/rbinter/it/98/25.html)

- int 23h Ctrl-Cハンドラ rbint:[D-23](http://www.delorie.com/djgpp/doc/rbinter/id/25/41.html)

- int 24h 致命的エラーハンドラ rbint:[D-24](http://www.delorie.com/djgpp/doc/rbinter/id/27/41.html)


カテゴリ別：システム情報
------------------------

- int 21h AH=2Ah 現在の日付を取得 rbint:[D-212A](http://www.delorie.com/djgpp/doc/rbinter/id/97/26.html)
- int 21h AH=2Bh 現在の日付を設定 rbint:[D-212B](http://www.delorie.com/djgpp/doc/rbinter/id/98/26.html)
- int 21h AH=2Ch 現在の時刻を取得 rbint:[D-212C](http://www.delorie.com/djgpp/doc/rbinter/id/14/27.html)
- int 21h AH=2Dh 現在の時刻を設定 rbint:[D-212D](http://www.delorie.com/djgpp/doc/rbinter/id/18/27.html)
- int 21h AH=25h 割り込みベクタ設定 rbint:[D-2125](http://www.delorie.com/djgpp/doc/rbinter/id/13/26.html)
- int 21h AH=35h 割り込みベクタアドレス取得 rbint:[D-2135](http://www.delorie.com/djgpp/doc/rbinter/id/51/27.html)
- int 21h AH=30h DOSバージョン取得 rbint:[D-2130](http://www.delorie.com/djgpp/doc/rbinter/id/22/27.html)
- int 21h AX=3306h (DOS 5+) 真のバージョン取得 rbint:[D-213306](http://www.delorie.com/djgpp/doc/rbinter/id/41/27.html)
- int 21h AX=4452h DR-DOSバージョン取得(DR-DOS3.41+) rbint:[D-214452](http://www.delorie.com/djgpp/doc/rbinter/id/32/29.html)
- int 21h AH=2Eh (DL=00h) ベリファイフラグ設定 rbint:[D-212E--DL00](http://www.delorie.com/djgpp/doc/rbinter/id/20/27.html)
- int 21h AH=54h ベリファイフラグ取得 rbint:[D-2154](http://www.delorie.com/djgpp/doc/rbinter/id/98/29.html)
- int 21h AH=33h (AL=00h/01h) BREAKチェックの状態取得／設定 rbint:[D-2133](http://www.delorie.com/djgpp/doc/rbinter/id/36/27.html)
- int 21h AX=3700h オプションスイッチ用の文字(SWITCHAR)取得 rbint:[D-213700](http://www.delorie.com/djgpp/doc/rbinter/id/63/27.html)
- int 21h AX=3701h オプションスイッチ用の文字(SWITCHAR)設定 rbint:[D-213701](http://www.delorie.com/djgpp/doc/rbinter/id/64/27.html)
- int 21h AX=3305h (DOS 4+) 起動ドライブ取得 rbint:[D-213305](http://www.delorie.com/djgpp/doc/rbinter/id/40/27.html)
- int 21h AH=34h InDOSフラグのアドレス取得 rbint:[D-2134](http://www.delorie.com/djgpp/doc/rbinter/id/50/27.html)
- int 21h AH=38h 国別情報の取得／設定 rbint:[D-2138](http://www.delorie.com/djgpp/doc/rbinter/id/84/27.html)
- int 21h AH=52h List of Lists(SYSVARS)取得 rbint:[D-2152](http://www.delorie.com/djgpp/doc/rbinter/id/95/29.html)
- int 21h AX=5D06h SDA(スワッパブルデータエリア)アドレス取得 rbint:[D-215D06](http://www.delorie.com/djgpp/doc/rbinter/id/35/30.html)
  - DOS 4+ SDA rbint:[Table 01690](http://www.delorie.com/djgpp/doc/rbinter/it/90/16.html)
- int 21h AX=6300h DBCSベクター情報の取得 rbint:[D-216300](http://www.delorie.com/djgpp/doc/rbinter/id/56/31.html)
- int 21h AX=6501h～6507h 拡張国別情報の取得 rbint:[D-2165](http://www.delorie.com/djgpp/doc/rbinter/id/76/31.html)
