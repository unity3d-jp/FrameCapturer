using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{

    public abstract class ImageSequenceRecorderBase : MonoBehaviour
    {
        [SerializeField] protected DataPath m_outputDir = new DataPath(DataPath.Root.PersistentDataPath, "");
        [SerializeField] protected bool m_fixDeltaTime = false;
        [SerializeField] protected int m_framerate = 30;

        public abstract int beginFrame { get; set; }
        public abstract int endFrame { get; set; }
    }

    public abstract class MovieRecorderBase : MonoBehaviour
    {
        public enum FrameRateMode
        {
            Variable,
            Constant,
        }

        [SerializeField] public DataPath m_outputDir = new DataPath(DataPath.Root.PersistentDataPath, "");

        [SerializeField] public bool m_captureVideo = true;
        [SerializeField] public int m_resolutionWidth = 640;
        [SerializeField] public int m_videoBitrate = 8192000;
        [SerializeField] public FrameRateMode m_frameRateMode = FrameRateMode.Variable;
        [SerializeField] public int m_framerate = 30;
        [SerializeField] public bool m_fixDeltaTime = false;
        [SerializeField] public int m_captureEveryNthFrame = 1;

        [SerializeField] public bool m_captureAudio = true;
        [SerializeField] public int m_audioBitrate = 64000;

        [SerializeField] protected Shader m_shCopy;
        protected Material m_matCopy;
        protected Mesh m_quad;
        protected CommandBuffer m_cb;
        protected RenderTexture m_scratchBuffer;
        protected string m_outputPath;
        protected bool m_recording = false;


        public bool isRecording { get { return m_recording; } }
        public string outputPath { get { return m_outputPath; } }


        public abstract bool BeginRecording();
        public abstract bool EndRecording();


        #region impl
        protected void UpdateScratchBuffer(int targetWidth, int targetHeight)
        {
            int captureWidth = targetWidth;
            int captureHeight = targetHeight;

            if (m_resolutionWidth > 0)
            {
                captureWidth = m_resolutionWidth;
                captureHeight = (int)((float)m_resolutionWidth / ((float)targetWidth / (float)targetHeight));
            }
            else if (m_resolutionWidth < 0)
            {
                int div = System.Math.Abs(m_resolutionWidth);
                captureWidth = targetWidth / div;
                captureHeight = targetHeight / div;
            }


            if (m_scratchBuffer != null)
            {
                if (m_scratchBuffer.IsCreated() &&
                    m_scratchBuffer.width == captureWidth && m_scratchBuffer.height == captureHeight)
                {
                    // update is not needed
                    return;
                }
                else
                {
                    ReleaseScratchBuffer();
                }
            }

            m_scratchBuffer = new RenderTexture(captureWidth, captureHeight, 0, RenderTextureFormat.ARGB32);
            m_scratchBuffer.wrapMode = TextureWrapMode.Repeat;
            m_scratchBuffer.Create();
        }

        protected void ReleaseScratchBuffer()
        {
            if (m_scratchBuffer != null)
            {
                m_scratchBuffer.Release();
                m_scratchBuffer = null;
            }
        }

#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
#if UNITY_EDITOR
            if (m_captureAudio && m_frameRateMode == FrameRateMode.Constant)
            {
                Debug.LogWarning("MovieRecorder: capture audio with Constant frame rate mode will cause desync");
            }
#endif
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_matCopy = new Material(m_shCopy);

            if (GetComponent<Camera>().targetTexture != null)
            {
                m_matCopy.EnableKeyword("OFFSCREEN");
            }
        }

        void OnDisable()
        {
            EndRecording();
            ReleaseScratchBuffer();
        }

        #endregion
    }
}
