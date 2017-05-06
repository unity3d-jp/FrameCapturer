using System;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer
{
    [CustomEditor(typeof(ImageSequenceRecorder))]
    public class ImageSequenceRecorderEditor : Editor
    {
        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as ImageSequenceRecorder;
            var so = serializedObject;

            EditorGUILayout.PropertyField(so.FindProperty("m_outputDir"), true);
            recorder.format = (ImageSequenceRecorderContext.Type)EditorGUILayout.EnumPopup("Format", recorder.format);

            EditorGUILayout.Space();
            EditorGUILayout.PropertyField(so.FindProperty("m_captureTarget"), true);
            EditorGUI.indentLevel++;
            if (recorder.captureTarget == ImageSequenceRecorder.CaptureTarget.FrameBuffer)
            {
                var fbc = recorder.fbComponents;
                fbc.frameBuffer = EditorGUILayout.Toggle("Frame Buffer", fbc.frameBuffer);
                fbc.GBuffer = EditorGUILayout.Toggle("GBuffer", fbc.GBuffer);
                if (fbc.GBuffer)
                {
                    EditorGUI.indentLevel++;
                    fbc.albedo      = EditorGUILayout.Toggle("Albedo", fbc.albedo);
                    fbc.occlusion   = EditorGUILayout.Toggle("Occlusion", fbc.occlusion);
                    fbc.specular    = EditorGUILayout.Toggle("Specular", fbc.specular);
                    fbc.smoothness  = EditorGUILayout.Toggle("Smoothness", fbc.smoothness);
                    fbc.normal      = EditorGUILayout.Toggle("Normal", fbc.normal);
                    fbc.emission    = EditorGUILayout.Toggle("Emission", fbc.emission);
                    fbc.depth       = EditorGUILayout.Toggle("Depth", fbc.depth);
                    EditorGUI.indentLevel--;
                }
                recorder.fbComponents = fbc;
            }
            else if (recorder.captureTarget == ImageSequenceRecorder.CaptureTarget.RenderTexture)
            {
                EditorGUILayout.PropertyField(so.FindProperty("m_targetRT"), true);
            }
            EditorGUI.indentLevel--;

            EditorGUILayout.Space();

            EditorGUILayout.PropertyField(so.FindProperty("m_fixDeltaTime"), true);
            if(recorder.fixDeltaTime)
            {
                EditorGUI.indentLevel++;
                EditorGUILayout.PropertyField(so.FindProperty("m_targetFramerate"), true);
                EditorGUI.indentLevel--;
            }

            EditorGUILayout.Space();

            EditorGUILayout.PropertyField(so.FindProperty("m_startFrame"), true);
            EditorGUILayout.PropertyField(so.FindProperty("m_endFrame"), true);

            so.ApplyModifiedProperties();


            // recording control

            EditorGUILayout.Space();
            if (!recorder.isRecording)
            {
                if (GUILayout.Button("Begin Recording"))
                {
                    recorder.BeginRecording();
                }
                EditorGUILayout.Space();
                if (GUILayout.Button("One Shot"))
                {
                    recorder.OneShot();
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

    }
}
