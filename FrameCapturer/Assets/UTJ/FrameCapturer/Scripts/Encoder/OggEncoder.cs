using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class OggEncoder : AudioEncoder
    {
        fcAPI.fcOggContext m_ctx;
        fcAPI.fcOggConfig m_config;
        fcAPI.fcStream m_ostream;

        public override Type type { get { return Type.Ogg; } }

        public override void Initialize(object config, string outPath)
        {
            m_config = (fcAPI.fcOggConfig)config;
            m_config.sampleRate = AudioSettings.outputSampleRate;
            m_config.numChannels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcOggCreateContext(ref m_config);

            var path = outPath + ".ogg";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcOggAddOutputStream(m_ctx, m_ostream);
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
            fcAPI.fcOggAddAudioFrame(m_ctx, samples, samples.Length);
        }
    }
}
