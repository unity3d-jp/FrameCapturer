using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor.Recorder.FrameRecorder.Utilities;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;
using UnityEngine.Recorder.FrameRecorder.Utilities;

namespace UnityEditor.Recorder.FrameRecorder
{
    public class RecorderEditorAttribute : Attribute
    {
        public Type recorderType { get; private set; }

        public RecorderEditorAttribute(Type type)
        {
            recorderType = type;
        }
    }

    public abstract class RecorderSettingsEditor : Editor
    {
        static SortedDictionary<string, Type> m_Editors;

        internal LayoutHelper m_LayoutHelper;

        public virtual Vector2 minSize
        {
            get { return new Vector2(0, 0); }
        }

        protected virtual void OnEnable()
        {
            m_LayoutHelper = new LayoutHelper(GUILayout.Width(175));
        }

        protected virtual void OnDisable() {}

        protected virtual void Awake() {}

        public bool isValid
        {
            get { return (target as FrameRecorderSettings).isValid; }
        }

        public bool showBounds { get; set; }

        bool m_FoldoutInput = true;
        bool m_FoldoutEncoder = true;
        bool m_FoldoutTime = true;
        bool m_FoldoutBounds = true;
        bool m_FoldoutOutput = true;
        public override void OnInspectorGUI()
        {
            if (target == null)
                return;

            m_LayoutHelper.indentLevel = 0;

            EditorGUI.BeginChangeCheck();
            serializedObject.Update();

            m_FoldoutInput = EditorGUILayout.Foldout(m_FoldoutInput, "Input");
            if (m_FoldoutInput)
            {
                m_LayoutHelper.indentLevel++;
                OnInputGui();
                m_LayoutHelper.indentLevel--;
            }

            m_FoldoutOutput = EditorGUILayout.Foldout(m_FoldoutOutput, "Output");
            if (m_FoldoutOutput)
            {
                m_LayoutHelper.indentLevel++;
                OnOutputGui();
                m_LayoutHelper.indentLevel--;
            }

            m_FoldoutEncoder = EditorGUILayout.Foldout(m_FoldoutEncoder, "Encoding");
            if (m_FoldoutEncoder)
            {
                m_LayoutHelper.indentLevel++;
                OnEncodingGui();
                m_LayoutHelper.indentLevel--;
            }

            m_FoldoutTime = EditorGUILayout.Foldout(m_FoldoutTime, "Time");
            if (m_FoldoutTime)
            {
                m_LayoutHelper.indentLevel++;
                OnTimeGui();
                m_LayoutHelper.indentLevel--;
            }

            if (showBounds)
            {
                m_FoldoutBounds = EditorGUILayout.Foldout(m_FoldoutBounds, "Bounds / Limits");
                if (m_FoldoutBounds)
                {
                    m_LayoutHelper.indentLevel++;
                    OnBounds();
                    m_LayoutHelper.indentLevel--;
                }
            }

            var settingsObj = serializedObject.targetObject as FrameRecorderSettings;
            m_LayoutHelper.AddBoolProperty("Verbose logging", serializedObject, () => settingsObj.m_Verbose);

            serializedObject.ApplyModifiedProperties();
            EditorGUI.EndChangeCheck();
        }

        protected virtual void OnInputGui()
        {
            m_LayoutHelper.AddIntProperty("Capture every n'th frame", serializedObject, () => (target as FrameRecorderSettings).m_CaptureEveryNthFrame);
        }

        protected virtual void OnOutputGui()
        {
        }

        protected virtual void OnEncodingGui()
        {
        }

        protected virtual void OnTimeGui()
        {
            var settingsObj = serializedObject.targetObject as FrameRecorderSettings;

            m_LayoutHelper.AddEnumProperty("Frame rate mode", serializedObject, () => settingsObj.m_FrameRateMode);
            m_LayoutHelper.indentLevel++;
            var label = settingsObj.m_FrameRateMode == FrameRateMode.Fixed ? "Frame rate" : "Max frame rate";
            m_LayoutHelper.AddDoubleProperty(label, serializedObject, () => settingsObj.m_FrameRate);
            m_LayoutHelper.indentLevel--;
        }

        protected virtual void OnBounds()
        {
            var settingsObj = serializedObject.targetObject as FrameRecorderSettings;

            m_LayoutHelper.AddEnumProperty("Recording Duration", serializedObject, () => settingsObj.m_DurationMode);

            m_LayoutHelper.indentLevel++;
            switch (settingsObj.m_DurationMode)
            {
                case DurationMode.Indefinite:
                    break;
                case DurationMode.SingleFrame:
                {
                    m_LayoutHelper.AddIntProperty("Frame", serializedObject, () => settingsObj.m_StartFrame, "Tooltip");
                    settingsObj.m_EndFrame = settingsObj.m_StartFrame;
                    break;
                }
                case DurationMode.FrameInterval:
                {
                    EditorGUILayout.BeginHorizontal();
                    m_LayoutHelper.AddPropertyLabel("Frames");
                    m_LayoutHelper.AddIntProperty(serializedObject, () => settingsObj.m_StartFrame);
                    m_LayoutHelper.AddIntProperty(serializedObject, () => settingsObj.m_EndFrame);
                    EditorGUILayout.EndHorizontal();
                    break;
                }
                case DurationMode.TimeInterval:
                {
                    EditorGUILayout.BeginHorizontal();
                    m_LayoutHelper.AddPropertyLabel("Time");
                    m_LayoutHelper.AddFloatProperty(serializedObject, () => settingsObj.m_StartTime);
                    m_LayoutHelper.AddFloatProperty(serializedObject, () => settingsObj.m_EndTime);
                    EditorGUILayout.EndHorizontal();
                    break;
                }
            }
            m_LayoutHelper.indentLevel--;
        }

        private static void Init()
        {
            if (m_Editors != null)
                return;

            m_Editors = new SortedDictionary<string, Type>();
            foreach (var editor in ClassHelpers.FilterByAttribute<RecorderEditorAttribute>())
            {
                var attrib = editor.Value[0];
                m_Editors.Add((attrib as RecorderEditorAttribute).recorderType.FullName, editor.Key);
            }
        }

        public static Type FindEditorForRecorder(Type recorder)
        {
            Init();
            return m_Editors.ContainsKey(recorder.FullName) ? m_Editors[recorder.FullName] : null;
        }
    }
}
