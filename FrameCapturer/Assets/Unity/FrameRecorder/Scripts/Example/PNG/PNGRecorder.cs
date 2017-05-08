using System;
using System.IO;
using UnityEngine.Recorder.FrameRecorder.DataSource;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UTJ.FrameCapturer;

namespace UnityEngine.Recorder.FrameRecorder.Example
{
    [FrameRecorderClass]
    public class PNGRecorder : RenderTextureRecorder<PNGRecorderSettings>
    {
        fcAPI.fcPngContext m_ctx;

        public static RecorderInfo GetRecorderInfo()
        {
            return RecorderInfo.Instantiate<PNGRecorder, PNGRecorderSettings>("Video", "PNG Recorder");
        }

        public override bool BeginRecording(RecordingSession session)
        {
            if (!base.BeginRecording(session)) { return false; }

            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            m_ctx = fcAPI.fcPngCreateContext(ref m_Settings.m_PngEncoderSettings);
            return m_ctx;
        }

        public override void EndRecording(RecordingSession session)
        {
            m_ctx.Release();
            base.EndRecording(session);
        }

        public override void RecordFrame(RecordingSession session)
        {
            if (m_BoxedSources.Count != 1)
                throw new Exception("Unsupported number of sources");

            var path = BuildOutputPath(session);
            var source = (RenderTextureSource)m_BoxedSources[0].m_Source;
            var frame = source.buffer;

            fcAPI.fcLock(frame, (data, fmt) =>
            {
                fcAPI.fcPngExportPixels(m_ctx, path, data, frame.width, frame.height, fmt, 0);
            });
        }

        string BuildOutputPath(RecordingSession session)
        {
            var outputPath = m_Settings.m_DestinationPath;
            if (outputPath.Length > 0 && !outputPath.EndsWith("/"))
                outputPath += "/";
            outputPath += m_OutputFile + (settings as PNGRecorderSettings).m_BaseFileName + recordedFramesCount.ToString("0000") + ".png";
            return outputPath;
        }
    }
}
