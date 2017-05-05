using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class GifContext : MovieRecorderContext
    {
        public int m_numColors = 256;
        public int m_keyframe = 30;

        fcAPI.fcGIFContext m_ctx;
        fcAPI.fcStream m_ostream;
        int m_callback;
        int m_numVideoFrames;

        public override Type type { get { return Type.Gif; } }
        public override bool supportAudio { get { return false; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_numVideoFrames = 0;

            fcAPI.fcGifConfig conf;
            conf.width = recorder.scratchBuffer.width;
            conf.height = recorder.scratchBuffer.height;
            conf.num_colors = Mathf.Clamp(m_numColors, 1, 256);
            conf.max_active_tasks = 0;
            m_ctx = fcAPI.fcGifCreateContext(ref conf);

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
            bool keyframe = m_keyframe > 0 && m_numVideoFrames % m_keyframe == 0;
            m_callback = fcAPI.fcGifAddFrameTexture(m_ctx, frame, keyframe, timestamp, m_callback);
            return m_callback;
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
        }
    }
}
