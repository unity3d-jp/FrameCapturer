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
            gifconf.num_colors = Mathf.Clamp(m_config.numColors, 1, 256);
            gifconf.max_active_tasks = 0;
            m_ctx = fcAPI.fcGifCreateContext(ref gifconf);

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcGifAddOutputStream(m_ctx, m_ostream);

            m_callback = fcAPI.fcAllocateDeferredCall();
            recorder.commandBuffer.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callback);
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

        public override void AddVideoFrame(RenderTexture frame, double timestamp)
        {
            bool keyframe = m_config.keyframeInterval > 0 && m_numVideoFrames % m_config.keyframeInterval == 0;
            m_callback = fcAPI.fcGifAddFrameTexture(m_ctx, frame, keyframe, timestamp, m_callback);
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
        }
    }
}
