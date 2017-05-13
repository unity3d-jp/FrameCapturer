using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class FlacEncoder : AudioEncoder
    {
        fcAPI.fcFlacContext m_ctx;
        fcAPI.fcFlacConfig m_config;

        public override Type type { get { return Type.Flac; } }

        public override void Initialize(object config, string outPath)
        {
            m_config = (fcAPI.fcFlacConfig)config;
            m_config.sampleRate = AudioSettings.outputSampleRate;
            m_config.numChannels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcFlacCreateContext(ref m_config);

            var path = outPath + ".flac";
            var stream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcFlacAddOutputStream(m_ctx, stream);
            stream.Release();
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            fcAPI.fcFlacAddAudioFrame(m_ctx, samples, samples.Length);
        }
    }
}
