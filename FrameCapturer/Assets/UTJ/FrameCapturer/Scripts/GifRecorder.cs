using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/GifRecorder")]
    [RequireComponent(typeof(Camera))]
    public class GifRecorder : IEditableMovieRecorder
    {
        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.PersistentDataPath, "");
        public int m_resolutionWidth = 300;
        public int m_numColors = 255;
        public int m_captureEveryNthFrame = 2;
        public int m_intervalCS = 3;
        public int m_maxFrame = 1800;
        public int m_maxSize = 0;
        public int m_maxTasks = 0;
        public int m_keyframe = 0;
        public Shader m_sh_copy;

        string m_output_file;
        fcAPI.fcGIFContext m_gif;
        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratch_buffer;
        Camera m_cam;
        int m_frame;
        bool m_recode = false;

        public override bool record
        {
            get { return m_recode; }
            set { m_recode = value; }
        }

        public override string GetOutputPath()
        {
            return m_outputDir.GetPath() + "/" + m_output_file;
        }

        public override bool FlushFile()
        {
            return FlushFile(0, -1);
        }

        public override bool FlushFile(int begin_frame, int end_frame)
        {
            bool ret = false;
            if (m_gif.ptr != IntPtr.Zero)
            {
                m_output_file = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
                var path = GetOutputPath();
                ret = fcAPI.fcGifWriteFile(m_gif, path, begin_frame, end_frame);
                Debug.Log("GifCapturer.FlushFile(" + begin_frame + ", " + end_frame + "): " + path);
            }
            return ret;
        }

        public override RenderTexture GetScratchBuffer() { return m_scratch_buffer; }

        public override void ResetRecordingState()
        {
            fcAPI.fcGifDestroyContext(m_gif);
            m_gif.ptr = IntPtr.Zero;
            if (m_scratch_buffer != null)
            {
                m_scratch_buffer.Release();
                m_scratch_buffer = null;
            }

            int capture_width = m_resolutionWidth;
            int capture_height = (int)(m_resolutionWidth / ((float)m_cam.pixelWidth / (float)m_cam.pixelHeight));
            m_scratch_buffer = new RenderTexture(capture_width, capture_height, 0, RenderTextureFormat.ARGB32);
            m_scratch_buffer.wrapMode = TextureWrapMode.Repeat;
            m_scratch_buffer.Create();

            m_frame = 0;
            if (m_maxTasks <= 0)
            {
                m_maxTasks = SystemInfo.processorCount;
            }
            fcAPI.fcGifConfig conf;
            conf.width = m_scratch_buffer.width;
            conf.height = m_scratch_buffer.height;
            conf.num_colors = m_numColors;
            conf.max_active_tasks = m_maxTasks;
            m_gif = fcAPI.fcGifCreateContext(ref conf);
        }

        public override void EraseFrame(int begin_frame, int end_frame)
        {
            fcAPI.fcGifEraseFrame(m_gif, begin_frame, end_frame);
        }

        public override int GetExpectedFileSize(int begin_frame = 0, int end_frame = -1)
        {
            return fcAPI.fcGifGetExpectedDataSize(m_gif, begin_frame, end_frame);
        }

        public override int GetFrameCount()
        {
            return fcAPI.fcGifGetFrameCount(m_gif);
        }

        public override void GetFrameData(RenderTexture rt, int frame)
        {
            fcAPI.fcGifGetFrameData(m_gif, rt.GetNativeTexturePtr(), frame);
        }

        public fcAPI.fcGIFContext GetGifContext() { return m_gif; }

#if UNITY_EDITOR
        void Reset()
        {
            m_sh_copy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }

        void OnValidate()
        {
            m_numColors = Mathf.Clamp(m_numColors, 1, 255);
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
            m_cam = GetComponent<Camera>();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_sh_copy);
            if (m_cam.targetTexture != null)
            {
                m_mat_copy.EnableKeyword("OFFSCREEN");
            }

            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");
                m_cb = new CommandBuffer();
                m_cb.name = "GifCapturer: copy frame buffer";
                m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                // tid は意図的に開放しない
                m_cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
            }
            ResetRecordingState();
        }

        void OnDisable()
        {
            fcAPI.fcGifDestroyContext(m_gif);
            m_gif.ptr = IntPtr.Zero;

            m_cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            m_cb.Release();
            m_cb = null;

            m_scratch_buffer.Release();
            m_scratch_buffer = null;
        }

        IEnumerator OnPostRender()
        {
            if (m_recode)
            {
                yield return new WaitForEndOfFrame();

                int frame = m_frame++;
                if (frame % m_captureEveryNthFrame == 0)
                {
                    m_mat_copy.SetPass(0);
                    Graphics.SetRenderTarget(m_scratch_buffer);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);
                    fcAPI.fcGifAddFrame(m_gif, m_scratch_buffer.GetNativeTexturePtr());
                }
            }
        }
    }

}
