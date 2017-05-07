using System;
using UnityEditor;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UnityEngine;
using UnityEngine.Playables;
using UnityEngine.Timeline;

namespace UnityEngine.Recorder.FrameRecorder.Timeline
{
    public class FrameRecorderClip : PlayableAsset, ITimelineClipAsset
    {
        [SerializeField] public string m_RecorderTypeName;
        [SerializeField] public string m_RecorderCategory;

        public FrameRecorderSettings m_Settings;
        Type m_RecorderType;

        void OnEnable()
        {
            if (string.IsNullOrEmpty(m_RecorderTypeName))
                return;

            var assetGuid = AssetDatabase.AssetPathToGUID(AssetDatabase.GetAssetPath(this));
            m_Settings = RecordersInventory.CreateRecorderSettings(recorderType, assetGuid, "TmLn-Clip:" + assetGuid);
        }

        public Type recorderType
        {
            get
            {
                if (m_RecorderType == null && !string.IsNullOrEmpty(m_RecorderTypeName))
                    m_RecorderType = Type.GetType(m_RecorderTypeName);
                return m_RecorderType;
            }
            set
            {
                if (m_RecorderType != value)
                {
                    m_RecorderType = value;
                    m_RecorderTypeName = value != null ? m_RecorderType.AssemblyQualifiedName : string.Empty;
                }
            }
        }

        public ClipCaps clipCaps
        {
            get { return ClipCaps.None; }
        }

        public override Playable CreatePlayable(PlayableGraph graph, GameObject owner)
        {
            var playable = ScriptPlayable<FrameRecorderPlayable>.Create( graph );
            var behaviour = playable.GetBehaviour();
            if (recorderType != null && UnityHelpers.IsPlaying())
            {
                behaviour.session = new RecordingSession()
                {
                    m_Recorder = RecordersInventory.InstantiateRecorder(recorderType, m_Settings),
                    m_RecorderGO = FrameRecorderGOControler.HookupRecorder(m_Settings),
                    m_RecordingStartTS = Time.time,
                    m_FrameIndex = 0
                };
            }
            return playable;
        }

        public virtual void OnDestroy()
        {
            RecordersInventory.DeleteSettings(m_Settings.m_UniqueID);
        }
    }
}
