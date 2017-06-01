
# FrameCapturer
[English](https://translate.google.com/translate?sl=ja&tl=en&u=https://github.com/unity3d-jp/FrameCapturer/) (by Google Translate)

フレームバッファ、およびサウンドをキャプチャして画像や動画に出力する Unity 用のプラグインです。exr, png, gif, webm, mp4, wav, ogg, flac への出力に対応しています。動作環境は Unity 5.2 or later & Windows, Mac, Linux に対応しています。

使用するにはまずこのパッケージをプロジェクトにインポートしてください: [FrameCapturer.unitypackage](https://github.com/unity3d-jp/FrameCapturer/releases/download/20170531/FrameCapturer.unitypackage)   
(Linux の場合、プラグインはソースからビルドする必要があります。このリポジトリを clone した後 Plugin/ に移動して cmake を用いてビルドしてください)

---

## MovieRecorder

<img align="right" src = "https://cloud.githubusercontent.com/assets/1488611/26622197/86f42110-4624-11e7-970b-58f13a85ba6c.png">

出力例:  
![gif_example1](Screenshots/gif_example1.gif)  

## AudioRecorder

<img align="right" src = "https://cloud.githubusercontent.com/assets/1488611/26622199/87207922-4624-11e7-961c-bc7da891645a.png">

## GBufferRecorder
deferred shading の際に生成される G-Buffer および motion vector をキャプチャします。
主にコンポジットに使うことを想定しています。  
エンコーダの設定など多くの設定は MovieRecorder と共通で、"Capture Component" でキャプチャする G-Buffer のコンポーネントを選べるようになっています。  
コンポジットの場合主に exr か png のイメージシーケンスを使うことになると思われますが、WebM などの動画としても出力可能です。

<img align="right" src = "https://cloud.githubusercontent.com/assets/1488611/26622198/8719de50-4624-11e7-9989-cbb3b5024979.png">

出力例:  
![gif_example1](Screenshots/exr_example1.png)  


## License
[MIT](LICENSE.txt)
