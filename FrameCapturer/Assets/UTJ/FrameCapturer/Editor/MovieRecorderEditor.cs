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
            var so = serializedObject;
            EditorGUILayout.PropertyField(so.FindProperty("m_outputDir"), true);
            EditorGUILayout.PropertyField(so.FindProperty("m_encoderConfigs"), true);
        }

        public virtual void VideoConfig()
        {
            var recorder = target as MovieRecorder;
            var so = serializedObject;

            EditorGUILayout.PropertyField(so.FindProperty("m_resolutionWidth"));
            EditorGUILayout.PropertyField(so.FindProperty("m_captureTarget"));
            if(recorder.captureTarget == MovieRecorder.CaptureTarget.RenderTexture)
            {
                EditorGUI.indentLevel++;
                EditorGUILayout.PropertyField(so.FindProperty("m_targetRT"));
                EditorGUI.indentLevel--;
            }

            EditorGUILayout.PropertyField(so.FindProperty("m_framerateMode"));
            if(recorder.framerateMode == MovieRecorder.FrameRateMode.Constant)
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

        public virtual void RecordingControl()
        {
            var recorder = target as MovieRecorder;
            var so = serializedObject;

            EditorGUILayout.Space();

            // capture control
            EditorGUILayout.PropertyField(so.FindProperty("m_captureControl"));
            EditorGUI.indentLevel++;
            if (recorder.captureControl == MovieRecorder.CaptureControl.FrameRange)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_startFrame"));
                EditorGUILayout.PropertyField(so.FindProperty("m_endFrame"));
            }
            else if (recorder.captureControl == MovieRecorder.CaptureControl.TimeRange)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_startTime"));
                EditorGUILayout.PropertyField(so.FindProperty("m_endTime"));
            }

            if( recorder.captureControl == MovieRecorder.CaptureControl.FrameRange ||
                recorder.captureControl == MovieRecorder.CaptureControl.TimeRange)
            {
                if (!EditorApplication.isPlaying)
                {
                    EditorGUILayout.Space();
                    if (GUILayout.Button("Play"))
                    {
                        EditorApplication.isPlaying = true;
                    }
                }
                else if(recorder.isRecording)
                {
                    if (GUILayout.Button("Abort"))
                    {
                        recorder.EndRecording();
                    }
                }
            }
            else if (recorder.captureControl == MovieRecorder.CaptureControl.Manual)
            {
                EditorGUILayout.Space();
                if (!recorder.isRecording)
                {
                    if (GUILayout.Button("Start Recording"))
                    {
                        if (!EditorApplication.isPlaying)
                        {
                            recorder.recordingOnStart = true;
                            EditorApplication.isPlaying = true;
                        }
                        else
                        {
                            recorder.BeginRecording();
                        }
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
                ec.captureVideo = EditorGUILayout.Toggle("Video", ec.captureVideo);
                if(ec.captureVideo)
                {
                    EditorGUI.indentLevel++;
                    VideoConfig();
                    EditorGUI.indentLevel--;
                }
                EditorGUILayout.Space();

                ec.captureAudio = EditorGUILayout.Toggle("Audio", ec.captureAudio);
                if (ec.captureAudio)
                {
                    EditorGUI.indentLevel++;
                    AudioConfig();
                    EditorGUI.indentLevel--;
                }
            }

            EditorGUILayout.Space();

            RecordingControl();

            so.ApplyModifiedProperties();
        }
    }
}
