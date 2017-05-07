using System;
using UnityEditor;
using UnityEngine.Recorder.FrameRecorder.Utilities;

namespace UnityEngine.Recorder.FrameRecorder
{
    public enum EImageSourceType
    {
        GameDisplay,
        SceneView,
        MainCamera,
        TaggedCamera,
        RenderTexture
    }

    public enum FrameRateMode
    {
        Variable,
        Fixed,
    }

    public enum DurationMode
    {
        Indefinite,
        SingleFrame,
        FrameInterval,
        TimeInterval
    }

    public class FrameRecorderSettings : MonoBehaviour
    {
        public string m_UniqueID;
        public string m_OwnerAssetID;
        public int m_CaptureEveryNthFrame = 1;
        public FrameRateMode m_FrameRateMode = FrameRateMode.Fixed;
        public double m_FrameRate = 24.0;
        public int m_StartFrame;
        public int m_EndFrame = 1000;
        public float m_StartTime = 0.0f;
        public float m_EndTime = 1.0f;
        public DurationMode m_DurationMode;
        public bool m_Verbose = false;

        public virtual bool isValid
        {
            get { return m_FrameRate > 0; }
        }

        public virtual void OnEnable()
        {
            GarbageCollect();

            if (string.IsNullOrEmpty(m_UniqueID))
                m_UniqueID = string.Format("{0}-{1}", GetType().Name, Guid.NewGuid());
        }

        public bool fixedDuration
        {
            get { return m_DurationMode != DurationMode.Indefinite; }
        }

        void GarbageCollect()
        {
#if UNITY_EDITOR
            if (string.IsNullOrEmpty(m_OwnerAssetID))
                return;
            try
            {
                var guid = new Guid(m_OwnerAssetID);
                if (guid == Guid.Empty)
                    return;
            }
            catch
            {
                return;
            }

            // Check if it's associated asset, if there was one, still exists. If it does not: cleanup / delete this setting
            if (string.IsNullOrEmpty(AssetDatabase.GUIDToAssetPath(m_OwnerAssetID)))
            {
                UnityHelpers.Destroy(gameObject);
            }
#endif
        }
    }
}
