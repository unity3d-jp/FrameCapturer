
# FrameCapturer
This plugin allow you to capture framebuffer, G-buffer and audio and output to file. Supported file formats are exr, png, gif, webm, mp4, wav, ogg and flac. You may also interested in  [FrameRecorder](https://github.com/Unity-Technologies/GenericFrameRecorder).

Supported platforms are Windows and Mac. (Also confirmed to work on Linux, but you need to build plugin from source)

## Usage

1. Import this package to your project: [FrameCapturer.unitypackage](https://github.com/unity3d-jp/FrameCapturer/releases/download/20170715/FrameCapturer.unitypackage)
2. Add recorder component to camera (Add Component -> UTJ -> FrameCapturer -> * Recorder)
3. Setup recorder settings and capture

There are 3 recorder components: MovieRecorder, GBufferRecorder and AudioRecorder.
- MovieRecorder: capture framebuffer and audio.  
- GBufferRecorder: capture G-buffer (depth buffer, albedo buffer, normal buffer, etc). This is useful for composite process in movie making. Rendering path must be deferred to use this recorder.  
- AudioRecorder: just capture audio. This is also useful for movie making.   

## Limitations
Currently MP4 recordering is available only on Windows.

## Example Outputs:  
![gif_example1](Screenshots/gif_example1.gif)  
![gif_example1](Screenshots/exr_example1.png)  


## License
[MIT](LICENSE.txt)
