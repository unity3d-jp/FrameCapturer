using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{

    [AddComponentMenu("UTJ/FrameCapturer/Image Sequence Recorder")]
    [RequireComponent(typeof(Camera))]
    public abstract class ImageSequenceRecorder : MonoBehaviour
    {
        public enum CaptureTarget
        {
            FrameBuffer,
            RenderTexture,
        }
        
        [Serializable]
        public struct FrameBufferConponents
        {
            public bool FrameBuffer;
            public bool GBuffer;
            public bool  Albedo;
            public bool  Occlusion;
            public bool  Specular;
            public bool  Smoothness;
            public bool  Normal;
            public bool  Emission;
            public bool  Depth;

            public static FrameBufferConponents default_value
            {
                get
                {
                    var ret = default(FrameBufferConponents);
                    return ret;
                }
            }

        }


        [SerializeField] public ImageSequenceRecorderContext.Type m_format = ImageSequenceRecorderContext.Type.Exr;
        [SerializeField] public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "");
        [SerializeField] public CaptureTarget m_captureTarget = CaptureTarget.FrameBuffer;
        [SerializeField] public FrameBufferConponents m_fbComponents = FrameBufferConponents.default_value;
        [SerializeField] public RenderTexture[] m_targetRT;
        [SerializeField] public bool m_fixDeltaTime = true;
        [SerializeField] public int m_targetFramerate = 30;
        [SerializeField] public int m_beginFrame = 0;
        [SerializeField] public int m_endFrame = 0;

        //[SerializeField] protected ImageSequenceRecorderContext m_ctx;
        [SerializeField] protected Shader m_shCopy;
        protected Material m_matCopy;
        protected Mesh m_quad;
        protected CommandBuffer m_cbCopyFB;
        protected CommandBuffer m_cbCopyGB;
        protected CommandBuffer m_cbCopyRT;
        protected RenderTexture m_rtFB;
        protected RenderTexture[] m_rtGB;
        protected RenderTexture[] m_rtScratch;

#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = fcAPI.GetFrameBufferCopyShader();
        }

        void OnValidate()
        {
            m_beginFrame = Mathf.Max(1, m_beginFrame);
        }
#endif // UNITY_EDITOR

    }

}
