using System;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer
{
    [CustomEditor(typeof(AudioRecorder))]
    public class AudioRecorderEditor : Editor
    {
        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as AudioRecorder;
            var so = serializedObject;

            EditorGUILayout.PropertyField(so.FindProperty("m_outputDir"), true);
            EditorGUILayout.PropertyField(so.FindProperty("m_encoderConfigs"), true);

            EditorGUILayout.Space();

            // capture control
            EditorGUILayout.PropertyField(so.FindProperty("m_captureControl"), true);
            EditorGUI.indentLevel++;
            if (recorder.captureControl == AudioRecorder.CaptureControl.SpecifiedRange)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_startFrame"), true);
                EditorGUILayout.PropertyField(so.FindProperty("m_endFrame"), true);
            }
            else if (recorder.captureControl == AudioRecorder.CaptureControl.Manual)
            {
                EditorGUILayout.Space();
                if (!recorder.isRecording)
                {
                    if (GUILayout.Button("Begin Recording"))
                    {
                        recorder.BeginRecording();
                    }
                }
                else
                {
                    if (GUILayout.Button("End Recording"))
                    {
                        recorder.EndRecording();
                    }
                }
            }
            EditorGUI.indentLevel--;

            so.ApplyModifiedProperties();
        }
    }
}
