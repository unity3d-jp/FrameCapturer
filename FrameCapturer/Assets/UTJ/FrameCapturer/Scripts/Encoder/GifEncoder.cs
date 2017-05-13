using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class GifEncoder : MovieEncoder
    {
        fcAPI.fcGifContext m_ctx;
        fcAPI.fcGifConfig m_config;

        public override Type type { get { return Type.Gif; } }

        public override void Initialize(object config, string outPath)
        {
            m_config = (fcAPI.fcGifConfig)config;
            m_config.numColors = Mathf.Clamp(m_config.numColors, 1, 256);
            m_ctx = fcAPI.fcGifCreateContext(ref m_config);

            var path = outPath + ".gif";
            var stream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcGifAddOutputStream(m_ctx, stream);
            stream.Release();
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void AddVideoFrame(byte[] frame, fcAPI.fcPixelFormat format, double timestamp)
        {
            fcAPI.fcGifAddFramePixels(m_ctx, frame, format, timestamp);
        }

        public override void AddAudioSamples(float[] samples)
        {
            // not supported
        }
    }
}
