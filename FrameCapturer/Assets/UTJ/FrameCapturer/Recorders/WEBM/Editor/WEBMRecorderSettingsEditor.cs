using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer.Recorders
{
    [CustomEditor(typeof(WEBMRecorderSettings))]
    public class WEBMRecorderSettingsEditor : RecorderEditorBase
    {
        protected override void OnEncodingGroupGui()
        {
            if (EditorGUILayout.PropertyField(serializedObject.FindProperty("m_WebmEncoderSettings"), new GUIContent("Encoding"), true))
            {
                EditorGUI.indentLevel++;
                base.OnEncodingGui();
                EditorGUI.indentLevel--;
            }
        }
    }
}
