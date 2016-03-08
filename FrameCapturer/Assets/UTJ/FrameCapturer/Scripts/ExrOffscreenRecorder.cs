using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/ExrOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class ExrOffscreenRecorder : IImageSequenceRecorder
    {
        public RenderTexture[] m_targets;

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "ExrOutput");
        public string m_outputFilename = "RenderTarget";
        public int m_beginFrame = 1;
        public int m_endFrame = 100;
        public Shader m_sh_copy;

        fcAPI.fcEXRContext m_ctx;
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

        void DoExport()
        {
            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".exr";

            if (m_callbacks == null)
            {
                m_callbacks = new int[m_scratch_buffers.Length * 6];
            }
            for (int i = 0; i < m_scratch_buffers.Length; ++i)
            {
                int i6 = i * 6;
                string path = dir + "/" + m_outputFilename + "[" + i + "]_" + ext;
                var rt = m_scratch_buffers[i];

                m_callbacks[i6 + 0] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks[i6 + 0]);
                m_callbacks[i6 + 1] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks[i6 + 1]);
                m_callbacks[i6 + 2] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks[i6 + 2]);
                m_callbacks[i6 + 3] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks[i6 + 3]);
                m_callbacks[i6 + 4] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 3, "A", m_callbacks[i6 + 4]);
                m_callbacks[i6 + 5] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks[i6 + 5]);
            }
            for (int i = 0; i < m_callbacks.Length; ++i)
            {
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks[i]);
            }
        }


#if UNITY_EDITOR
        void Reset()
        {
            m_sh_copy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }

        void OnValidate()
        {
            m_beginFrame = Mathf.Max(1, m_beginFrame);
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_sh_copy);

            // initialize exr context
            fcAPI.fcExrConfig conf = fcAPI.fcExrConfig.default_value;
            m_ctx = fcAPI.fcExrCreateContext(ref conf);

            // initialize render targets
            m_scratch_buffers = new RenderTexture[m_targets.Length];
            for (int i = 0; i < m_scratch_buffers.Length; ++i)
            {
                var rt = m_targets[i];
                m_scratch_buffers[i] = new RenderTexture(rt.width, rt.height, 0, rt.format);
                m_scratch_buffers[i].Create();
            }

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
            }
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

            fcAPI.fcExrDestroyContext(m_ctx);
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
        }

        IEnumerator OnPostRender()
        {
            int frame = Time.frameCount;
            if (frame >= m_beginFrame && frame <= m_endFrame)
            {
                yield return new WaitForEndOfFrame();
                DoExport();
            }
        }
    }
}

