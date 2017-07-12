using System;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer.Recorders
{
    class RecorderPackager : ScriptableObject {}

    [InitializeOnLoad]
    class RecorderPackagerInternal : ScriptableObject
    {
        const string k_PackageName = "FrameRecorders";
        const string k_WitnessClass = "Recorder";
        const string k_WitnessNamespace = "UnityEngine.FrameRecorder";

        static string m_PkgFile;
        static string m_ScriptFile;

        [MenuItem("Assets/FrameCapturer/Make Recorders Sub-Package")]
        static void GeneratePackage()
        {
            string[] files = new string[]
            {
                "Assets/UTJ/FrameCapturer/Recorders",
            };
            var path = GetScriptPath() + "/../";
            var destFile = Path.Combine(path, k_PackageName + ".unitypackage");
            AssetDatabase.ExportPackage(files, destFile, ExportPackageOptions.Recurse);
            Debug.Log("Generated package: " + destFile);
        }
        
        static RecorderPackagerInternal() // auto extracts
        {
            var havePostProcessing = AppDomain.CurrentDomain.GetAssemblies()
                .Any(x => x.GetTypes().Any(y => y.Name == k_WitnessClass && y.Namespace == k_WitnessNamespace));

            if (havePostProcessing)
            {
                var path = GetScriptPath();
                m_PkgFile = Path.Combine( path,  "../" + k_PackageName + ".unityPackage" );
                m_ScriptFile = Path.Combine(path, "BaseFCRecorderSettings.cs");
                if ( File.Exists(m_PkgFile) && 
                    (!File.Exists(m_ScriptFile) || File.GetLastWriteTime(m_PkgFile) > File.GetLastWriteTime(m_ScriptFile)))
                {
                    Debug.Log("PostProcessing asset detected - Importing FrameCapturer's Recorders");
                    AssetDatabase.importPackageCompleted += AssetDatabase_importPackageCompleted;
                    AssetDatabase.importPackageFailed += AssetDatabase_importPackageFailed;
                    AssetDatabase.importPackageCancelled += RemovePackageImportCallbacks;
                    AssetDatabase.ImportPackage(m_PkgFile, false);
                }
            }
        }
        
        static void AssetDatabase_importPackageCompleted(string packageName)
        {
            if (packageName == k_PackageName)
            {
                File.SetLastWriteTime(m_ScriptFile, File.GetLastWriteTime(m_PkgFile));
                RemovePackageImportCallbacks(k_PackageName);
            }
        }

        static void AssetDatabase_importPackageFailed(string packageName, string errorMessage)
        {
            if (packageName == k_PackageName)
            {
                Debug.LogError("Failed to import " + k_PackageName + ": " + errorMessage);
                RemovePackageImportCallbacks(k_PackageName);
            }
        }

        static void RemovePackageImportCallbacks(string packageName)
        {
            AssetDatabase.importPackageCompleted -= AssetDatabase_importPackageCompleted;
            AssetDatabase.importPackageCancelled -= RemovePackageImportCallbacks;
            AssetDatabase.importPackageFailed -= AssetDatabase_importPackageFailed;
        }

        static string GetScriptPath()
        {
            ScriptableObject dummy = ScriptableObject.CreateInstance<RecorderPackager>();
            string path = Application.dataPath + AssetDatabase.GetAssetPath(
                MonoScript.FromScriptableObject(dummy)).Substring("Assets".Length);

            path = path.Substring(0, path.LastIndexOf('/')-1);
            path = path.Substring(0, path.LastIndexOf('/'));
            return Path.Combine(path, "Recorders");
        }        
    }

}