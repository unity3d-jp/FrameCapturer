using System;
using UnityEditor;
using UnityEngine.Recorder.FrameRecorder;
using UnityEngine;

namespace UnityEditor.Recorder.FrameRecorder
{
    [CustomEditor(typeof(GIFRecorderSettings))]
    [RecorderEditor(typeof(GIFRecorder))]
    public class GIFRecorderSettingsEditor : DefaultImageRecorderSettingsEditor
    {
        protected override void OnEncodingGui()
        {
            base.OnEncodingGui();
            //m_LayoutHelper.AddIntProperty("Bitrate", serializedObject, () => (target as MP4RecorderSettings).m_VideoBitrate);
        }

        protected override void OnOutputGui()
        {
            var settingsObj = serializedObject.targetObject as GIFRecorderSettings;

            GUILayout.BeginHorizontal();
            m_LayoutHelper.AddPropertyLabel("Directory");
            settingsObj.m_DestinationPath = EditorGUILayout.TextField(settingsObj.m_DestinationPath);
            if (GUILayout.Button("...", GUILayout.Width(30)))
                settingsObj.m_DestinationPath = EditorUtility.OpenFolderPanel(m_LayoutHelper + "Select output location", settingsObj.m_DestinationPath, "");
            GUILayout.EndHorizontal();

            m_LayoutHelper.AddStringProperty("File name", serializedObject, () => settingsObj.m_BaseFileName);
        }

        public override Vector2 minSize
        {
            get { return new Vector2(400, 370); }
        }
    }
}
