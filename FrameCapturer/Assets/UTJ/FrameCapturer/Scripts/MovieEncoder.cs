using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [Serializable]
    public class MovieEncoderConfigs
    {
        public MovieEncoder.Type format = MovieEncoder.Type.WebM;
        public fcAPI.fcPngConfig pngEncoderSettings = fcAPI.fcPngConfig.default_value;
        public fcAPI.fcExrConfig exrEncoderSettings = fcAPI.fcExrConfig.default_value;
        public fcAPI.fcGifConfig gifEncoderSettings = fcAPI.fcGifConfig.default_value;
        public fcAPI.fcWebMConfig webmEncoderSettings = fcAPI.fcWebMConfig.default_value;
        public fcAPI.fcMP4Config mp4EncoderSettings = fcAPI.fcMP4Config.default_value;

        public void SetResolution(int w, int h, int ch = 4)
        {
            pngEncoderSettings.width =
            exrEncoderSettings.width =
            gifEncoderSettings.width = 
            webmEncoderSettings.videoWidth =
            mp4EncoderSettings.videoWidth = w;

            pngEncoderSettings.height =
            exrEncoderSettings.height =
            gifEncoderSettings.height =
            webmEncoderSettings.videoHeight =
            mp4EncoderSettings.videoHeight = h;

            pngEncoderSettings.channels =
            exrEncoderSettings.channels = ch;
        }
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
                case Type.Png: return CreateInstance<PngEncoder>();
                case Type.Exr: return CreateInstance<ExrEncoder>();
                case Type.Gif: return CreateInstance<GifEncoder>();
                case Type.WebM: return CreateInstance<WebMEncoder>();
                case Type.MP4: return CreateInstance<MP4Encoder>();
            }
            return null;
        }

        public static MovieEncoder Create(MovieEncoderConfigs c, string path)
        {
            var ret = Create(c.format);
            switch (c.format)
            {
                case Type.Png: ret.Initialize(c.pngEncoderSettings, path); break;
                case Type.Exr: ret.Initialize(c.exrEncoderSettings, path); break;
                case Type.Gif: ret.Initialize(c.gifEncoderSettings, path); break;
                case Type.WebM:ret.Initialize(c.webmEncoderSettings, path); break;
                case Type.MP4: ret.Initialize(c.mp4EncoderSettings, path); break;
            }
            return ret;
        }
    }
}
