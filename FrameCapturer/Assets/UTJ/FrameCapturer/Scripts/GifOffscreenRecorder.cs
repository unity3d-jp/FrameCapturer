using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/GifOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class GifOffscreenRecorder : IMovieRecorder
    {
        public enum FrameRateMode
        {
            Variable,
            Constant,
        }

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.PersistentDataPath, "");
        public RenderTexture m_target;
        public int m_resolutionWidth = 300;
        public int m_numColors = 256;
        public FrameRateMode m_frameRateMode = FrameRateMode.Constant;
        [Tooltip("relevant only if FrameRateMode is Constant")]
        public int m_framerate = 30;
        public int m_captureEveryNthFrame = 2;
        public int m_keyframe = 30;
        [Tooltip("0 is treated as processor count")]
        public Shader m_shCopy;

        string m_output_file;
        fcAPI.fcGIFContext m_ctx;
        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratch_buffer;
        int m_callback;
        int m_num_video_frames;
        bool m_recording = false;



        void InitializeContext()
        {
            m_num_video_frames = 0;

            // initialize scratch buffer
            UpdateScratchBuffer();

            // initialize context and stream
            {
                fcAPI.fcGifConfig conf;
                conf.width = m_scratch_buffer.width;
                conf.height = m_scratch_buffer.height;
                conf.num_colors = Mathf.Clamp(m_numColors, 1, 255);
                conf.max_active_tasks = 0;
                m_ctx = fcAPI.fcGifCreateContext(ref conf);
            }

            // initialize command buffer
            {
                m_cb = new CommandBuffer();
                m_cb.name = "GifOffscreenRecorder: copy frame buffer";
                m_cb.SetRenderTarget(m_scratch_buffer);
                m_cb.SetGlobalTexture("_TmpRenderTarget", m_target);
                m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 3);
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

                if (m_ctx.ptr != IntPtr.Zero)
                {
                    fcAPI.fcGifDestroyContext(m_ctx);
                    m_ctx.ptr = IntPtr.Zero;
                }
            });
        }


        void UpdateScratchBuffer()
        {
            var cam = GetComponent<Camera>();
            int capture_width = m_resolutionWidth;
            int capture_height = (int)((float)m_resolutionWidth / ((float)cam.pixelWidth / (float)cam.pixelHeight));

            if (m_scratch_buffer != null)
            {
                if (m_scratch_buffer.IsCreated() &&
                    m_scratch_buffer.width == capture_width && m_scratch_buffer.height == capture_height)
                {
                    // update is not needed
                    return;
                }
                else
                {
                    ReleaseScratchBuffer();
                }
            }

            m_scratch_buffer = new RenderTexture(capture_width, capture_height, 0, RenderTextureFormat.ARGB32);
            m_scratch_buffer.wrapMode = TextureWrapMode.Repeat;
            m_scratch_buffer.Create();
        }

        void ReleaseScratchBuffer()
        {
            if (m_scratch_buffer != null)
            {
                m_scratch_buffer.Release();
                m_scratch_buffer = null;
            }
        }


        public override bool IsSeekable() { return true; }
        public override bool IsEditable() { return true; }

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

        public override bool recording
        {
            get { return m_recording; }
            set { m_recording = value; }
        }

        public override string GetOutputPath()
        {
            return m_outputDir.GetPath() + "/" + m_output_file;
        }

        public override bool Flush()
        {
            return Flush(0, -1);
        }

        public override bool Flush(int begin_frame, int end_frame)
        {
            bool ret = false;
            if (m_ctx.ptr != IntPtr.Zero && m_num_video_frames > 0)
            {
                m_output_file = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
                var path = GetOutputPath();
                fcAPI.fcGuard(() =>
                {
                    ret = fcAPI.fcGifWriteFile(m_ctx, path, begin_frame, end_frame);
                });
                Debug.Log("GifOffscreenRecorder.FlushFile(" + begin_frame + ", " + end_frame + "): " + path);
            }
            return ret;
        }

        public override RenderTexture GetScratchBuffer() { return m_scratch_buffer; }

        public override void EraseFrame(int begin_frame, int end_frame)
        {
            fcAPI.fcGifEraseFrame(m_ctx, begin_frame, end_frame);
        }

        public override int GetExpectedFileSize(int begin_frame = 0, int end_frame = -1)
        {
            return fcAPI.fcGifGetExpectedDataSize(m_ctx, begin_frame, end_frame);
        }

        public override int GetFrameCount()
        {
            return fcAPI.fcGifGetFrameCount(m_ctx);
        }

        public override void GetFrameData(RenderTexture rt, int frame)
        {
            fcAPI.fcGifGetFrameData(m_ctx, rt.GetNativeTexturePtr(), frame);
        }

        public fcAPI.fcGIFContext GetGifContext() { return m_ctx; }


#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }

        void OnValidate()
        {
            m_numColors = Mathf.Clamp(m_numColors, 1, 256);
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_shCopy);

            if (GetComponent<Camera>().targetTexture != null)
            {
                m_mat_copy.EnableKeyword("OFFSCREEN");
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

                bool keyframe = m_keyframe > 0 && m_num_video_frames % m_keyframe == 0;
                double timestamp = Time.unscaledTime;
                if (m_frameRateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_framerate * m_num_video_frames;
                }

                m_callback = fcAPI.fcGifAddFrameTexture(m_ctx, m_scratch_buffer, keyframe, timestamp, m_callback);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callback);
                m_num_video_frames++;
            }
        }
    }

}
