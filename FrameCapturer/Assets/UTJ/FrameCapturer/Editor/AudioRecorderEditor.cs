using System;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer
{
    [CustomEditor(typeof(AudioRecorder))]
    public class AudioRecorderEditor : RecorderBaseEditor
    {
        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as AudioRecorder;
            var so = serializedObject;

            CommonConfig();

            EditorGUILayout.Space();

            RecordingControl();

            so.ApplyModifiedProperties();
        }
    }
}
