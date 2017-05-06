using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public abstract class MovieRecorderContext : ScriptableObject
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

        public static MovieRecorderContext Create(Type t)
        {
            switch (t)
            {
                case Type.Gif: return CreateInstance<GifContext>();
                case Type.WebM: return CreateInstance<WebMContext>();
                case Type.MP4: return CreateInstance<MP4Context>();
            }
            return null;
        }
    }
}
