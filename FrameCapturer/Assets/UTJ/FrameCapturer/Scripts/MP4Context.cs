using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class MP4Context : MovieRecorderContext
    {
        [Serializable]
        public class EncoderConfig
        {
            [HideInInspector] public bool captureVideo = true;
            [HideInInspector] public bool captureAudio = true;
            public int videoBitrate = 8192000;
            public int audioBitrate = 64000;
        }

        fcAPI.fcMP4Context m_ctx;
        int m_callback;
        int m_numVideoFrames;

        public override Type type { get { return Type.MP4; } }

        public override void Initialize(MovieRecorder recorder)
        {
            var c = recorder.mp4Config;
            fcAPI.fcMP4Config mp4conf = fcAPI.fcMP4Config.default_value;
            mp4conf = fcAPI.fcMP4Config.default_value;
            mp4conf.video = c.captureVideo;
            mp4conf.audio = c.captureAudio;
            mp4conf.video_width = recorder.scratchBuffer.width;
            mp4conf.video_height = recorder.scratchBuffer.height;
            mp4conf.video_target_framerate = 60;
            mp4conf.video_target_bitrate = c.videoBitrate;
            mp4conf.audio_target_bitrate = c.audioBitrate;
            mp4conf.audio_sample_rate = AudioSettings.outputSampleRate;
            mp4conf.audio_num_channels = fcAPI.fcGetNumAudioChannels();

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            m_ctx = fcAPI.fcMP4OSCreateContext(ref mp4conf, path);
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
