using System;
using UnityEditor;
using UnityEngine;

namespace UTJ.FrameCapturer
{
    [CustomEditor(typeof(GBufferRecorder))]
    public class ImageSequenceRecorderEditor : RecorderBaseEditor
    {
        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as GBufferRecorder;
            var so = serializedObject;

            CommonConfig();

            EditorGUILayout.Space();
            EditorGUILayout.LabelField("Capture Components");
            EditorGUI.indentLevel++;
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
                    fbc.velocity    = EditorGUILayout.Toggle("Velocity", fbc.velocity);
                    EditorGUI.indentLevel--;
                }
                recorder.fbComponents = fbc;
            }
            EditorGUI.indentLevel--;

            EditorGUILayout.Space();

            ResolutionControl();
            FramerateControl();

            EditorGUILayout.Space();

            RecordingControl();

            so.ApplyModifiedProperties();
        }

    }
}
