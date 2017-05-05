using System;
using UnityEngine;


namespace UTJ
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
        public abstract bool supportAudio { get; }

        public abstract void Initialize(MovieRecorder recorder);
        public abstract void Release();
        public abstract int AddVideoFrame(RenderTexture frame, double timestamp = -1.0);
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
