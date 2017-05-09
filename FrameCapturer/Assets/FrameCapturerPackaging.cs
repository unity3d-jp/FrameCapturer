#if UNITY_EDITOR
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
                "Assets/StreamingAssets/UTJ",
                "Assets/Unity",
            };
            AssetDatabase.ExportPackage(files, "FrameCapturer.unitypackage", ExportPackageOptions.Recurse);
        }
    }

}
#endif // UNITY_EDITOR
