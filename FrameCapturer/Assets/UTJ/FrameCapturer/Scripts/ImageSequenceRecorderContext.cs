using System;
using UnityEngine;


namespace UTJ
{
    public abstract class ImageSequenceRecorderContext : ScriptableObject
    {
        public enum Type
        {
            Png,
            Exr,
        }


        public abstract void Initialize(ImageSequenceRecorder recorder);
        public abstract void Release();

        public abstract void BeginFrame();
        public abstract void Export(RenderTexture frame, int channels, string path);
        public abstract void EndFrame();

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
