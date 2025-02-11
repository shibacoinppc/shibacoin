<h1 align="center">
<img src="https://i.imgur.com/j6X1XyF.png" alt="Shibacoin" width="300"/>
<br/><br/>
Shibacoin Core [SHIC, ₷]  
</h1>

言語を選択: [EN](./README.md) | [CN](./README_zh_CN.md) | [PT](./README_pt_BR.md) | [FA](./README_fa_IR.md) | [VI](./README_vi_VN.md) | JA

Shibacoinは、2013年のオリジナルのDogecoin shibesの1人によって作成されたコミュニティに焦点を当てた暗号通貨です。これは、オリジナルのDogecoinコミュニティと同じように、新しくて楽しいコミュニティを作成するために作成されました。

以前のすべてのバージョンとは異なり、Shibacoinはレイヤー1のコインです。これは、排水する流動性プールがなく、ブラックリストに登録されたウォレットがなく、混乱するスマートコントラクトがないことを意味します。Shibacoinはシンプルなブロックチェーンです。

Dogecoinと同様に、Shibacoin Coreソフトウェアは、誰でもShibacoinブロックチェーンネットワークでノードを操作できるようにし、作業証明のためにScryptハッシュメソッドを使用します。これは、Bitcoin Coreおよび他の暗号通貨から適応されています。

Shibacoinネットワークで使用されるデフォルトの料金に関する情報については、[料金の推奨](doc/fee-recommendation.md)を参照してください。

**ウェブサイト:** [shibainucoin.net](https://shibainucoin.net)

## Dogecoinの違い

ShibacoinはDogecoinのフォークです。親しみやすさのために、ShibacoinをDogecoinに似せるように努めます。

変更点：

* アドレスは `D` の代わりに `S` で始まります
* BIPS機能はブロック1000から開始されます
* AuxPowはブロック24,000から開始されます（チェーンID：74）
* Shibacoin用にテーマ化されたGUI

## 使用法 💻

Shibacoin Coreを使い始めるには、[インストールガイド](INSTALL.md)と[入門](doc/getting-started.md)チュートリアルを参照してください。

Shibacoin Coreが提供するJSON-RPC APIは自己文書化されており、`shibacoin-cli help`を使用して参照できます。各コマンドの詳細情報は`shibacoin-cli help <command>`を使用して表示できます。あるいは、[Bitcoin Coreのドキュメント](https://developer.bitcoin.org/reference/rpc/)を参照してください。これは同様のプロトコルを実装しており、参照可能なバージョンを取得できます。

### ポート

Shibacoin Coreはデフォルトでポート`33864`を使用してピアツーピア通信を行います。これは、「メインネット」ブロックチェーンを同期し、新しいトランザクションとブロックの情報を取得するために必要です。さらに、JSONRPCポートを開くことができ、デフォルトではメインネットノードのポート`33863`です。RPCポートをパブリックインターネットに公開しないことを強くお勧めします。

| 機能     | メインネット | テストネット | regtest |
| :------- | ------: | ------: | ------: |
| P2P      |   33864 |   44864 |   18444 |
| RPC      |   33863 |   44863 |   18332 |

## 継続的な開発 💻

Shibacoin Coreはオープンソースでコミュニティ主導のソフトウェアです。開発プロセスは公開されており、誰でもソフトウェアを見て、議論し、作業することができます。

主な開発リソース：

* [GitHubプロジェクト](https://github.com/shibacoinppc/shibacoin/projects)は、今後のリリースのための計画および進行中の作業を追跡するために使用されます。
* [GitHubディスカッション](https://github.com/shibacoinppc/shibacoin/discussions)は、Shibacoin Coreソフトウェアの開発、基礎となるプロトコル、およびSHIC資産に関連する計画されたおよび計画されていない機能について議論するために使用されます。
* [ShibacoinDev subreddit](https://www.reddit.com/r/shibacoindev/)

### バージョン戦略
バージョン番号は```major.minor.patch```のセマンティクスに従います。

### ブランチ
このリポジトリには3種類のブランチがあります：

- **master:** 安定版、最新の*major.minor*リリースの最新バージョンを含みます。
- **maintenance:** 安定版、以前のリリースの最新バージョンを含み、まだ積極的にメンテナンスされています。形式：```<version>-maint```
- **development:** 不安定版、計画されたリリースの新しいコードを含みます。形式：```<version>-dev```

*Masterおよびmaintenanceブランチはリリースによってのみ変更可能です。計画された*
*リリースには常に開発ブランチがあり、プルリクエストはそれらに対して提出されるべきです。メンテナンスブランチは**バグ修正のみ**のために存在します。新しい機能は、最も高いバージョンの開発ブランチに対して提出してください。*

## 貢献 🤝

このソフトウェアにバグを見つけたり、問題が発生した場合は、[問題システム](https://github.com/shibacoinppc/shibacoin/issues/new?assignees=&labels=bug&template=bug_report.md&title=%5Bbug%5D+)を使用して報告してください。

Shibacoin Coreの開発に参加する方法については、[貢献ガイド](CONTRIBUTING.md)を参照してください。しばしば[助けを求めるトピック](https://github.com/shibacoinppc/shibacoin/labels/help%20wanted)があり、あなたの貢献が高い影響を与え、非常に感謝されます。

## コミュニティ 🐶️

さまざまなソーシャルメディアでコミュニティに参加できます。
何が起こっているのかを見て、人々と出会い、議論し、最新のミームを見つけ、Shibacoinについて学び、助けを提供したり求めたり、プロジェクトを共有するために。

訪れるべき場所のいくつかは次のとおりです：

* [Discord](https://discord.com/invite/h3PrtfcKVJ)

## よくある質問 ❓

Shibacoinに関する質問がありますか？答えはすでに[FAQ](doc/FAQ.md)またはディスカッションボードの[Q&Aセクション](https://github.com/shibacoinppc/shibacoin/discussions/categories/q-a)にあるかもしれません。

## ライセンス ⚖️
Shibacoin CoreはMITライセンスの条件の下でリリースされています。詳細については[COPYING](COPYING)を参照するか、[opensource.org](https://opensource.org/licenses/MIT)を参照してください。
