using System;
using System.Collections;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UnityEngine.Playables;

namespace UnityEngine.Recorder.FrameRecorder.Timeline
{
    /// <summary>
    /// Note: Totally ignores the time info comming from the playable infrastructure. Only conciders scaled time.
    /// </summary>
    public class FrameRecorderPlayable : PlayableBehaviour
    {
        PlayState m_PlayState = PlayState.Paused;
        public RecordingSession session { get; set; }
        WaitForEndOfFrameComponent endOfFrameComp;
        bool m_FirstOneSkipped;

        public override void OnGraphStart(Playable playable)
        {
            if (session != null)
            {
                // does not support multiple starts...
                session.BeginRecording();
                m_PlayState = PlayState.Paused;
            }
        }

        public override void OnGraphStop(Playable playable)
        {
            if (session != null)
                session.EndRecording();
        }

        public override void PrepareFrame(Playable playable, FrameData info)
        {
            if (session != null && session.recording)
            {
                session.m_CurrentFrameStartTS = (Time.time / Time.timeScale) - session.m_RecordingStartTS;
                session.PrepareNewFrame();
            }
        }

        public override void ProcessFrame(Playable playable, FrameData info, object playerData)
        {
            if (session != null)
            {
                if (endOfFrameComp == null)
                {
                    endOfFrameComp = session.m_RecorderGO.AddComponent<WaitForEndOfFrameComponent>();
                    endOfFrameComp.m_playable = this;
                }

                if (session.recording)
                    session.m_FrameIndex++;
            }
        }


        public override void OnBehaviourPlay(Playable playable, FrameData info)
        {
            if (session == null)
                return;

            // Assumption: OnPlayStateChanged( PlayState.Playing ) ONLY EVER CALLED ONCE for this type of playable.
            session.m_RecordingStartTS = Time.time / Time.timeScale;
            m_PlayState = PlayState.Playing;
        }

        public override void OnBehaviourPause(Playable playable, FrameData info)
        {
            if (session == null)
                return;
            if (session.recording && m_PlayState == PlayState.Playing)
                session.EndRecording();

            m_PlayState = PlayState.Paused;
        }

        public void FrameEnded()
        {
            if (!m_FirstOneSkipped)
            {
                m_FirstOneSkipped = true;
                return;
            }

            if (session != null && session.recording)
                session.RecordFrame();
        }
    }
}
