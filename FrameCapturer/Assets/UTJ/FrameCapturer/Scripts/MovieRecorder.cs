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
            SpecifiedRange,
        }
        #endregion


        #region fields
        // base settings
        [SerializeField] DataPath m_outputDir = new DataPath(DataPath.Root.Current, "Capture");
        [SerializeField] MovieEncoderConfigs m_encoderConfigs = new MovieEncoderConfigs(MovieEncoder.Type.WebM);

        // video settings
        [SerializeField] int m_resolutionWidth = -1;
        [SerializeField] CaptureTarget m_captureTarget = CaptureTarget.FrameBuffer;
        [SerializeField] RenderTexture m_targetRT;
        [SerializeField] FrameRateMode m_framerateMode = FrameRateMode.Constant;
        [SerializeField] int m_targetFramerate = 30;
        [SerializeField] bool m_fixDeltaTime = true;
        [SerializeField] int m_captureEveryNthFrame = 1;

        // capture control
        [SerializeField] CaptureControl m_captureControl = CaptureControl.SpecifiedRange;
        [SerializeField] int m_startFrame = 0;
        [SerializeField] int m_endFrame = 100;

        // internal
        [SerializeField] Shader m_shCopy;

        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratchBuffer;
        bool m_recording = false;
        int m_numVideoFrames = 0;

        MovieEncoder m_encoder;
        #endregion


        #region properties
        public DataPath outputDir
        {
            get { return m_outputDir; }
            set { m_outputDir = value; }
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
        public MovieEncoderConfigs encoderConfigs { get { return m_encoderConfigs; } }

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

            m_outputDir.CreateDirectory();
            if (m_quad == null) m_quad = fcAPI.CreateFullscreenQuad();
            if (m_matCopy == null) m_matCopy = new Material(m_shCopy);

            var cam = GetComponent<Camera>();
            if (cam.targetTexture != null)
            {
                m_matCopy.EnableKeyword("OFFSCREEN");
            }
            else
            {
                m_matCopy.DisableKeyword("OFFSCREEN");
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

                if (m_encoderConfigs.format == MovieEncoder.Type.MP4 ||
                    m_encoderConfigs.format == MovieEncoder.Type.WebM)
                {
                    captureWidth = (captureWidth + 1) & ~1;
                    captureHeight = (captureHeight + 1) & ~1;
                }

                m_scratchBuffer = new RenderTexture(captureWidth, captureHeight, 0, RenderTextureFormat.ARGB32);
                m_scratchBuffer.wrapMode = TextureWrapMode.Repeat;
                m_scratchBuffer.Create();
            }

            // initialize encoder
            {
                int targetFramerate = 60;
                if(m_framerateMode == FrameRateMode.Constant)
                {
                    targetFramerate = m_targetFramerate;
                }
                string outPath = m_outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss");

                m_encoderConfigs.Setup(m_scratchBuffer.width, m_scratchBuffer.height, 3, targetFramerate);
                m_encoder = MovieEncoder.Create(m_encoderConfigs, outPath);
                if (m_encoder == null)
                {
                    EndRecording();
                    return false;
                }
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

            cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);

            m_recording = true;
            Debug.Log("MovieRecorder: BeginRecording()");
            return true;
        }

        public void EndRecording()
        {
            if(m_encoder != null)
            {
                m_encoder.Release();
                m_encoder = null;
            }
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

            if(m_recording)
            {
                m_recording = false;
                Debug.Log("MovieRecorder: EndRecording()");
            }
        }


        #region impl
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
            if (m_captureControl == CaptureControl.SpecifiedRange)
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
            if (m_recording && m_encoder != null && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                double timestamp = Time.unscaledTime;
                if (m_framerateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_targetFramerate * m_numVideoFrames;
                }

                fcAPI.fcLock(m_scratchBuffer, TextureFormat.RGB24, (data, fmt) => {
                    m_encoder.AddVideoFrame(data, fmt, timestamp);
                });
                m_numVideoFrames++;
            }
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_encoder != null)
            {
                m_encoder.AddAudioFrame(samples);
            }
        }
        #endregion
    }
}
