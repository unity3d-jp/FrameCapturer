using System;
using System.Resources;
using Assets.Unity.FrameRecorder.Scripts.Editor;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;
using UnityEngine.Recorder.FrameRecorder.Timeline;
using UnityEngine.Timeline;

namespace UnityEditor.Recorder.FrameRecorder.Timeline
{
    [CustomEditor(typeof(FrameRecorderClip), true)]
    public class RecorderClipEditor : Editor, IRecorderSelectorTarget
    {
        RecorderSettingsEditor m_SettingsEditor;
        TimelineAsset m_Timeline;
        RecorderSelector m_recorderSelector;

        public void OnEnable()
        {
            m_recorderSelector = new RecorderSelector(this);
            var shot = this.target as FrameRecorderClip;
            var editorType = RecorderSettingsEditor.FindEditorForRecorder(shot.recorderType);
            if (editorType != null)
            {
                m_SettingsEditor = Editor.CreateEditor(shot.m_Settings, editorType) as RecorderSettingsEditor;
            }
        }

        public override void OnInspectorGUI()
        {
            if (target == null)
                return;

            m_recorderSelector.OnGui();

            if (m_SettingsEditor != null)
            {
                m_SettingsEditor.showBounds = false;
                m_Timeline = FindTimelineAsset();

                PushTimelineIntoRecorder();

                using (new EditorGUI.DisabledScope(EditorApplication.isPlaying))
                {
                    EditorGUILayout.Separator();

                    using (new EditorGUI.DisabledScope(true))
                    {
                        GUILayout.BeginHorizontal();
                        GUILayout.Label("Settings ID:");
                        EditorGUILayout.TextField((m_SettingsEditor.target as FrameRecorderSettings).m_UniqueID);
                        GUILayout.EndHorizontal();
                    }


                    m_SettingsEditor.OnInspectorGUI();

                    EditorGUILayout.Separator();

                    PushRecorderIntoTimeline();

                    serializedObject.Update();
                }
            }
        }

        public string recorderCategory
        {
            get
            {
                var shot = this.target as FrameRecorderClip;
                return shot.m_RecorderCategory;
            }

            set
            {
                var shot = this.target as FrameRecorderClip;
                if (shot.m_RecorderCategory != value)
                {
                    shot.m_RecorderCategory = value;
                    m_SettingsEditor = null;
                    shot.recorderType = null;
                }
            }
        }

        public string selectedRecorder
        {
            get
            {
                var shot = this.target as FrameRecorderClip;
                return shot.m_RecorderTypeName;
            }
        }

        public void SetRecorder(Type newRecorderType)
        {
            var clip = this.target as FrameRecorderClip;

            if (newRecorderType == null || (m_SettingsEditor != null && m_SettingsEditor.target != null && clip.recorderType == newRecorderType))
                return;

            clip.recorderType = newRecorderType;

            var editorType = RecorderSettingsEditor.FindEditorForRecorder(clip.recorderType);
            if (editorType != null)
            {
                var assetGuid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(clip));
                clip.m_Settings = RecordersInventory.CreateRecorderSettings(clip.recorderType, assetGuid, "TmLn-Clip:" + assetGuid);
                m_SettingsEditor = CreateEditor(clip.m_Settings, editorType) as RecorderSettingsEditor;
            }
            else
                Debug.LogError(string.Format("No editor class declared for recorder of type " + newRecorderType.FullName));
        }

        TimelineAsset FindTimelineAsset()
        {
            if (!AssetDatabase.Contains(target))
                return null;

            var path = AssetDatabase.GetAssetPath(target);
            var objs = AssetDatabase.LoadAllAssetsAtPath(path);

            foreach (var obj in objs)
            {
                if (obj != null && AssetDatabase.IsMainAsset(obj))
                    return obj as TimelineAsset;
            }
            return null;
        }

        void PushTimelineIntoRecorder()
        {
            if (m_Timeline == null)
                return;

            var settings = m_SettingsEditor.target as FrameRecorderSettings;
            settings.m_DurationMode = DurationMode.Indefinite;

            // Time
            settings.m_FrameRate = m_Timeline.editorSettings.fps;
        }

        void PushRecorderIntoTimeline()
        {
            if (m_Timeline == null)
                return;

            var settings = m_SettingsEditor.target as FrameRecorderSettings;
            settings.m_DurationMode = DurationMode.Indefinite;

            // Time
            m_Timeline.editorSettings.fps = (float)settings.m_FrameRate;
        }
    }
}
