using System;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class WaveEncoder : AudioEncoder
    {
        fcAPI.fcWaveContext m_ctx;
        fcAPI.fcWaveConfig m_config;
        fcAPI.fcStream m_ostream;

        public override Type type { get { return Type.Wave; } }

        public override void Initialize(object config, string outPath)
        {
            m_config = (fcAPI.fcWaveConfig)config;
            m_config.sampleRate = AudioSettings.outputSampleRate;
            m_config.numChannels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcWaveCreateContext(ref m_config);

            var path = outPath + ".wave";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcWaveAddOutputStream(m_ctx, m_ostream);
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
            fcAPI.fcWaveAddAudioFrame(m_ctx, samples, samples.Length);
        }
    }
}
