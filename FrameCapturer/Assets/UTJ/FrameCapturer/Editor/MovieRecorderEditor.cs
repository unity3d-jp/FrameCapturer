using System;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer
{
    [CustomEditor(typeof(MovieRecorder))]
    public class MovieRecorderEditor : RecorderBaseEditor
    {
        public virtual void VideoConfig()
        {
            var recorder = target as MovieRecorder;
            var so = serializedObject;

            EditorGUILayout.PropertyField(so.FindProperty("m_captureTarget"));
            if(recorder.captureTarget == MovieRecorder.CaptureTarget.RenderTexture)
            {
                EditorGUI.indentLevel++;
                EditorGUILayout.PropertyField(so.FindProperty("m_targetRT"));
                EditorGUI.indentLevel--;
            }

            ResolutionControl();
            EditorGUILayout.PropertyField(so.FindProperty("m_captureEveryNthFrame"));
        }


        public virtual void AudioConfig()
        {
        }

        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as MovieRecorder;
            var so = serializedObject;

            CommonConfig();

            EditorGUILayout.Space();

            var ec = recorder.encoderConfigs;
            if (ec.supportVideo && !ec.supportAudio)
            {
                VideoConfig();
            }
            else if (!ec.supportVideo && ec.supportAudio)
            {
                AudioConfig();
            }
            else if (ec.supportVideo && ec.supportAudio)
            {
                ec.captureVideo = EditorGUILayout.Toggle("Capture Video", ec.captureVideo);
                if(ec.captureVideo)
                {
                    EditorGUI.indentLevel++;
                    VideoConfig();
                    EditorGUI.indentLevel--;
                }
                EditorGUILayout.Space();

                ec.captureAudio = EditorGUILayout.Toggle("Capture Audio", ec.captureAudio);
                if (ec.captureAudio)
                {
                    EditorGUI.indentLevel++;
                    AudioConfig();
                    EditorGUI.indentLevel--;
                }
            }

            EditorGUILayout.Space();
            FramerateControl();
            EditorGUILayout.Space();
            RecordingControl();

            so.ApplyModifiedProperties();
        }
    }
}
