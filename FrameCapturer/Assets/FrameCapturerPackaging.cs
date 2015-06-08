#if UNITY_EDITOR
using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEditor;


public class FrameCapturerPackaging
{
    [MenuItem("Assets/FrameCapturer/ExportGif")]
    public static void ExportPackage_Gif()
    {
        string[] files = new string[]
        {
"Assets/FrameCapturer/Example/Animation/Camera.anim",
"Assets/FrameCapturer/Example/Animation/CameraRig.controller",
"Assets/FrameCapturer/Example/Textures/TestRenderTarget.renderTexture",
"Assets/FrameCapturer/Prefabs/GifCapturerHUD.prefab",
"Assets/FrameCapturer/Scripts/FrameCapturer.cs",
"Assets/FrameCapturer/Scripts/MovieCapturer.cs",
"Assets/FrameCapturer/Scripts/MovieCapturerHUD.cs",
"Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader",

"Assets/Plugins/x86_64/FrameCapturer.dll",
"Assets/Plugins/x86/FrameCapturer.dll",

"Assets/FrameCapturer/Example/GifCapturerExample.unity",
"Assets/FrameCapturer/Example/GifOffscreenCapturerExample.unity",
"Assets/FrameCapturer/Scripts/GifCapturer.cs",
"Assets/FrameCapturer/Scripts/GifOffscreenCapturer.cs",

// TweetMedia
"Assets/TweetMedia/Examples/Example.unity",
"Assets/TweetMedia/Prefabs/TweetMedia.prefab",
"Assets/TweetMedia/Scripts/TweetMedia.cs",
"Assets/TweetMedia/Scripts/TweetMediaGUI.cs",
"Assets/TweetMedia/Scripts/TweetMediaPlugin.cs",
"Assets/Plugins/x86_64/TweetMedia.dll",
"Assets/Plugins/x86/TweetMedia.dll",
// TweetMedia Ext
"Assets/TweetMedia/Prefabs/TweetScreenshot.prefab",
"Assets/TweetMedia/Scripts/TMExtAttachScreenshot.cs",
        };
        AssetDatabase.ExportPackage(files, "FrameCapturer.unitypackage");
    }


    [MenuItem("Assets/FrameCapturer/ExportComplete")]
    public static void ExportPackage_Complete()
    {
        string[] files = new string[]
        {
"Assets/FrameCapturer/Example/Animation/Camera.anim",
"Assets/FrameCapturer/Example/Animation/CameraRig.controller",
"Assets/FrameCapturer/Example/Textures/TestRenderTarget.renderTexture",
"Assets/FrameCapturer/Prefabs/GifCapturerHUD.prefab",
"Assets/FrameCapturer/Scripts/FrameCapturer.cs",
"Assets/FrameCapturer/Scripts/MovieCapturer.cs",
"Assets/FrameCapturer/Scripts/MovieCapturerHUD.cs",
"Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader",

"Assets/Plugins/x86_64/FrameCapturer.dll",

"Assets/FrameCapturer/Example/GifCapturerExample.unity",
"Assets/FrameCapturer/Example/GifOffscreenCapturerExample.unity",
"Assets/FrameCapturer/Scripts/GifCapturer.cs",
"Assets/FrameCapturer/Scripts/GifOffscreenCapturer.cs",

"Assets/FrameCapturer/Example/ExrCapturerExample.unity",
"Assets/FrameCapturer/Example/ExrOffscreenCapturerExample.unity",
"Assets/FrameCapturer/Scripts/ExrCapturer.cs",
"Assets/FrameCapturer/Scripts/ExrOffscreenCapturer.cs",
        };
        AssetDatabase.ExportPackage(files, "FrameCapturer_Exr.unitypackage");
    }

}
#endif // UNITY_EDITOR
