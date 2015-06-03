# FrameCapturer
[English](README_EN.md)  
  
フレームバッファの内容をキャプチャして画像や動画に出力する Unity 用のプラグインです。現在アニメ gif と exr への出力に対応しています。

## Gif Capturer
以下のような機能を備えています。
- 直近 N フレームをメモリに残し、後で指定部分だけをファイルに出力 (=ShadowPlay や PS4 と似た録画方法)
- 非同期＆並列エンコーディング (メインスレッドをブロックしない)
- 簡単な in-game プレビューア＆エディタ
- D3D9, D3D11, OpenGL と x86, x86-64 の組み合わせに対応 (ただし Windows でのみ動作を確認)

以下の動画を見ると何ができるのか大体わかると思います。  
[![FrameCapturer: GifRecorder](http://img.youtube.com/vi/VRmVIzhxewI/0.jpg)](http://www.youtube.com/watch?v=VRmVIzhxewI)  

以下はこのプラグインで出力されたアニメ gif の例です。  
![example1](Screenshots/gif_example1.gif)  

### 使い方
1. [このパッケージ](https://github.com/unity3d-jp/FrameCapturer/blob/master/Packages/GifRecoder.unitypackage?raw=true)をインポート
2. 録画したいカメラに GifCapturer コンポーネントを追加
3. uGUI オブジェクト GifCapturerHUD.prefab をどこかに配置し、それの capturer に 2 で追加したコンポーネントを設定

3 は必須ではありませんが、GifCapturer には録画の on/off 切り替えやファイルへの書き出しなどをコントロールするための GUI やスクリプトが必要になります。GifCapturerHUD.prefab を改造して独自の GUI を作るのもいいでしょう。
  
## Exr Capturer  
Exr は主に映像業界で使われるフォーマットで、G-Buffer やマテリアル ID など、コンポジットに使うためのデータを保持するのに用いられます。  
現状一応 G-Buffer を書き出せるものの、まだ開発中で動作が怪しいです。  
