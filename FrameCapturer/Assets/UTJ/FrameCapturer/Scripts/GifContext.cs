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
        int m_keyframeInterval;
        int m_callback;
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
                fcAPI.fcEraseDeferredCall(m_callback);
                m_callback = 0;

                if (m_ctx)
                {
                    fcAPI.fcGifDestroyContext(m_ctx);
                    m_ctx.ptr = IntPtr.Zero;
                }
                if (m_ostream)
                {
                    fcAPI.fcDestroyStream(m_ostream);
                    m_ostream.ptr = IntPtr.Zero;
                }
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
