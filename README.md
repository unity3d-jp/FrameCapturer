# FrameCapturer
[English](README_EN.md)  

フレームバッファの内容をキャプチャして画像や動画に出力する Unity 用のプラグインです。現在アニメ gif と exr への出力に対応しています。

## Gif Capturer
ゲーム画面をキャプチャしてアニメ gif としてエクスポートします。
常時録画して後で面白いカットを切り出してファイルに出力する、というような使い方を想定しています。録画解像度はかなり小さめ (横 300 pixel 程度) を推奨しています。解像度に比例してすごい勢いで処理が重くなり、等倍での録画ともなるとリアルタイム処理は絶望的です。

以下のような機能を備えています。
- 直近 N フレームをメモリに残し、後で指定部分だけをファイルに出力 (=ShadowPlay や PS4 と似た録画方法)
- 非同期＆並列エンコーディング (メインスレッドをブロックしない)
- 簡単な in-game プレビューア＆エディタ
- D3D9, D3D11, OpenGL と x86, x86-64 の組み合わせに対応 (ただし Windows でのみ動作を確認)
- Twitter へ録画結果をゲーム内から直接投稿

以下の動画を見ると何ができるのか大体わかると思います。  
[![FrameCapturer: GifRecorder](http://img.youtube.com/vi/VRmVIzhxewI/0.jpg)](http://www.youtube.com/watch?v=VRmVIzhxewI)  

以下はこのプラグインで出力されたアニメ gif の例です。  
![gif_example1](Screenshots/gif_example1.gif)  

### 使い方
1. [このパッケージ](https://github.com/unity3d-jp/FrameCapturer/blob/master/Packages/FrameCapturer.unitypackage?raw=true)をインポート
2. 録画したいカメラに GifCapturer コンポーネントを追加
3. uGUI オブジェクト GifCapturerHUD.prefab をどこかに配置し、それの capturer に 2 で追加したコンポーネントを設定

3 は必須ではありませんが、GifCapturer には録画の on/off 切り替えやファイルへの書き出しなどをコントロールするための GUI やスクリプトが必要になります。
GifCapturerHUD.prefab は機能はともかく見た目は必要最小限のため、これを使う場合も独自に改良した方がいいでしょう。

Twitter への投稿機能は、[TweetMedia](https://github.com/unity3d-jp/TweetMedia) によって実現されています。詳しくはそちらをご参照ください。  
TweetScreenshot.prefab はこちらのパッケージにしかない prefab で、録画した gif を添付する機能が追加された Tweet 用 GUI になっています。

## Exr Capturer  
Exr は主に映像業界で使われる画像フォーマットで、float や half のピクセルデータで構成された画像、いわゆる HDR 画像を保持できます。  
ExrCapturer は主に映像向けの用途を想定したもので、G-Buffer やマテリアル ID などを書き出し、コンポジットに使うことを目的としています。現状 G-Buffer の書き出しと、任意の RenderTexture の書き出しに対応しています。  
Exr エクスポートは非常に重く、リアルタイムで行うのは厳しい処理であるため、デルタタイムを固定して事前に指定しておいた範囲のフレームを書き出す、という使い方を想定しています。  

![exr_example1](Screenshots/exr_example1.png)  

### 使い方

#### G-Buffer & フレームバッファのキャプチャ (ExrCapturer)
1. [このパッケージ](https://github.com/unity3d-jp/FrameCapturer/blob/master/Packages/FrameCapturer_Exr.unitypackage?raw=true)をインポート  
  (Exr は用途が特殊なため、専用パッケージになっています。また、64 bit 版のみの提供となります)
2. 録画したいカメラに ExrCapturer コンポーネントを追加
3. キャプチャしたい要素 (capture_framebuffer, capture_gbuffer)、録画開始 / 終了フレームなどを設定し、Play  

![ExrCapturer](Screenshots/ExrCapturer.png)  

depth_format は、depth を 16 bit で書き出す場合 Half、32 bit で書き出す場合 Float を指定します。
通常 depth は元データが 24 bit のため Float が望ましいのですが、編集ソフトが Float 画像に対応していない場合などに Half で書き出す必要性が生じます。

---

#### RenderTexture のキャプチャ (ExrOffscreenCapturer)
1. パッケージをインポート (同上)
2. 録画したいカメラに ExrOffscreenCapturer コンポーネントを追加
3. RenderTexture を用意し、カメラと ExrOffscreenCapturer に設定

大体 ExrCapturer と同じですが、3 の RenderTexture を ExrOffscreenCapturer に設定する手順がやや複雑です。  
exr はピクセルの各チャンネル (RGBA) を個別にレイヤーとして持つ構造になっています。
これに合わせ、ExrOffscreenCapturer もチャンネル単位で書き出す要素を指定するようになっています。

Targets は 1 要素が 1 つの RenderTexture に対応します。
書き出したい数だけ Size を増やし、それぞれ Target に RenderTexture を指定します。  
Channnels にチャンネル情報を指定します。
Name は他とかぶっていなければなんでもよく、これがそのまま編集ツールで見えるレイヤー名になります。
Channel は何番目のチャンネルかを指定するもので、0,1,2,3 がそれぞれ R,G,B,A に対応します。  

例: 1 つの RenderTexture の RGBA 各要素を書き出したい場合、以下のような設定になります。(ExrOffscreenCapturerExample.unity からの抜粋)  
![ExrOffscreenCapturer](Screenshots/ExrOffscreenCapturer.png)  
