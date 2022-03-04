# NONASCII - a simple filter for 'ShiftJIS-unaware' C/C++

README for NONASCII (in Japanese)


## 説明

基本的には、シフトJIS（CP932）エンコードされた文字（列）の含まれるC/C++言語ソースを、シフトJIS未対応のC/C++コンパイラで正しくコンパイルさせるための簡易フィルタです。
具体的な処理として、

- 2バイト目が \ (0x5c) で終わるシフトJIS文字の後ろに \ を追加
- 文字コード0x80～0xffの文字を \xnn の形式にエスケープ

のいずれかが選べます。
構文解析的なことは一切していないので、文字（列）リテラル中だけでなくコメントなども変換対象となります。


## 書式

  nonascii -s [-i _input_file_] [-o _output_file_]  
  nonascii -x [-i _input_file_] [-o _output_file_]  

## オプション

**-s**
シフトJISの２バイト目が \（文字コード0x5c）の文字の直後に \ を追加します。
シフトJIS（CP932）のテキストエンコーディングを考慮していないC/C++コンパイラや一部のアセンブラ（nasmなど）で、シフトJIS文字列をなるべくソースの意図通りにコンパイル／アセンブルするためのオプションです。

**-x**
文字コードが0x80～0xffの文字をすべて \x*nn* に置換します。
シフトJISに限らずすべての非Unicode系文字エンコーディングで比較的安全に使えますが、（人間にとっての）変換後の視認性が落ちます。

**-i** _input_file_
入力ファイルを指定します。
省略時は標準入力からの入力となります。

**-o** _output_file_
出力ファイルを指定します。
省略時は標準出力への出力となります。


## ソース

https://github.com/lpproj/mydosuty/tree/master/nonascii

ANSI/ISO準拠のCコンパイラ（C89～C11）でコンパイル可能です。

