using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [Serializable]
    public class AudioEncoderConfigs
    {
        public AudioEncoder.Type format = AudioEncoder.Type.Wave;
        public fcAPI.fcWaveConfig waveEncoderSettings = fcAPI.fcWaveConfig.default_value;
        public fcAPI.fcFlacConfig flacEncoderSettings = fcAPI.fcFlacConfig.default_value;

        public void Setup()
        {
        }
    }

    public abstract class AudioEncoder : ScriptableObject
    {
        public enum Type
        {
            Wave,
            Flac,
            //Ogg,
        }

        public abstract Type type { get; }

        // config: config struct (fcGifConfig, fcWebMConfig, etc)
        public abstract void Initialize(object config, string outPath);
        public abstract void Release();
        public abstract void AddAudioFrame(float[] samples, double timestamp = -1.0);


        public static AudioEncoder Create(Type t)
        {
            switch (t)
            {
                case Type.Wave: return CreateInstance<WaveEncoder>();
                case Type.Flac: return CreateInstance<FlacEncoder>();
            }
            return null;
        }

        public static AudioEncoder Create(AudioEncoderConfigs c, string path)
        {
            var ret = Create(c.format);
            switch (c.format)
            {
                case Type.Wave: ret.Initialize(c.waveEncoderSettings, path); break;
                case Type.Flac: ret.Initialize(c.flacEncoderSettings, path); break;
            }
            return ret;
        }
    }
}
