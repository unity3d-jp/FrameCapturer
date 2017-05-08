using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public abstract class ImageSequenceEncoder : ScriptableObject
    {
        public enum Type
        {
            Png,
            Exr,
        }


        public abstract Type type { get; }

        public abstract void Initialize(ImageSequenceRecorder recorder);
        public abstract void Release();
        public abstract void Export(RenderTexture frame, int channels, string name);



        public static ImageSequenceEncoder Create(Type t)
        {
            switch (t)
            {
                case Type.Png: return CreateInstance<PngEncoder>();
                case Type.Exr: return CreateInstance<ExrEncoder>();
            }
            return null;
        }
    }
}
