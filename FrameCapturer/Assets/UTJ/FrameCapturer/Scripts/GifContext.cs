using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class GifContext : MovieRecorderContext
    {
        [Serializable]
        public class EncoderConfig
        {
            public int numColors = 256;
            public int keyframeInterval = 30;
        }

        fcAPI.fcGIFContext m_ctx;
        fcAPI.fcStream m_ostream;
        fcAPI.fcDeferredCall m_callback;
        int m_keyframeInterval = 30;
        int m_numVideoFrames;

        public override Type type { get { return Type.Gif; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_numVideoFrames = 0;

            var c = recorder.gifConfig;
            m_keyframeInterval = c.keyframeInterval;

            fcAPI.fcGifConfig gifconf = fcAPI.fcGifConfig.default_value;
            gifconf.width = recorder.scratchBuffer.width;
            gifconf.height = recorder.scratchBuffer.height;
            gifconf.num_colors = Mathf.Clamp(c.numColors, 1, 256);
            gifconf.max_active_tasks = 0;
            m_ctx = fcAPI.fcGifCreateContext(ref gifconf);

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcGifAddOutputStream(m_ctx, m_ostream);
        }

        public override void Release()
        {
            fcAPI.fcGuard(() =>
            {
                m_callback.Release();
                m_ctx.Release();
                m_ostream.Release();
            });
        }

        public override int AddVideoFrame(RenderTexture frame, double timestamp)
        {
            bool keyframe = m_keyframeInterval > 0 && m_numVideoFrames % m_keyframeInterval == 0;
            m_callback = fcAPI.fcGifAddFrameTexture(m_ctx, frame, keyframe, timestamp, m_callback);
            return m_callback;
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
        }
    }
}
