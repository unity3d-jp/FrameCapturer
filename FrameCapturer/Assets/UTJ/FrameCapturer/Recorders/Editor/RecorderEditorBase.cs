using UnityEditor;
using UnityEditor.FrameRecorder;
using UnityEngine;

namespace UTJ.FrameCapturer.Recorders
{
    public class RecorderEditorBase : RecorderEditor
    {
        public string m_BaseFileName;
        public string m_DestinationPath;

        SerializedProperty m_DestPathProp;
        SerializedProperty m_BaseFileNameProp;

        [MenuItem("Window/Recorder/Video...")]
        static void ShowRecorderWindow()
        {
            RecorderWindow.ShowAndPreselectCategory("Video");
        }

        protected override void OnEnable()
        {
            base.OnEnable();
            if (target != null)
            {
                m_DestPathProp = serializedObject.FindProperty<BaseFCRecorderSettings>(x => x.m_DestinationPath);
                m_BaseFileNameProp = serializedObject.FindProperty<BaseFCRecorderSettings>(x => x.m_BaseFileName);
            }
        }

        protected override void OnOutputGui()
        {
            GUILayout.BeginHorizontal();
            EditorGUILayout.LabelField("Directory");
            m_DestPathProp.stringValue = EditorGUILayout.TextField(m_DestPathProp.stringValue);
            if (GUILayout.Button("...", GUILayout.Width(30)))
                m_DestPathProp.stringValue = EditorUtility.OpenFolderPanel( "Select output location", m_DestPathProp.stringValue, "");
            GUILayout.EndHorizontal();

            EditorGUILayout.PropertyField(m_BaseFileNameProp, new GUIContent("File name"));

            base.OnOutputGui();
        }
    }
}
