using System;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;

namespace UnityEditor.Recorder.FrameRecorder.Example
{
    [CustomEditor(typeof(PNGRecorderSettings))]
    [RecorderEditor(typeof(PNGRecorder))]
    public class PngRecorderSettingsEditor : DefaultImageRecorderSettingsEditor
    {
        public override Vector2 minSize
        {
            get { return new Vector2(400, 340); }
        }

        protected override void OnOutputGui()
        {
            var settingsObj = serializedObject.targetObject as PNGRecorderSettings;

            GUILayout.BeginHorizontal();
            m_LayoutHelper.AddPropertyLabel("Directory");
            settingsObj.m_DestinationPath = EditorGUILayout.TextField(settingsObj.m_DestinationPath);
            if (GUILayout.Button("...", GUILayout.Width(30)))
                settingsObj.m_DestinationPath = EditorUtility.OpenFolderPanel(m_LayoutHelper + "Select output location", settingsObj.m_DestinationPath, "");
            GUILayout.EndHorizontal();

            m_LayoutHelper.AddStringProperty("File name", serializedObject, () => settingsObj.m_BaseFileName);
        }
    }
}
