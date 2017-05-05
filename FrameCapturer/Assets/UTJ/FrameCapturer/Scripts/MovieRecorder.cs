using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/Movie Recorder")]
    [RequireComponent(typeof(Camera))]
    public class MovieRecorder : MonoBehaviour
    {
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

        // base settings
        [SerializeField] public MovieRecorderContext.Type m_format = MovieRecorderContext.Type.WebM;
        [SerializeField] public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "");

        // video settings
        [SerializeField] public bool m_captureVideo = true;
        [SerializeField] public CaptureTarget m_captureTarget = CaptureTarget.FrameBuffer;
        [SerializeField] public RenderTexture m_targetRT;
        [SerializeField] public int m_resolutionWidth = -1;
        [SerializeField] public int m_videoBitrate = 8192000;
        [SerializeField] public FrameRateMode m_frameRateMode = FrameRateMode.Variable;
        [SerializeField] public int m_framerate = 30;
        [SerializeField] public bool m_fixDeltaTime = false;
        [SerializeField] public int m_captureEveryNthFrame = 1;

        // audio settings
        [SerializeField] public bool m_captureAudio = true;
        [SerializeField] public int m_audioBitrate = 64000;

        // internal
        [SerializeField] MovieRecorderContext m_ctx;
        [SerializeField] Shader m_shCopy;

        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratchBuffer;
        bool m_recording = false;
        int m_numVideoFrames = 0;


        public MovieRecorderContext.Type format {
            get { return m_format; }
            set { m_format = value; ValidateContext(); }
        }
        public bool captureVideo { get { return m_captureVideo; } }
        public CaptureTarget captureTarget { get { return m_captureTarget; } }
        public bool captureAudio { get { return m_captureAudio; } }
        public bool isRecording { get { return m_recording; } }
        public DataPath outputDir { get { return m_outputDir; } }
        public RenderTexture scratchBuffer { get { return m_scratchBuffer; } }


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

            if (m_ctx == null)
            {
                CreateContext();
                if (m_ctx == null) { return false; }
            }

            m_recording = true;

#if UNITY_EDITOR
            if (m_captureAudio && m_frameRateMode == FrameRateMode.Constant)
            {
                Debug.LogWarning("MovieRecorder: capture audio with Constant frame rate mode will cause desync");
            }
#endif
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
                    m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 3);
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

#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = fcAPI.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        void OnDisable()
        {
            EndRecording();
        }


        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_captureAudio && m_ctx != null)
            {
                m_ctx.AddAudioFrame(samples);
            }
        }

        IEnumerator OnPostRender()
        {
            if (m_recording && m_captureVideo && m_ctx != null && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                double timestamp = Time.unscaledTime;
                if (m_frameRateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_framerate * m_numVideoFrames;
                }

                int cb = m_ctx.AddVideoFrame(m_scratchBuffer, timestamp);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), cb);
                m_numVideoFrames++;
            }
        }
        #endregion
    }
}
