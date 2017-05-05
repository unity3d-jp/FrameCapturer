using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class WebMContext : MovieRecorderContext
    {
        fcAPI.fcWebMContext m_ctx;
        fcAPI.fcWebMConfig m_webmconf = fcAPI.fcWebMConfig.default_value;
        fcAPI.fcStream m_ostream;
        int m_callback = 0;

        public override Type type { get { return Type.WebM; } }
        public override bool supportAudio { get { return true; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_webmconf = fcAPI.fcWebMConfig.default_value;
            m_webmconf.video = recorder.captureVideo;
            m_webmconf.audio = recorder.captureAudio;
            m_webmconf.video_width = recorder.scratchBuffer.width;
            m_webmconf.video_height = recorder.scratchBuffer.height;
            m_webmconf.video_target_framerate = 60;
            m_webmconf.video_target_bitrate = recorder.m_videoBitrate;
            m_webmconf.audio_target_bitrate = recorder.m_audioBitrate;
            m_webmconf.audio_sample_rate = AudioSettings.outputSampleRate;
            m_webmconf.audio_num_channels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcWebMCreateContext(ref m_webmconf);

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".webm";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcWebMAddOutputStream(m_ctx, m_ostream);
        }

        public override void Release()
        {
            fcAPI.fcGuard(() =>
            {
                fcAPI.fcEraseDeferredCall(m_callback);
                m_callback = 0;

                if (m_ctx)
                {
                    fcAPI.fcWebMDestroyContext(m_ctx);
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
            m_callback = fcAPI.fcWebMAddVideoFrameTexture(m_ctx, frame, timestamp, m_callback);
            return m_callback;
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            fcAPI.fcWebMAddAudioFrame(m_ctx, samples, samples.Length);
        }
    }
}
