using System;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public abstract class ImageSequenceRecorderContext : ScriptableObject
    {
        public enum Type
        {
            Png,
            Exr,
        }


        public abstract Type type { get; }

        public abstract void Initialize(ImageSequenceRecorder recorder);
        public abstract void Release();

        public abstract void AddCommand(CommandBuffer cb, RenderTexture frame, int channels, string name);
        public abstract void Update();


        public static ImageSequenceRecorderContext Create(Type t)
        {
            switch (t)
            {
                case Type.Png: return CreateInstance<PngContext>();
                case Type.Exr: return CreateInstance<ExrContext>();
            }
            return null;
        }
    }
}
