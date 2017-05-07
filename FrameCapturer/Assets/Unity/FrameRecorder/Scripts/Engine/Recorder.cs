using System;
using System.Collections.Generic;
using UnityEngine.Recorder.FrameRecorder.DataSource;

namespace UnityEngine.Recorder.FrameRecorder
{
    public enum ERecordingSessionStage
    {
        BeginRecording,
        NewFrameStarting,
        NewFrameReady,
        FrameDone,
        EndRecording,
    }

    public abstract class Recorder : ScriptableObject
    {
        double m_OriginalCaptureFrameRate;

        public int recordedFramesCount { get; set; }

        /// <summary>
        /// Concept: container class for Recorder Source objects that assumes nothing on the contained source object
        ///
        /// Motivation: let's recorders use any type of object as a source of data to record.
        /// </summary>
        protected struct BoxedSource
        {
            public System.Object m_Source;
            public SortedDictionary<ERecordingSessionStage, Action<RecordingSession>> m_StageHandlers;

            public BoxedSource(System.Object source)
            {
                m_Source = source;
                m_StageHandlers = new SortedDictionary<ERecordingSessionStage, Action<RecordingSession>>();
            }

            public void SignalNewStage(ERecordingSessionStage stage, RecordingSession session)
            {
                if (m_StageHandlers.ContainsKey(stage))
                    m_StageHandlers[stage](session);
            }
        }
        protected List<BoxedSource> m_BoxedSources;

        public virtual void Reset()
        {
            recordedFramesCount = 0;
            recording = false;
        }

        protected virtual void OnDestroy()
        {
        }

        public abstract FrameRecorderSettings settings { get; set; }

        // returns true if recording is starting. false if failed to begin recording or was already recording
        public virtual bool BeginRecording(RecordingSession session)
        {
            if (recording)
                return false;

            if (settings.m_Verbose)
                Debug.Log(string.Format("Recorder {0} starting to record", GetType().Name));

            m_OriginalCaptureFrameRate = Time.captureFramerate;
            var fixedRate = settings.m_FrameRateMode == FrameRateMode.Fixed ? (int)settings.m_FrameRate : m_OriginalCaptureFrameRate;
            if (fixedRate != m_OriginalCaptureFrameRate)
            {
                if (Time.captureFramerate > 0)
                    Debug.LogWarning(string.Format("Frame Recorder {0} is set to record at a fixed rate and another component has already set a conflicting value for [Time.captureFramerate], new value being applied : {1}!", GetType().Name, fixedRate));
                Time.captureFramerate = (int)fixedRate;

                if (settings.m_Verbose)
                    Debug.Log("Frame recorder set fixed frame rate to " + fixedRate);
            }

            return true;
        }

        public virtual void EndRecording(RecordingSession ctx)
        {
            if (!recording)
                return;
            recording = false;

            if (Time.captureFramerate != m_OriginalCaptureFrameRate)
            {
                Time.captureFramerate = (int)m_OriginalCaptureFrameRate;
                if (settings.m_Verbose)
                    Debug.Log("Frame recorder resetting fixed frame rate to original value of " + m_OriginalCaptureFrameRate);
            }

            foreach (var source in m_BoxedSources)
            {
                if (source.m_Source is IDisposable)
                    (source.m_Source as IDisposable).Dispose();
            }

            Debug.Log(string.Format("{0} recording stopped, total frame count: {1}", GetType().Name, recordedFramesCount));
        }

        public abstract void RecordFrame(RecordingSession ctx);
        public virtual void PrepareNewFrame(RecordingSession ctx)
        {
        }

        public virtual bool SkipFrame(RecordingSession ctx)
        {
            return !recording || (ctx.m_FrameIndex % settings.m_CaptureEveryNthFrame) != 0;
        }

        public bool recording { get; protected set; }

        public void SignalSourcesOfStage(ERecordingSessionStage stage, RecordingSession session)
        {
            if (m_BoxedSources != null)
            {
                foreach (var boxedSource in m_BoxedSources)
                    boxedSource.SignalNewStage(stage, session);
            }
        }
    }
}
