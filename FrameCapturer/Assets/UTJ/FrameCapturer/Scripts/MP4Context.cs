using System;
using UnityEngine;


namespace UTJ
{
    public class MP4Context : MovieRecorderContext
    {
        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcMP4Config m_mp4conf = fcAPI.fcMP4Config.default_value;
        int m_callback;
        int m_numVideoFrames;

        public override Type type { get { return Type.MP4; } }
        public override bool supportAudio { get { return true; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_mp4conf = fcAPI.fcMP4Config.default_value;
            m_mp4conf.video = recorder.captureVideo;
            m_mp4conf.audio = recorder.captureAudio;
            m_mp4conf.video_width = recorder.scratchBuffer.width;
            m_mp4conf.video_height = recorder.scratchBuffer.height;
            m_mp4conf.video_target_framerate = 60;
            m_mp4conf.video_target_bitrate = recorder.m_videoBitrate;
            m_mp4conf.audio_target_bitrate = recorder.m_audioBitrate;
            m_mp4conf.audio_sample_rate = AudioSettings.outputSampleRate;
            m_mp4conf.audio_num_channels = fcAPI.fcGetNumAudioChannels();

            var path = recorder.outputDir.GetPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            m_ctx = fcAPI.fcMP4OSCreateContext(ref m_mp4conf, path);
        }

        public override void Release()
        {
            fcAPI.fcGuard(() =>
            {
                fcAPI.fcEraseDeferredCall(m_callback);
                m_callback = 0;

                if (m_ctx)
                {
                    fcAPI.fcMP4DestroyContext(m_ctx);
                    m_ctx.ptr = IntPtr.Zero;
                }
            });
        }

        public override int AddVideoFrame(RenderTexture frame, double timestamp)
        {
            m_callback = fcAPI.fcMP4AddVideoFrameTexture(m_ctx, frame, timestamp, m_callback);
            return m_callback;
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            fcAPI.fcMP4AddAudioFrame(m_ctx, samples, samples.Length);
        }
    }
}
