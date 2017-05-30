using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [Serializable]
    public class AudioEncoderConfigs
    {
        public AudioEncoder.Type format = AudioEncoder.Type.Flac;
        public fcAPI.fcWaveConfig waveEncoderSettings = fcAPI.fcWaveConfig.default_value;
        public fcAPI.fcOggConfig oggEncoderSettings = fcAPI.fcOggConfig.default_value;
        public fcAPI.fcFlacConfig flacEncoderSettings = fcAPI.fcFlacConfig.default_value;

        public void Setup()
        {
        }
    }

    public abstract class AudioEncoder : EncoderBase
    {
        public enum Type
        {
            Wave,
            Ogg,
            Flac,
        }

        public abstract Type type { get; }

        // config: config struct (fcGifConfig, fcWebMConfig, etc)
        public abstract void Initialize(object config, string outPath);
        public abstract void AddAudioSamples(float[] samples);


        public static AudioEncoder Create(Type t)
        {
            switch (t)
            {
                case Type.Wave: return new WaveEncoder();
                case Type.Ogg: return new OggEncoder();
                case Type.Flac: return new FlacEncoder();
            }
            return null;
        }

        public static AudioEncoder Create(AudioEncoderConfigs c, string path)
        {
            var ret = Create(c.format);
            switch (c.format)
            {
                case Type.Wave: ret.Initialize(c.waveEncoderSettings, path); break;
                case Type.Ogg: ret.Initialize(c.oggEncoderSettings, path); break;
                case Type.Flac: ret.Initialize(c.flacEncoderSettings, path); break;
            }
            return ret;
        }
    }
}
