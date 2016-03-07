using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/PngOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class PngOffscreenRecorder : IImageSequenceRecorder
    {
        public RenderTexture[] m_targets;

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "PngOutput");
        public string m_outputFilename = "RenderTarget";
        public int m_beginFrame = 1;
        public int m_endFrame = 100;
        public bool m_fillAlpha = false;
        public Shader m_sh_copy;

        fcAPI.fcPNGContext m_ctx;
        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb_copy;
        RenderTexture[] m_scratch_buffers;
        int[] m_callbacks;


        public override int beginFrame
        {
            get { return m_beginFrame; }
            set { m_beginFrame = value; }
        }

        public override int endFrame
        {
            get { return m_endFrame; }
            set { m_endFrame = value; }
        }

        void UpdateCallbacks()
        {
            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".png";

            {
                if (m_callbacks == null)
                {
                    m_callbacks = new int[m_scratch_buffers.Length];
                }
                for (int i = 0; i < m_callbacks.Length; ++i)
                {
                    string path = dir + "/" + m_outputFilename + "[" + i + "]_" + ext;
                    m_callbacks[i] = fcAPI.fcPngExportTexture(m_ctx, path, m_scratch_buffers[i], m_callbacks[i]);
                }
            }
        }

        void EraseCallbacks()
        {
            for (int i = 0; i < m_callbacks.Length; ++i)
            {
                fcAPI.fcEraseDeferredCall(m_callbacks[i]);
            }
            m_callbacks = null;
        }

        void AddCommandBuffers()
        {
            GetComponent<Camera>().AddCommandBuffer(CameraEvent.AfterEverything, m_cb_copy);
        }

        void RemoveCommandBuffers()
        {
            GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb_copy);
        }


#if UNITY_EDITOR
        void Reset()
        {
            m_sh_copy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_sh_copy);

            // initialize render targets
            m_scratch_buffers = new RenderTexture[m_targets.Length];
            for (int i = 0; i < m_scratch_buffers.Length; ++i)
            {
                var rt = m_targets[i];
                m_scratch_buffers[i] = new RenderTexture(rt.width, rt.height, 0, rt.format);
            }

            // initialize callbacks
            UpdateCallbacks();

            // initialize command buffers
            {
                m_cb_copy = new CommandBuffer();
                m_cb_copy.name = "PngOffscreenRecorder: Copy";
                for (int i = 0; i < m_targets.Length; ++i)
                {
                    m_cb_copy.SetRenderTarget(m_scratch_buffers[i]);
                    m_cb_copy.SetGlobalTexture("_TmpRenderTarget", m_targets[i]);
                    m_cb_copy.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 3);
                }
                for (int i = 0; i < m_targets.Length; ++i)
                {
                    m_cb_copy.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks[i]);
                }
            }

            fcAPI.fcPngConfig conf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);
        }

        void OnDisable()
        {
            RemoveCommandBuffers();
            EraseCallbacks();

            if (m_cb_copy != null)
            {
                m_cb_copy.Release();
                m_cb_copy = null;
            }

            for (int i = 0; i < m_scratch_buffers.Length; ++i)
            {
                m_scratch_buffers[i].Release();
            }
            m_scratch_buffers = null;

            fcAPI.fcPngDestroyContext(m_ctx);
            m_ctx.ptr = System.IntPtr.Zero;
        }

        void Update()
        {
            int frame = Time.frameCount;

            if (frame == m_beginFrame)
            {
                AddCommandBuffers();
            }
            if (frame == m_endFrame + 1)
            {
                RemoveCommandBuffers();
            }

            if (frame >= m_beginFrame && frame <= m_endFrame)
            {
                UpdateCallbacks();

                if (m_fillAlpha)
                {
                    m_mat_copy.EnableKeyword("FILL_ALPHA");
                }
                else
                {
                    m_mat_copy.DisableKeyword("FILL_ALPHA");
                }
            }
        }

    }
}

