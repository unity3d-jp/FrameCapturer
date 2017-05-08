using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public abstract class MovieEncoder : ScriptableObject
    {
        public enum Type
        {
            Gif,
            WebM,
            MP4,
        }

        public abstract Type type { get; }

        public abstract void Initialize(MovieRecorder recorder);
        public abstract void Release();
        public abstract void AddVideoFrame(byte[] frame, fcAPI.fcPixelFormat format, double timestamp = -1.0);
        public virtual void AddVideoFrame(Texture2D tex, double timestamp = -1.0)
        {
            AddVideoFrame(tex.GetRawTextureData(), fcAPI.fcGetPixelFormat(tex.format), timestamp);
        }
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
