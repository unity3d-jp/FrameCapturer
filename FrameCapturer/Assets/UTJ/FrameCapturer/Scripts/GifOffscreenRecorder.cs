using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/GifOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class GifOffscreenRecorder : MovieRecorderBase
    {
        public RenderTexture m_target;
        public int m_numColors = 256;
        public int m_keyframe = 30;

        fcAPI.fcGIFContext m_ctx;
        fcAPI.fcStream m_ostream;
        int m_callback;
        int m_numVideoFrames;



        void InitializeContext()
        {
            m_numVideoFrames = 0;

            // initialize scratch buffer
            var cam = GetComponent<Camera>();
            UpdateScratchBuffer(cam.pixelWidth, cam.pixelHeight);

            // initialize context and stream
            {
                fcAPI.fcGifConfig conf;
                conf.width = m_scratchBuffer.width;
                conf.height = m_scratchBuffer.height;
                conf.num_colors = Mathf.Clamp(m_numColors, 1, 256);
                conf.max_active_tasks = 0;
                m_ctx = fcAPI.fcGifCreateContext(ref conf);

                m_outputPath = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
                m_ostream = fcAPI.fcCreateFileStream(outputPath);
                fcAPI.fcGifAddOutputStream(m_ctx, m_ostream);
            }

            // initialize command buffer
            {
                m_cb = new CommandBuffer();
                m_cb.name = "GifOffscreenRecorder: copy frame buffer";
                m_cb.SetRenderTarget(m_scratchBuffer);
                m_cb.SetGlobalTexture("_TmpRenderTarget", m_target);
                m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 3);
            }
        }

        void ReleaseContext()
        {
            if (m_cb != null)
            {
                m_cb.Release();
                m_cb = null;
            }

            // scratch buffer is kept

            fcAPI.fcGuard(() =>
            {
                fcAPI.fcEraseDeferredCall(m_callback);
                m_callback = 0;

                if (m_ctx)
                {
                    fcAPI.fcGifDestroyContext(m_ctx);
                    m_ctx.ptr = IntPtr.Zero;
                }
                if (m_ostream)
                {
                    fcAPI.fcDestroyStream(m_ostream);
                    m_ostream.ptr = IntPtr.Zero;
                }
            });
        }


        public override bool BeginRecording()
        {
            if (m_recording) { return false; }
            m_recording = true;

            InitializeContext();
            GetComponent<Camera>().AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
            Debug.Log("GifOffscreenRecorder.BeginRecording()");
            return true;
        }

        public override bool EndRecording()
        {
            if (!m_recording) { return false; }
            m_recording = false;

            GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            ReleaseContext();
            Debug.Log("GifOffscreenRecorder.EndRecording()");
            return true;
        }


#if UNITY_EDITOR
        void OnValidate()
        {
            m_numColors = Mathf.Clamp(m_numColors, 1, 256);
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
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
            ReleaseContext();
            ReleaseScratchBuffer();
        }

        IEnumerator OnPostRender()
        {
            if (m_recording && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                bool keyframe = m_keyframe > 0 && m_numVideoFrames % m_keyframe == 0;
                double timestamp = Time.unscaledTime;
                if (m_frameRateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_framerate * m_numVideoFrames;
                }

                m_callback = fcAPI.fcGifAddFrameTexture(m_ctx, m_scratchBuffer, keyframe, timestamp, m_callback);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callback);
                m_numVideoFrames++;
            }
        }
    }

}
