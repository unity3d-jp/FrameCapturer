using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class FlacEncoder : AudioEncoder
    {
        fcAPI.fcFlacContext m_ctx;
        fcAPI.fcFlacConfig m_config;
        fcAPI.fcStream m_ostream;

        public override Type type { get { return Type.Flac; } }

        public override void Initialize(object config, string outPath)
        {
            m_config = (fcAPI.fcFlacConfig)config;
            m_config.sampleRate = AudioSettings.outputSampleRate;
            m_config.numChannels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcFlacCreateContext(ref m_config);

            var path = outPath + ".flac";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcFlacAddOutputStream(m_ctx, m_ostream);
        }

        public override void Release()
        {
            fcAPI.fcGuard(() =>
            {
                m_ctx.Release();
                m_ostream.Release();
            });
        }

        public override void AddAudioFrame(float[] samples, double timestamp)
        {
            fcAPI.fcFlacAddAudioFrame(m_ctx, samples, samples.Length);
        }
    }
}
