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
    [MenuItem("Assets/UTJ/FrameCapturer/MakePackage")]
    public static void MakePackage()
    {
        // FrameCapturer.unitypackage
        {
            string[] files = new string[]
            {
                "Assets/UTJ",
                "Assets/StreamingAssets/UTJ/FrameCapturer/License.text",
                "Assets/StreamingAssets/UTJ/FrameCapturer/OpenH264_BinaryLicense.txt",
            };
            AssetDatabase.ExportPackage(files, "FrameCapturer.unitypackage", ExportPackageOptions.Recurse);
        }

        // FAACSelfBuild.unitypackage
        {
            string[] files = new string[]
            {
                "Assets/StreamingAssets/UTJ/FrameCapturer/FAAC_SelfBuild.zip",
            };
            AssetDatabase.ExportPackage(files, "FAACSelfBuild.unitypackage", ExportPackageOptions.Recurse);
        }
    }

}
#endif // UNITY_EDITOR
