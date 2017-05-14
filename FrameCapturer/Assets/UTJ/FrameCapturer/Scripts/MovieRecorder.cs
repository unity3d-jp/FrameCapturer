using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif


namespace UTJ.FrameCapturer
{
    [AddComponentMenu("UTJ/FrameCapturer/Movie Recorder")]
    [RequireComponent(typeof(Camera))]
    [ExecuteInEditMode]
    public class MovieRecorder : RecorderBase
    {
        #region inner_types
        public enum CaptureTarget
        {
            FrameBuffer,
            RenderTexture,
        }
        #endregion


        #region fields
        [SerializeField] MovieEncoderConfigs m_encoderConfigs = new MovieEncoderConfigs(MovieEncoder.Type.WebM);
        [SerializeField] int m_resolutionWidth = -1;
        [SerializeField] CaptureTarget m_captureTarget = CaptureTarget.FrameBuffer;
        [SerializeField] RenderTexture m_targetRT;

        [SerializeField] Shader m_shCopy;
        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratchBuffer;
        MovieEncoder m_encoder;
        #endregion


        #region properties
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

        public MovieEncoderConfigs encoderConfigs { get { return m_encoderConfigs; } }

        public RenderTexture scratchBuffer { get { return m_scratchBuffer; } }
        public CommandBuffer commandBuffer { get { return m_cb; } }
        #endregion


        public override bool BeginRecording()
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

            m_initialTime = Time.unscaledTime;
            m_recordedFrames = 0;
            m_recordedSamples = 0;
            m_recording = true;

            Debug.Log("MovieRecorder: BeginRecording()");
            return true;
        }

        public override void EndRecording()
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
                m_aborted = true;
                Debug.Log("MovieRecorder: EndRecording()");
            }
        }


        #region impl
#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = fcAPI.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        IEnumerator OnPostRender()
        {
            if (m_recording && m_encoder != null && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                double timestamp = Time.unscaledTime - m_initialTime;
                if (m_framerateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_targetFramerate * m_recordedFrames;
                }

                fcAPI.fcLock(m_scratchBuffer, TextureFormat.RGB24, (data, fmt) =>
                {
                    m_encoder.AddVideoFrame(data, fmt, timestamp);
                });
                ++m_recordedFrames;
            }
            ++m_frame;
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_encoder != null)
            {
                m_encoder.AddAudioSamples(samples);
                m_recordedSamples += samples.Length;
            }
        }
        #endregion
    }
}
