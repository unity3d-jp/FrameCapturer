using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class MP4Encoder : MovieEncoder
    {
        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcMP4Config m_config;

        public override Type type { get { return Type.MP4; } }

        public override void Initialize(MovieRecorder recorder)
        {
            m_config = recorder.mp4Config;
            m_config.videoWidth = recorder.scratchBuffer.width;
            m_config.videoHeight = recorder.scratchBuffer.height;
            m_config.videoTargetFramerate = 60;
            m_config.audioSampleRate = AudioSettings.outputSampleRate;
            m_config.audioNumChannels = fcAPI.fcGetNumAudioChannels();

            var path = recorder.outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            m_ctx = fcAPI.fcMP4OSCreateContext(ref m_config, path);
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
            if (m_config.video)
            {
                fcAPI.fcMP4AddVideoFramePixels(m_ctx, frame, format, timestamp);
            }
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            if (m_config.audio)
            {
                fcAPI.fcMP4AddAudioFrame(m_ctx, samples, samples.Length);
            }
        }
    }
}
