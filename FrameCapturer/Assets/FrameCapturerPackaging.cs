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
    [MenuItem("Assets/FrameCapturer/MakePackage")]
    public static void MakePackage()
    {
        string[] files = new string[]
        {
"Assets/FrameCapturer",
"Assets/TweetMedia",
"Assets/Plugins",
        };
        AssetDatabase.ExportPackage(files, "FrameCapturer.unitypackage", ExportPackageOptions.Recurse);
    }


    [MenuItem("Assets/FrameCapturer/MakePackage (EXR)")]
    public static void MakePackage_EXR()
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
