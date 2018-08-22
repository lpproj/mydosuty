README for mtcp2td in Japanese (encoded with Shift_JIS, CP932)


概要
----

mTCP の設定ファイルから IP などの設定を取り出し、TEEN 設定ファイル中の
該当部分をその内容で書き換えるというものです。


用法
----

事前に mTCP と TEEN 用の設定ファイルをそれなりに正しく設定し、環境変数に
それぞれのファイルのフルパス名を設定しておく必要があります。

設定が必要な環境変数:

  MTCPCFG       mTCP用設定ファイルのフルパス名
  TEEN          TEEN用定義ファイルのフルパス名

mtcp2td を実行すると、mTCP 用設定ファイル内の設定で TEEN 用設定の対応部分が
書き換えられます。

  mTCP      |   TEEN
------------+-------------------------------------------
  IPADDR    |   [ETHERNET] 内 <NETIF> サブセクション 400
  NETMASK   |   [ETHERNET] 内 <NETIF> サブセクション 401
  GATEWAY   |   [ETHERNET] 内 <NETIF> サブセクション 404
  RESOLVER  |   [RESOLVER] 内 300

TEEN 側の対応部分が存在していない場合、新規作成はされない点に注意して
ください。TEEN の定義ファイル中にあらかじめそれっぽい値を書いておく必要が
あるでしょう。
