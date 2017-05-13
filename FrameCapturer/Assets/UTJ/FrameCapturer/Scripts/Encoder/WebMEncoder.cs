using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class WebMEncoder : MovieEncoder
    {
        fcAPI.fcWebMContext m_ctx;
        fcAPI.fcWebMConfig m_config;

        public override Type type { get { return Type.WebM; } }

        public override void Initialize(object config, string outPath)
        {
            m_config = (fcAPI.fcWebMConfig)config;
            m_config.videoTargetFramerate = 60;
            m_config.audioSampleRate = AudioSettings.outputSampleRate;
            m_config.audioNumChannels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcWebMCreateContext(ref m_config);

            var path = outPath + ".webm";
            var stream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcWebMAddOutputStream(m_ctx, stream);
            stream.Release();
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void AddVideoFrame(byte[] frame, fcAPI.fcPixelFormat format, double timestamp)
        {
            if (m_config.video)
            {
                fcAPI.fcWebMAddVideoFramePixels(m_ctx, frame, format, timestamp);
            }
        }

        public override void AddAudioFrame(float[] samples)
        {
            if (m_config.audio)
            {
                fcAPI.fcWebMAddAudioFrame(m_ctx, samples, samples.Length);
            }
        }
    }
}
