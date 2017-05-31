
# FrameCapturer
[English](https://translate.google.com/translate?sl=ja&tl=en&u=https://github.com/unity3d-jp/FrameCapturer/) (by Google Translate)

フレームバッファ、およびサウンドをキャプチャして画像や動画に出力する Unity 用のプラグインです。exr, png, gif, webm, mp4, wav, ogg, flac への出力に対応しています。動作環境は Unity 5.2 or later & Windows, Mac, Linux に対応しています。

使用するにはまずこのパッケージをプロジェクトにインポートしてください: [FrameCapturer.unitypackage](https://github.com/unity3d-jp/FrameCapturer/releases/download/20170531/FrameCapturer.unitypackage)   
(Linux の場合、プラグインはソースからビルドする必要があります。このリポジトリを clone した後 Plugin/ に移動して cmake を用いてビルドしてください)

以下は各コンポーネントの説明になります。
- [Gif Recorder](#gif-recorder)
- [MP4 Recorder](#mp4-recorder)
- [Exr Recorder](#exr-recorder)
- [Png Recorder](#png-recorder)

---

## Gif Recorder
ゲーム画面をキャプチャしてアニメ gif としてエクスポートします。
常時録画して後で面白いカットを切り出してファイルに出力する、というような使い方を想定しています。  
以下のような機能を備えています。
- 直近 N フレームをメモリに残し、後で指定部分だけをファイルに出力
- 非同期＆並列エンコーディング (メインスレッドをブロックしない)
- 簡単な in-game プレビューア＆エディタ
- 録画結果をゲーム内から直接 Twitter へ投稿可能

以下の動画を見ると何ができるのか大体わかると思います。  
[![FrameCapturer: GifRecorder](http://img.youtube.com/vi/VRmVIzhxewI/0.jpg)](http://www.youtube.com/watch?v=VRmVIzhxewI)  
以下はこのプラグインで出力されたアニメ gif の例です。  
![gif_example1](Screenshots/gif_example1.gif)  

##### 使い方
1. 録画したいカメラに GifRecorder コンポーネントを追加
2. GUI オブジェクトの MovieRecorderUI.prefab などをどこかに配置
3. GUI の recorder に 1 で追加したコンポーネントを設定

GUI は必須ではありませんが、GifRecorder には録画の on/off 切り替えやファイルへの書き出しなどをコントロールするための GUI やスクリプトが必要になります。  
パッケージには 2 種類の GUI が用意されています。
MovieRecorderUI.prefab と MovieRecorderEditorUI.prefab がそれで、前者は録画のオンオフのみをコントロールするもので、後者は録画結果のシークや簡易編集機能を備えています。  
これらは必要最小限の機能と見た目を提供するもので、実際のゲームに使うにはもっといい見た目にカスタマイズするか自作するかが必要があるかもしれません。

<img align="right" src="Screenshots/GifRecorder.png">
以下はコンポーネントの各パラメータの解説です。
- Output Dir  
  ファイルの出力先ディレクトリです。Root が PersistentDataPath だと Application.persistentDataPath 基準になります (デフォルト＆推奨設定)。
  Current Directory だと文字通りカレントディレクトリ (エディタの場合プロジェクトのディレクトリ、ビルド済みの場合は大抵実行ファイルと同じディレクトリ) 基準で、エディタの場合はこちらの方が何かと便利ではあります。
- Num Colors  
  gif の色数です。最大 256 で、少なくするほど画質が悪くなる代わりに容量が小さくなります。
- Frame Rate Mode  
  gif 側のフレームレートの設定で、処理落ちなどでフレームレートが低下したときの録画結果に影響します。  
  Variable だと処理落ちに応じて gif もコマ落ちするようになります。Constant だと gif 側のフレームレートは常に一定になります。
  言い方を変えると、Variable だとゲーム内の経過時間と gif の再生時間は同じになります。
  Constant だと再生はスムースになりますが、ゲーム内の経過時間と gif の再生時間にズレが生じます。  
  音声も録音して映像とタイミングを一致させたいような場合 Variable で録画する必要があります。
- Framerate  
  Frame Rate Mode が Constant の時、gif のフレームレートはこの数値に保たれます。Variable の時は無視されます。
- Capture Every Nth Frame  
  1 だと全フレーム録画、2 だと 2 フレームに一回録画します。60 FPS のゲームで 30 FPS の録画を行いたい時 2 に設定します。
- Keyframe  
  パレットを更新する間隔です。基本的には小さくすると画質が良くなる代わりに容量が増えます。  
  gif はインデックスカラーの画像データですが、アニメ gif ではパレットを複数フレームの間使い回すことができます。
  長期間使い回せばデータ量は減らせるものの、画面全体の色の傾向が変化していく場合画質がひどく劣化することになります。  
  Keyframe はこのパレット使い回しをコントロールするもので、Keyframe フレーム数が経過するたびにパレットを更新します。

RenderTexture の録画を行う GifOffscreenRecorder というのも用意されています。Target にRenderTexture を指定する以外は使い方は GifRecorder と同じです。

録画した gif をゲーム内から直接 Twitter へ投稿することもできます。
Twitter 投稿機能は、[TweetMedia](https://github.com/unity3d-jp/TweetMedia) によって実現されており、詳しくはそちらをご参照ください。  
TweetWithFile.prefab はこちらのパッケージにしかない prefab で、録画した gif や mp4 を添付する機能が追加された Tweet 用 GUI になっています。
これの TweetMediaAttachFile コンポーネントの RecorderUI に 録画 GUI (MovieRecorderUI.prefab など) を設定し、Screenshot にチェックを入れて Tweet すると録画結果と共に投稿されます。

録画解像度はかなり小さめ (横 300 pixel 程度) を推奨しています。
gif のエンコードはとても遅い上、解像度に比例してすごい勢いで負荷が上がっていくため、等倍解像度の録画をリアルタイムで行うのは絶望的です。  

また、Gif は仕様でフレーム間のデルタ時間は単位がセンチ秒 (10ms) になっています。
このため、再生のフレームレートは多くても 100FPS, 50FPS, 33FPS, 25FPS... になってしまい、60FPS は正確に表現することはできません。
この点は気に留めておいたほうがいいかもしれません。
(実際のところ大抵のソフトウェアは 100FPS ではなく 60FPS だったり 30FPS だったりで再生するようですが)


## MP4 Recorder
ゲーム画面をキャプチャして mp4 動画で出力します。

使用手順は大体 GifRecorder と同じで、録画したいカメラに MP4Recorder を追加し、GUI を配置して recorder を設定するだけです。RenderTexture を録画する MP4OffscreenRecorder が用意されているのも同様です。  
GUI も GifRecorder と同じくパッケージに付属の 2 種 (MovieRecorderUI.prefab, MovieRecorderEditorUI.prefab) を使えますが、
現状実用に足るのは MovieRecorderUI.prefab だけです。
MP4Recorder はシークや編集には未対応なため、MovieRecorderEditorUI.prefab を使ってもほとんどのボタンは機能しません。  
Twitter 投稿は GifRecorder と全く同じ手順で機能します。

<img align="right" src="Screenshots/MP4Recorder.png">
以下はコンポーネントの各パラメータの解説です。
- Capture Video
- Capture Audio  
  ビデオやオーディオのキャプチャのオンオフを指定します。mp4 の場合オーディオのみの出力も可能です。
- Frame Rate Mode  
- Framerate  
- Capture Every Nth Frame  
  これらは Gif Recorder での説明がそのまま当てはまりますが、mp4 の場合重要な注意事項があります。**録音もする場合、Frame Rate Mode は Variable にすべき** ということです。  
  Constant だとだんだん音と映像がズレていきます。これは Constant だとゲーム時間と再生時間の同期を諦めてフレームレートを一定に保つためです。
- Video Bitrate
- Audio Bitrate  
  ビットレート指定です。画質や音質を決定するパラメータになります。  

## Exr Recorder  
Exr は主に映像業界で使われる画像フォーマットで、float や half のピクセルデータで構成された画像、いわゆる HDR 画像を保持できます。
ExrRecorder および PngRecorder は映像制作用のツールとして作られており、
G-Buffer やマテリアル ID などを書き出してコンポジットに使う、といった使い方を目的としています。  
FrameCapturer_EXR.dll はそれなりにでかいため (3MB 超)、使わない場合は取り除いたほうがいいかもしれません。

Exr のエクスポートは非常に遅く、リアルタイムで行うのは困難であるため、デルタタイムを固定して事前に指定しておいた範囲のフレームを書き出す、という使い方を前提としています。
パッケージにはデルタタイムを固定する簡単なスクリプトが付属しています。(UTJ / Misc / FixDeltaTime)

<img align="right" src="Screenshots/ExrRecorder.png">
##### G-Buffer & フレームバッファのキャプチャ
1. 録画したいカメラに ExrRecorder コンポーネントを追加
2. キャプチャしたい要素 (G-Buffer, FrameBuffer)、キャプチャ開始 / 終了フレームを設定
3. Play

プレイ開始後、指定フレームの範囲に来ると自動的にフレームバッファや G-Buffer の内容をエクスポートします。
ファイル構成やレイヤー名は現状決め打ちになっています。
変更したい場合、ExrRecorder.cs の DoExport() 内のエクスポート部分を書き換えることで対応可能です。

<img align="right" src="Screenshots/ExrOffscreenRecorder.png">
##### RenderTexture のキャプチャ
RenderTexture の内容をキャプチャするバージョンも用意されています。

1. 録画したいカメラに ExrOffscreenCapturer コンポーネントを追加  
2. キャプチャしたい RenderTexture を Targets に設定  
3. キャプチャ開始 / 終了フレームなどを設定  
4. Play  

大体 ExrRecorder と同じで、Targets に指定した RenderTexture の内容が .exr に書き出されるようになっています。
Target は複数指定可能です。

出力例:  
![exr_example1](Screenshots/exr_example1.png)


## Png Recorder
ゲーム画面を連番 PNG でキャプチャします。  
EXR と同様、こちらも想定している用途は映像のコンポジットです。
使い方も ExrRecorder, ExrOffscreenRecorder と全く同じで、指定したフレーム間の G-Buffer やフレームバッファの各要素を連番 png で書き出せるようになっています。

png は 16 bit 整数カラーをサポートしており、half や float の RenderTexture は 16 bit モードで書き出します。
16 bit 整数カラーの場合、0.0 - 1.0 -> 0 - 255 の変換ルールはそのまま、1.0 より大きな色が 256 以上になる、という挙動になります。
つまり **0.0 - 1.0 の範囲しか扱わない場合、出力される情報量は 8 bit カラーと同等になります**。ご注意ください。  
以上のことからコンポジット用途としては exr の方が望ましいのですが、exr はインポート/エクスポートが遅く、png で精度が足りる場合はそちらが使われるケースもあるそうで、PngRecorder も用意されています。

![PngRecorder](Screenshots/PngRecorder.png) ![PngOffscreenRecorder](Screenshots/PngOffscreenRecorder.png)

## License
[MIT](LICENSE.txt)
