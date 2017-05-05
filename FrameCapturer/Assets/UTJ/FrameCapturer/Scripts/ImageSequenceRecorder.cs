using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    public abstract class ImageSequenceRecorderImpl : ScriptableObject
    {
        public abstract void Initialize(ImageSequenceRecorder recorder);
        public abstract void Release();
        public abstract int AddVideoFrame(RenderTexture frame, double timestamp = -1.0);
    }


    public abstract class ImageSequenceRecorder : MonoBehaviour
    {
        [SerializeField] protected DataPath m_outputDir = new DataPath(DataPath.Root.PersistentDataPath, "");
        [SerializeField] protected bool m_fixDeltaTime = false;
        [SerializeField] protected int m_framerate = 30;

        public abstract int beginFrame { get; set; }
        public abstract int endFrame { get; set; }
    }

}
