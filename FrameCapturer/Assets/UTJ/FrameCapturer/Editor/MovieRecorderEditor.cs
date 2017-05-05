using System;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer
{
    [CustomEditor(typeof(MovieRecorder))]
    public class MovieRecorderEditor : Editor
    {
        protected bool m_videoAdvanced = false;
        protected bool m_audioAdvanced = false;

        public virtual void CommonConfig()
        {
            var recorder = target as MovieRecorder;
            var so = serializedObject;
            EditorGUILayout.PropertyField(so.FindProperty("m_outputDir"), true);
            recorder.format = (MovieRecorderContext.Type)EditorGUILayout.EnumPopup("Format", recorder.format);
        }

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

            EditorGUILayout.PropertyField(so.FindProperty("m_resolutionWidth"));
            EditorGUILayout.PropertyField(so.FindProperty("m_videoBitrate"));
            EditorGUILayout.PropertyField(so.FindProperty("m_frameRateMode"));
            if(recorder.m_frameRateMode == MovieRecorder.FrameRateMode.Constant)
            {
                EditorGUI.indentLevel++;
                EditorGUILayout.PropertyField(so.FindProperty("m_targetFramerate"));
                EditorGUILayout.PropertyField(so.FindProperty("m_fixDeltaTime"));
                EditorGUI.indentLevel--;
            }
            EditorGUILayout.PropertyField(so.FindProperty("m_captureEveryNthFrame"));

            m_videoAdvanced = EditorGUILayout.Toggle("Advanced Settings", m_videoAdvanced);
            if(m_videoAdvanced)
            {
                EditorGUI.indentLevel++;
                VideoConfigAdvanced();
                EditorGUI.indentLevel--;
            }
        }

        public virtual void VideoConfigAdvanced()
        {
        }


        public virtual void AudioConfig()
        {
            var so = serializedObject;
            EditorGUILayout.PropertyField(so.FindProperty("m_audioBitrate"));

            m_audioAdvanced = EditorGUILayout.Toggle("Advanced Settings", m_audioAdvanced);
            if (m_audioAdvanced)
            {
                EditorGUI.indentLevel++;
                AudioConfigAdvanced();
                EditorGUI.indentLevel--;
            }
        }

        public virtual void AudioConfigAdvanced()
        {
        }


        public virtual void RecordingControl()
        {
            var recorder = target as MovieRecorder;

            EditorGUILayout.Space();

            if (!recorder.isRecording)
            {
                if(GUILayout.Button("Begin Recording"))
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


        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as MovieRecorder;
            var so = serializedObject;

            CommonConfig();

            EditorGUILayout.Space();

            EditorGUILayout.PropertyField(so.FindProperty("m_captureVideo"));
            if (recorder.m_captureVideo)
            {
                EditorGUI.indentLevel++;
                VideoConfig();
                EditorGUI.indentLevel--;
            }

            EditorGUILayout.Space();

            EditorGUILayout.PropertyField(so.FindProperty("m_captureAudio"));
            if (recorder.m_captureAudio)
            {
                EditorGUI.indentLevel++;
                AudioConfig();
                EditorGUI.indentLevel--;
            }

            so.ApplyModifiedProperties();

            RecordingControl();
        }
    }
}
