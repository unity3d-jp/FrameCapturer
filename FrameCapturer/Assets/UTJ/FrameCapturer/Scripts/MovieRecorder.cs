using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [AddComponentMenu("UTJ/FrameCapturer/Movie Recorder")]
    [RequireComponent(typeof(Camera))]
    public class MovieRecorder : MonoBehaviour
    {
        #region inner_types
        public enum CaptureTarget
        {
            FrameBuffer,
            RenderTexture,
        }

        public enum FrameRateMode
        {
            Variable,
            Constant,
        }

        public enum CaptureControl
        {
            Manual,
            SelectedRange,
        }
        #endregion


        #region fields
        // base settings
        [SerializeField] DataPath m_outputDir = new DataPath(DataPath.Root.Current, "Capture");
        [SerializeField] MovieRecorderContext.Type m_format = MovieRecorderContext.Type.WebM;

        // video settings
        [SerializeField] CaptureTarget m_captureTarget = CaptureTarget.FrameBuffer;
        [SerializeField] RenderTexture m_targetRT;
        [SerializeField] int m_resolutionWidth = -1;
        [SerializeField] FrameRateMode m_framerateMode = FrameRateMode.Constant;
        [SerializeField] int m_targetFramerate = 30;
        [SerializeField] bool m_fixDeltaTime = true;
        [SerializeField] int m_captureEveryNthFrame = 1;

        // capture control
        [SerializeField] CaptureControl m_captureControl = CaptureControl.SelectedRange;
        [SerializeField] int m_startFrame = 0;
        [SerializeField] int m_endFrame = 100;

        // internal
        [SerializeField] MovieRecorderContext m_ctx;
        [SerializeField] GifContext.EncoderConfig m_gifEncoderConfig = new GifContext.EncoderConfig();
        [SerializeField] WebMContext.EncoderConfig m_webmEncoderConfig = new WebMContext.EncoderConfig();
        [SerializeField] MP4Context.EncoderConfig m_mp4EncoderConfig = new MP4Context.EncoderConfig();
        [SerializeField] Shader m_shCopy;

        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratchBuffer;
        bool m_recording = false;
        int m_numVideoFrames = 0;
        #endregion


        #region properties
        public DataPath outputDir
        {
            get { return m_outputDir; }
            set { m_outputDir = value; }
        }
        public MovieRecorderContext.Type format {
            get { return m_format; }
            set { m_format = value; ValidateContext(); }
        }
        public CaptureTarget captureTarget
        {
            get { return m_captureTarget; }
            set { m_captureTarget = value; }
        }
        public RenderTexture targetRT
        {
            get { return m_targetRT; }
            set { m_targetRT = value; }
        }
        public FrameRateMode framerateMode
        {
            get { return m_framerateMode; }
            set { m_framerateMode = value; }
        }
        public int targetFramerate
        {
            get { return m_targetFramerate; }
            set { m_targetFramerate = value; }
        }
        public bool fixDeltaTime
        {
            get { return m_fixDeltaTime; }
            set { m_fixDeltaTime = value; }
        }
        public int captureEveryNthFrame
        {
            get { return m_captureEveryNthFrame; }
            set { m_captureEveryNthFrame = value; }
        }

        public CaptureControl captureControl
        {
            get { return m_captureControl; }
            set { m_captureControl = value; }
        }
        public int startFrame
        {
            get { return m_startFrame; }
            set { m_startFrame = value; }
        }
        public int endFrame
        {
            get { return m_endFrame; }
            set { m_endFrame = value; }
        }

        public MovieRecorderContext context { get { ValidateContext(); return m_ctx; } }
        public GifContext.EncoderConfig gifConfig { get { return m_gifEncoderConfig; } }
        public WebMContext.EncoderConfig webmConfig { get { return m_webmEncoderConfig; } }
        public MP4Context.EncoderConfig mp4Config { get { return m_mp4EncoderConfig; } }

        public RenderTexture scratchBuffer { get { return m_scratchBuffer; } }
        public CommandBuffer commandBuffer { get { return m_cb; } }
        public bool isRecording { get { return m_recording; } }
        #endregion


        public bool BeginRecording()
        {
            if (m_recording) { return false; }
            if (m_shCopy == null)
            {
                Debug.LogError("MovieRecorder: copy shader is missing!");
                return false;
            }
            if (m_captureTarget == CaptureTarget.RenderTexture && m_targetRT == null)
            {
                Debug.LogError("MovieRecorder: target RenderTexture is null!");
                return false;
            }

            ValidateContext();
            if (m_ctx == null) { return false; }

            m_recording = true;

            m_outputDir.CreateDirectory();
            if (m_quad == null) m_quad = fcAPI.CreateFullscreenQuad();
            if (m_matCopy == null) m_matCopy = new Material(m_shCopy);

            var cam = GetComponent<Camera>();
            if (cam.targetTexture != null)
            {
                m_matCopy.EnableKeyword("OFFSCREEN");
            }


            m_numVideoFrames = 0;

            // create scratch buffer
            {
                int targetWidth = cam.pixelWidth;
                int targetHeight = cam.pixelHeight;
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

                if( m_ctx.type == MovieRecorderContext.Type.MP4 ||
                    m_ctx.type == MovieRecorderContext.Type.WebM)
                {
                    captureWidth = (captureWidth + 1) & ~1;
                    captureHeight = (captureHeight + 1) & ~1;
                }

                m_scratchBuffer = new RenderTexture(captureWidth, captureHeight, 0, RenderTextureFormat.ARGB32);
                m_scratchBuffer.wrapMode = TextureWrapMode.Repeat;
                m_scratchBuffer.Create();
            }

            // create command buffer
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");
                m_cb = new CommandBuffer();
                m_cb.name = "MovieRecorder: copy frame buffer";

                if(m_captureTarget == CaptureTarget.FrameBuffer)
                {
                    m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Bilinear);
                    m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                    m_cb.SetRenderTarget(m_scratchBuffer);
                    m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                    m_cb.ReleaseTemporaryRT(tid);
                }
                else if(m_captureTarget == CaptureTarget.RenderTexture)
                {
                    m_cb.SetRenderTarget(m_scratchBuffer);
                    m_cb.SetGlobalTexture("_TmpRenderTarget", m_targetRT);
                    m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 1);
                }
            }

            m_ctx.Initialize(this);

            cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
            Debug.Log("MovieMRecorder: BeginRecording()");
            return true;
        }

        public void EndRecording()
        {
            if (!m_recording) { return; }
            m_recording = false;

            ReleaseContext();
            if (m_cb != null)
            {
                GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
                m_cb.Release();
                m_cb = null;
            }
            if (m_scratchBuffer != null)
            {
                m_scratchBuffer.Release();
                m_scratchBuffer = null;
            }
            Debug.Log("MovieMRecorder: EndRecording()");
        }


        #region impl
        void ReleaseContext()
        {
            if (m_ctx != null)
            {
                m_ctx.Release();
                m_ctx = null;
            }
        }

        bool CreateContext()
        {
            m_ctx = MovieRecorderContext.Create(m_format);
            return m_ctx != null;
        }

        void ValidateContext()
        {
            if(m_recording) { return; }
            if(m_ctx == null)
            {
                CreateContext();
            }
            else
            {
                if(m_ctx.type != m_format)
                {
                    ReleaseContext();
                    CreateContext();
                }
            }
        }

        IEnumerator Wait()
        {
            yield return new WaitForEndOfFrame();

            // wait until current dt reaches target dt
            float wt = Time.maximumDeltaTime;
            while (Time.realtimeSinceStartup - Time.unscaledTime < wt)
            {
                System.Threading.Thread.Sleep(1);
            }
        }


#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = fcAPI.GetFrameBufferCopyShader();
            ValidateContext();
        }

        void OnValidate()
        {
            m_startFrame = Mathf.Max(0, m_startFrame);
            m_endFrame = Mathf.Max(m_startFrame, m_endFrame);
        }

#endif // UNITY_EDITOR

        void OnDisable()
        {
            EndRecording();
        }

        void Update()
        {
            int frame = Time.frameCount;
            if (m_captureControl == CaptureControl.SelectedRange)
            {
                if (frame >= m_startFrame && frame <= m_endFrame)
                {
                    if (!m_recording) { BeginRecording(); }
                }
                else if (m_recording)
                {
                    EndRecording();
                }
            }
            else if (m_captureControl == CaptureControl.Manual)
            {
            }

            if (m_fixDeltaTime)
            {
                Time.maximumDeltaTime = (1.0f / m_targetFramerate);
                StartCoroutine(Wait());
            }
        }

        IEnumerator OnPostRender()
        {
            if (m_recording && m_ctx != null && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                double timestamp = Time.unscaledTime;
                if (m_framerateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_targetFramerate * m_numVideoFrames;
                }

                fcAPI.fcLock(m_scratchBuffer, TextureFormat.RGB24, (data, fmt) => {
                    m_ctx.AddVideoFrame(data, fmt, timestamp);
                });
                m_numVideoFrames++;
            }
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_ctx != null)
            {
                m_ctx.AddAudioFrame(samples);
            }
        }
        #endregion
    }
}
