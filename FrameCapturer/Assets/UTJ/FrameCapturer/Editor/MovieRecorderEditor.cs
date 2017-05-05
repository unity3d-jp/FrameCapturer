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
            var ctx = recorder.context;

            EditorGUILayout.PropertyField(so.FindProperty("m_captureTarget"));
            if(recorder.captureTarget == MovieRecorder.CaptureTarget.RenderTexture)
            {
                EditorGUI.indentLevel++;
                EditorGUILayout.PropertyField(so.FindProperty("m_targetRT"));
                EditorGUI.indentLevel--;
            }

            EditorGUILayout.PropertyField(so.FindProperty("m_resolutionWidth"));
            EditorGUILayout.PropertyField(so.FindProperty("m_framerateMode"));
            if(recorder.m_framerateMode == MovieRecorder.FrameRateMode.Constant)
            {
                EditorGUI.indentLevel++;
                EditorGUILayout.PropertyField(so.FindProperty("m_targetFramerate"));
                EditorGUILayout.PropertyField(so.FindProperty("m_fixDeltaTime"));
                EditorGUI.indentLevel--;
            }
            EditorGUILayout.PropertyField(so.FindProperty("m_captureEveryNthFrame"));
        }


        public virtual void AudioConfig()
        {
        }

        public virtual void EncoderConfig()
        {
            var recorder = target as MovieRecorder;
            var so = serializedObject;
            var ctx = recorder.context;

            if (ctx.type == MovieRecorderContext.Type.Gif)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_gifEncoderConfig"), true);
            }
            else if (ctx.type == MovieRecorderContext.Type.WebM)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_webmEncoderConfig"), true);
            }
            else if (ctx.type == MovieRecorderContext.Type.MP4)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_mp4EncoderConfig"), true);
            }
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
            var ctx = recorder.context;

            CommonConfig();

            if(ctx != null)
            {
                EditorGUILayout.Space();

                if(ctx.type == MovieRecorderContext.Type.Gif)
                {
                    VideoConfig();
                }
                else if (ctx.type == MovieRecorderContext.Type.WebM)
                {
                    EditorGUILayout.PropertyField(so.FindProperty("m_webmEncoderConfig.captureVideo"));
                    if (recorder.webmConfig.captureVideo)
                    {
                        EditorGUI.indentLevel++;
                        VideoConfig();
                        EditorGUI.indentLevel--;
                    }
                    EditorGUILayout.Space();
                    EditorGUILayout.PropertyField(so.FindProperty("m_webmEncoderConfig.captureAudio"));
                    if (recorder.webmConfig.captureAudio)
                    {
                        EditorGUI.indentLevel++;
                        AudioConfig();
                        EditorGUI.indentLevel--;
                    }
                }
                else if (ctx.type == MovieRecorderContext.Type.MP4)
                {
                    EditorGUILayout.PropertyField(so.FindProperty("m_mp4EncoderConfig.captureVideo"));
                    if (recorder.mp4Config.captureVideo)
                    {
                        EditorGUI.indentLevel++;
                        VideoConfig();
                        EditorGUI.indentLevel--;
                    }
                    EditorGUILayout.Space();
                    EditorGUILayout.PropertyField(so.FindProperty("m_mp4EncoderConfig.captureAudio"));
                    if (recorder.mp4Config.captureAudio)
                    {
                        EditorGUI.indentLevel++;
                        AudioConfig();
                        EditorGUI.indentLevel--;
                    }
                }

                EditorGUILayout.Space();

                EncoderConfig();

                so.ApplyModifiedProperties();

                RecordingControl();

            }
        }
    }
}
