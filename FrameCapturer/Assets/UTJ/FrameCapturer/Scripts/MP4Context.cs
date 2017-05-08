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
        EncoderConfig m_config;

        public override Type type { get { return Type.MP4; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_config = recorder.mp4Config;
            var mp4conf = fcAPI.fcMP4Config.default_value;
            mp4conf.video = m_config.captureVideo;
            mp4conf.audio = m_config.captureAudio;
            mp4conf.videoWidth = recorder.scratchBuffer.width;
            mp4conf.videoHeight = recorder.scratchBuffer.height;
            mp4conf.videoTargetFramerate = 60;
            mp4conf.videoTargetBitrate = m_config.videoBitrate;
            mp4conf.audioTargetBitrate = m_config.audioBitrate;
            mp4conf.audioSampleRate = AudioSettings.outputSampleRate;
            mp4conf.audioNumChannels = fcAPI.fcGetNumAudioChannels();

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            m_ctx = fcAPI.fcMP4OSCreateContext(ref mp4conf, path);
        }

        public override void Release()
        {
            fcAPI.fcGuard(() =>
            {
                m_ctx.Release();
            });
        }

        public override void AddVideoFrame(byte[] frame, fcAPI.fcPixelFormat format, double timestamp)
        {
            if (m_config.captureVideo)
            {
                fcAPI.fcMP4AddVideoFramePixels(m_ctx, frame, format, timestamp);
            }
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            if (m_config.captureAudio)
            {
                fcAPI.fcMP4AddAudioFrame(m_ctx, samples, samples.Length);
            }
        }
    }
}
