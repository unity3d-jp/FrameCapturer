using System;
using System.IO;
using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;
using UnityEngine.Recorder.FrameRecorder.DataSource;
using UTJ.FrameCapturer;


namespace UnityEngine.Recorder.FrameRecorder.Example
{
    [FrameRecorderClass]
    public class MP4Recorder : RenderTextureRecorder<MP4RecorderSettings>
    {
        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcStream m_stream;

        public static RecorderInfo GetRecorderInfo()
        {
            return RecorderInfo.Instantiate<MP4Recorder, MP4RecorderSettings>("Video", "MP4 Recorder");
        }

        public override bool BeginRecording(RecordingSession session)
        {
            if (!base.BeginRecording(session)) { return false; }

            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            m_stream = fcAPI.fcCreateFileStream(BuildOutputPath(session));
            m_ctx = fcAPI.fcMP4CreateContext(ref m_Settings.m_MP4EncoderSettings);
            fcAPI.fcMP4AddOutputStream(m_ctx, m_stream);
            return m_ctx;
        }

        public override void EndRecording(RecordingSession session)
        {
            m_ctx.Release();
            m_stream.Release();
            base.EndRecording(session);
        }

        public override void RecordFrame(RecordingSession session)
        {
            if (m_BoxedSources.Count != 1)
                throw new Exception("Unsupported number of sources");

            var source = (RenderTextureSource)m_BoxedSources[0].m_Source;
            var frame = source.buffer;

            fcAPI.fcLock(frame, TextureFormat.RGB24, (data, fmt) =>
            {
                fcAPI.fcMP4AddVideoFramePixels(m_ctx, data, fmt, session.m_CurrentFrameStartTS);
            });
        }

        string BuildOutputPath(RecordingSession session)
        {
            var outputPath = m_Settings.m_DestinationPath;
            if (outputPath.Length > 0 && !outputPath.EndsWith("/"))
                outputPath += "/";
            outputPath += m_OutputFile + (settings as WEBMRecorderSettings).m_BaseFileName + ".mp4";
            return outputPath;
        }

    }
}
