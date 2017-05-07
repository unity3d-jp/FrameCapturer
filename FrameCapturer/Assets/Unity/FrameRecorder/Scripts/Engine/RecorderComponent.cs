using System;
using System.Collections;

namespace UnityEngine.Recorder.FrameRecorder
{
    [ExecuteInEditMode]
    public class RecorderComponent : MonoBehaviour
    {
        public bool autoExitPlayMode { get; set; }
        public RecordingSession session { get; set; }

        public void Update()
        {
            if (session != null && session.recording)
            {
                session.m_CurrentFrameStartTS = (Time.time / Time.timeScale) - session.m_RecordingStartTS;
                session.m_FrameIndex++;

                session.PrepareNewFrame();
            }
        }

        IEnumerator RecordFrame()
        {
            yield return new WaitForEndOfFrame();
            if (session != null && session.recording)
            {
                session.RecordFrame();

                switch (session.m_Recorder.settings.m_DurationMode)
                {
                    case DurationMode.Indefinite:
                        break;
                    case DurationMode.SingleFrame:
                        enabled = false;
                        break;
                    case DurationMode.FrameInterval:
                        if (session.m_FrameIndex >= session.settings.m_EndFrame)
                            enabled = false;
                        break;
                    case DurationMode.TimeInterval:
                        if (session.m_CurrentFrameStartTS >= session.settings.m_EndTime)
                            enabled = false;
                        break;
                }
            }
        }

        public void LateUpdate()
        {
            if (session != null && session.recording)
            {
                if (session.m_FrameIndex >= session.settings.m_StartFrame)
                {
                    StartCoroutine(RecordFrame());
                }
            }
        }

        public void OnDisable()
        {
            if (session != null)
            {
                session.Dispose();
                session = null;

#if UNITY_EDITOR
                if (autoExitPlayMode)
                    UnityEditor.EditorApplication.isPlaying = false;
#endif
            }
        }

        public void OnDestroy()
        {
            if (session != null)
                session.Dispose();
        }
    }
}
