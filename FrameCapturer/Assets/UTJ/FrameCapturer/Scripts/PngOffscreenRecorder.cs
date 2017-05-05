using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/PngOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class PngOffscreenRecorder : ImageSequenceRecorderBase
    {
        public RenderTexture[] m_targets;

        public string m_outputFilename = "RenderTarget";
        public int m_beginFrame = 1;
        public int m_endFrame = 100;
        public Shader m_shCopy;

        fcAPI.fcPNGContext m_ctx;
        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cb_copy;
        RenderTexture[] m_scratchBuffers;
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

        void DoExport()
        {
            Debug.Log("PngOffscreenRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".png";

            if (m_callbacks == null)
            {
                m_callbacks = new int[m_scratchBuffers.Length];
            }
            for (int i = 0; i < m_callbacks.Length; ++i)
            {
                string path = dir + "/" + m_outputFilename + "[" + i + "]_" + ext;
                m_callbacks[i] = fcAPI.fcPngExportTexture(m_ctx, path, m_scratchBuffers[i], m_callbacks[i]);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks[i]);
            }
        }

        void EraseCallbacks()
        {
            if (m_callbacks != null)
            {
                for (int i = 0; i < m_callbacks.Length; ++i)
                {
                    fcAPI.fcEraseDeferredCall(m_callbacks[i]);
                }
                m_callbacks = null;
            }
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
            m_shCopy = FrameCapturerUtils.GetFrameBufferCopyShader();
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
            m_matCopy = new Material(m_shCopy);

            // initialize png context
            fcAPI.fcPngConfig conf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);

            // initialize render targets
            m_scratchBuffers = new RenderTexture[m_targets.Length];
            for (int i = 0; i < m_scratchBuffers.Length; ++i)
            {
                var rt = m_targets[i];
                m_scratchBuffers[i] = new RenderTexture(rt.width, rt.height, 0, rt.format);
                m_scratchBuffers[i].Create();
            }

            // initialize command buffers
            {
                m_cb_copy = new CommandBuffer();
                m_cb_copy.name = "PngOffscreenRecorder: Copy";
                for (int i = 0; i < m_targets.Length; ++i)
                {
                    m_cb_copy.SetRenderTarget(m_scratchBuffers[i]);
                    m_cb_copy.SetGlobalTexture("_TmpRenderTarget", m_targets[i]);
                    m_cb_copy.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 3);
                }
            }
        }

        void OnDisable()
        {
            RemoveCommandBuffers();

            if (m_cb_copy != null)
            {
                m_cb_copy.Release();
                m_cb_copy = null;
            }

            for (int i = 0; i < m_scratchBuffers.Length; ++i)
            {
                m_scratchBuffers[i].Release();
            }
            m_scratchBuffers = null;

            fcAPI.fcGuard(() =>
            {
                EraseCallbacks();
                fcAPI.fcPngDestroyContext(m_ctx);
                m_ctx.ptr = System.IntPtr.Zero;
            });
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

