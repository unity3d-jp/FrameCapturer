#if UNITY_EDITOR
using UnityEditor;


public class FrameCapturerPackaging
{
    [MenuItem("Assets/Make FrameCapturer.unitypackage")]
    public static void MakePackage()
    {
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
