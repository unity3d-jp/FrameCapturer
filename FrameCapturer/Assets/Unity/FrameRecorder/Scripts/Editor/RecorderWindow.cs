using System;
using System.Collections.Generic;
using System.Linq;
using Assets.Unity.FrameRecorder.Scripts.Editor;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;

namespace UnityEditor.Recorder.FrameRecorder
{
    public class RecorderWindow : EditorWindow, IRecorderSelectorTarget
    {
        [SerializeField]    string m_RecorderTypeName;
        [SerializeField]    string m_RecorderCategory;
        [SerializeField]    RecorderSettingsEditor m_SettingsEditor;
        bool m_PendingStartRecording;
        RecorderSelector m_recorderSelector;

        Type recorderType
        {
            get { return Type.GetType(m_RecorderTypeName); }
            set { m_RecorderTypeName = value == null ? string.Empty : value.AssemblyQualifiedName; }
        }

        [MenuItem("Window/Frame Recorder...")]
        public static void ShowWindow()
        {
            GetWindow(typeof(RecorderWindow), true, "Frame Recorder");
        }

        public void OnEnable()
        {
            m_recorderSelector = new RecorderSelector(this);
        }

        public void OnGUI()
        {
            if (m_PendingStartRecording && EditorApplication.isPlaying)
                DelayedStartRecording();

            var size = new Vector2(400, 400);

            using (new EditorGUI.DisabledScope(EditorApplication.isPlaying))
                m_recorderSelector.OnGui();

            if (m_SettingsEditor != null)
            {
                m_SettingsEditor.showBounds = true;
                using (new EditorGUI.DisabledScope(EditorApplication.isPlaying))
                {
                    EditorGUILayout.Separator();

                    var editorMinSize = m_SettingsEditor.minSize;
                    if (editorMinSize.x > minSize.x) size.x = editorMinSize.x;
                    if (editorMinSize.y > minSize.y) size.y = editorMinSize.y;

                    m_SettingsEditor.OnInspectorGUI();

                    EditorGUILayout.Separator();
                }
                RecordButton();
            }

            minSize = size;
        }

        public void OnDestroy()
        {
            StopRecording();
            UnityHelpers.Destroy(m_SettingsEditor);
        }

        void RecordButton()
        {
            var settings = (FrameRecorderSettings)m_SettingsEditor.target;
            var recorderGO = FrameRecorderGOControler.FindRecorder(settings);

            if (recorderGO == null)
            {
                using (new EditorGUI.DisabledScope(!m_SettingsEditor.isValid))
                {
                    if (GUILayout.Button("Start Recording"))
                        StartRecording();
                }
            }
            else
            {
                if (GUILayout.Button("Stop Recording"))
                    StopRecording();
            }
        }

        void StartRecording()
        {
            if (!EditorApplication.isPlaying || EditorApplication.isPlaying)
            {
                m_PendingStartRecording = true;
                EditorApplication.isPlaying = true;
                return;
            }
            else
                StartRecording(false);
        }

        void DelayedStartRecording()
        {
            m_PendingStartRecording = false;
            StartRecording(true);
        }

        void StartRecording(bool autoExitPlayMode)
        {
            var settings = (FrameRecorderSettings)m_SettingsEditor.target;
            var go = FrameRecorderGOControler.HookupRecorder(settings);
            var session = new RecordingSession()
            {
                m_Recorder = RecordersInventory.InstantiateRecorder(recorderType, settings),
                m_RecorderGO = go,
                m_RecordingStartTS = Time.time / Time.timeScale,
                m_FrameIndex = 0
            };

            var component = go.AddComponent<RecorderComponent>();
            component.session = session;
            component.autoExitPlayMode = autoExitPlayMode;

            session.BeginRecording();
        }

        void StopRecording()
        {
            if (m_SettingsEditor != null)
            {
                var settings = (FrameRecorderSettings)m_SettingsEditor.target;
                if (settings != null)
                {
                    var recorderGO = FrameRecorderGOControler.FindRecorder(settings);
                    if (recorderGO != null)
                    {
                        UnityHelpers.Destroy(recorderGO);
                    }
                }
            }
        }

        public void SetRecorder(Type newRecorderType)
        {
            if (newRecorderType == null || (m_SettingsEditor != null && recorderType == newRecorderType))
                return;

            recorderType = newRecorderType;

            var editorType = RecorderSettingsEditor.FindEditorForRecorder(recorderType);
            if (editorType != null)
            {
                if (m_SettingsEditor != null)
                {
                    UnityHelpers.Destroy(m_SettingsEditor.target);
                    UnityHelpers.Destroy(m_SettingsEditor);
                    m_SettingsEditor = null;
                }

                var settings = RecordersInventory.CreateRecorderSettings(recorderType, "N/A", "RecorderWindow");
                m_SettingsEditor = UnityEditor.Editor.CreateEditor(settings, editorType) as RecorderSettingsEditor;
                m_SettingsEditor.hideFlags = HideFlags.DontUnloadUnusedAsset; // <-- this means life time is manually managed by this class!!
            }
            else
                Debug.LogError(string.Format("No editor class declared for recorder of type " + newRecorderType.FullName));
        }

        public string recorderCategory
        {
            get
            {
                return m_RecorderCategory;
            }

            set
            {
                if (m_RecorderCategory != value)
                {
                    m_RecorderCategory = value;
                    m_SettingsEditor = null;
                    recorderType = null;
                }
            }
        }

        public string selectedRecorder
        {
            get { return m_RecorderTypeName; }
        }
    }
}
