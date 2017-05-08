using System;
using System.IO;
using UnityEngine.Recorder.FrameRecorder.DataSource;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UTJ.FrameCapturer;

namespace UnityEngine.Recorder.FrameRecorder.Example
{
    [FrameRecorderClass]
    public class WEBMRecorder : RenderTextureRecorder<WEBMRecorderSettings>
    {
        fcAPI.fcWebMContext m_ctx;
        fcAPI.fcStream m_stream;

        public static RecorderInfo GetRecorderInfo()
        {
            return RecorderInfo.Instantiate<WEBMRecorder, WEBMRecorderSettings>("Video", "WEBM Recorder");
        }

        public override bool BeginRecording(RecordingSession session)
        {
            if (!base.BeginRecording(session)) { return false; }

            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            return true;
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

            if (!m_ctx)
            {
                var settings = m_Settings.m_WebmEncoderSettings;
                settings.video = true;
                settings.audio = false;
                settings.videoWidth = frame.width;
                settings.videoHeight = frame.height;
                settings.videoTargetFramerate = 60; // ?
                m_ctx = fcAPI.fcWebMCreateContext(ref settings);
                m_stream = fcAPI.fcCreateFileStream(BuildOutputPath(session));
                fcAPI.fcWebMAddOutputStream(m_ctx, m_stream);
            }

            fcAPI.fcLock(frame, TextureFormat.RGB24, (data, fmt) =>
            {
                fcAPI.fcWebMAddVideoFramePixels(m_ctx, data, fmt, session.m_CurrentFrameStartTS);
            });
        }

        string BuildOutputPath(RecordingSession session)
        {
            var outputPath = m_Settings.m_DestinationPath;
            if (outputPath.Length > 0 && !outputPath.EndsWith("/"))
                outputPath += "/";
            outputPath += m_OutputFile + (settings as WEBMRecorderSettings).m_BaseFileName + ".webm";
            return outputPath;
        }
    }
}
