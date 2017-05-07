using System;
using System.IO;
using UnityEngine.Recorder.FrameRecorder.DataSource;
using UnityEngine.Recorder.FrameRecorder.Utilities;
using UTJ.FrameCapturer;

namespace UnityEngine.Recorder.FrameRecorder
{
    [FrameRecorderClass]
    public class EXRRecorder : RenderTextureRecorder<EXRRecorderSettings>
    {
        static readonly string[] s_channelNames = { "R", "G", "B", "A" };
        fcAPI.fcExrContext m_ctx;

        public static RecorderInfo GetRecorderInfo()
        {
            return RecorderInfo.Instantiate<EXRRecorder, EXRRecorderSettings>("Video", "EXR Recorder");
        }

        public override bool BeginRecording(RecordingSession session)
        {
            if (!base.BeginRecording(session)) { return false; }

            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            m_ctx = fcAPI.fcExrCreateContext(ref m_Settings.m_ExrConfig);
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

            fcAPI.fcExrBeginImage(m_ctx, path, frame.width, frame.height);
            fcAPI.fcLock(frame, (data, fmt) =>
            {
                int channels = (int)fmt & 7;
                for (int i = 0; i < channels; ++i)
                {
                    fcAPI.fcExrAddLayerPixels(m_ctx, data, fmt, i, s_channelNames[i]);
                }
            });
            fcAPI.fcExrEndImage(m_ctx);
        }

        string BuildOutputPath(RecordingSession session)
        {
            var outputPath = m_Settings.m_DestinationPath;
            if (outputPath.Length > 0 && !outputPath.EndsWith("/"))
                outputPath += "/";
            outputPath += m_OutputFile + (settings as EXRRecorderSettings).m_BaseFileName + recordedFramesCount + ".exr";
            return outputPath;
        }
    }
}
