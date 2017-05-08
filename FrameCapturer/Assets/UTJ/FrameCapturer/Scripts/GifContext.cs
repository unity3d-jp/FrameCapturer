using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class GifContext : MovieRecorderContext
    {
        [Serializable]
        public class EncoderConfig
        {
            [Range(1, 256)] public int numColors = 256;
            [Range(1, 120)] public int keyframeInterval = 30;
        }

        fcAPI.fcGifContext m_ctx;
        fcAPI.fcStream m_ostream;
        EncoderConfig m_config;
        int m_numVideoFrames;

        public override Type type { get { return Type.Gif; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_numVideoFrames = 0;

            m_config = recorder.gifConfig;
            fcAPI.fcGifConfig gifconf = fcAPI.fcGifConfig.default_value;
            gifconf.width = recorder.scratchBuffer.width;
            gifconf.height = recorder.scratchBuffer.height;
            gifconf.numColors = Mathf.Clamp(m_config.numColors, 1, 256);
            gifconf.maxTasks = 0;
            m_ctx = fcAPI.fcGifCreateContext(ref gifconf);

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
            bool keyframe = m_config.keyframeInterval > 0 && m_numVideoFrames % m_config.keyframeInterval == 0;
            fcAPI.fcGifAddFramePixels(m_ctx, frame, format, keyframe, timestamp);
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            // not supported
        }
    }
}
