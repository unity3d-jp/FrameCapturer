using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [Serializable]
    public class EncoderConfigs
    {
        public MovieEncoder.Type m_type = MovieEncoder.Type.WebM;
        public fcAPI.fcPngConfig m_png = fcAPI.fcPngConfig.default_value;
        public fcAPI.fcExrConfig m_exr = fcAPI.fcExrConfig.default_value;
        public fcAPI.fcGifConfig m_gif = fcAPI.fcGifConfig.default_value;
        public fcAPI.fcWebMConfig m_webm = fcAPI.fcWebMConfig.default_value;
        public fcAPI.fcMP4Config m_mp4 = fcAPI.fcMP4Config.default_value;
    }

    public abstract class MovieEncoder : ScriptableObject
    {
        public enum Type
        {
            Png,
            Exr,
            Gif,
            WebM,
            MP4,
        }

        public abstract Type type { get; }

        // config: config struct (fcGifConfig, fcWebMConfig, etc)
        public abstract void Initialize(object config, string outPath);
        public abstract void Release();
        public abstract void AddVideoFrame(byte[] frame, fcAPI.fcPixelFormat format, double timestamp = -1.0);
        public abstract void AddAudioFrame(float[] samples, double timestamp = -1.0);


        public static MovieEncoder Create(Type t)
        {
            switch (t)
            {
                case Type.Gif: return CreateInstance<GifEncoder>();
                case Type.WebM: return CreateInstance<WebMEncoder>();
                case Type.MP4: return CreateInstance<MP4Encoder>();
            }
            return null;
        }
    }
}
