using System;
using System.IO;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;
using UnityEngine.Recorder.FrameRecorder.DataSource;

using UTJ;


namespace UTJ.FrameRecorder
{
    [FrameRecorderClass]
    public class MP4Recorder : RenderTextureRecorder<MP4RecorderSettings>
    {
        public static RecorderInfo GetRecorderInfo()
        {
            return RecorderInfo.Instantiate<MP4Recorder, MP4RecorderSettings>("Video", "Test MP4 Recorder");
        }

        public override bool BeginRecording(RecordingSession session)
        {
            if (!base.BeginRecording(session))
                return false;

            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            // Output file
            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            m_OutputFile = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            if (m_Settings.m_BaseFileName == "[date]")
                m_OutputFile = string.Format("{0:yyyyMMdd_HHmmss}.mp4", DateTime.Now);
            else
                m_OutputFile = string.Format("{0}.mp4", m_Settings.m_BaseFileName);


            // initialize context and stream
            // todo

            return recording;
        }

        public override void RecordFrame(RecordingSession ctx)
        {
            if (m_BoxedSources.Count != 1)
                throw new Exception("Unsupported number of sources");
            else if (settings.m_Verbose)
                Debug.Log("Recording frame #" + ctx.m_FrameIndex + " at " + Time.unscaledTime);

            var source = (RenderTextureSource)m_BoxedSources[0].m_Source;
            // todo
        }

        public override void EndRecording(RecordingSession ctx)
        {
            ReleaseMP4Context();

            base.EndRecording(ctx);
        }

        void ReleaseMP4Context()
        {
        }

        string BuildOutputPath()
        {
            var outputPath = m_Settings.m_DestinationPath;
            if (outputPath.Length > 0 && !outputPath.EndsWith("/"))
                outputPath += "/";
            outputPath += m_OutputFile;
            return outputPath;
        }

        protected override void OnDestroy()
        {
            ReleaseMP4Context();
            base.OnDestroy();
        }
    }
}
