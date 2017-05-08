using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class GifEncoder : MovieEncoder
    {
        fcAPI.fcGifContext m_ctx;
        fcAPI.fcGifConfig m_config;
        fcAPI.fcStream m_ostream;

        public override Type type { get { return Type.Gif; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_config = recorder.gifConfig;
            m_config.width = recorder.scratchBuffer.width;
            m_config.height = recorder.scratchBuffer.height;
            m_config.numColors = Mathf.Clamp(m_config.numColors, 1, 256);
            m_config.maxTasks = 0;
            m_ctx = fcAPI.fcGifCreateContext(ref m_config);

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcGifAddOutputStream(m_ctx, m_ostream);
        }

        public override void Release()
        {
            fcAPI.fcGuard(() =>
            {
                m_ctx.Release();
                m_ostream.Release();
            });
        }

        public override void AddVideoFrame(byte[] frame, fcAPI.fcPixelFormat format, double timestamp)
        {
            fcAPI.fcGifAddFramePixels(m_ctx, frame, format, timestamp);
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            // not supported
        }
    }
}
