using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [RequireComponent(typeof(Camera))]
    public class PngOffscreenRecorder : ImageSequenceRecorder
    {
        public RenderTexture[] m_targets;

        public string m_outputFilename = "RenderTarget";

        fcAPI.fcPNGContext m_ctx;
        int[] m_callbacks;


        void DoExport()
        {
            Debug.Log("PngOffscreenRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetFullPath();
            string ext = Time.frameCount.ToString("0000") + ".png";

            if (m_callbacks == null)
            {
                m_callbacks = new int[m_rtScratch.Length];
            }
            for (int i = 0; i < m_callbacks.Length; ++i)
            {
                string path = dir + "/" + m_outputFilename + "[" + i + "]_" + ext;
                m_callbacks[i] = fcAPI.fcPngExportTexture(m_ctx, path, m_rtScratch[i], m_callbacks[i]);
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
            GetComponent<Camera>().AddCommandBuffer(CameraEvent.AfterEverything, m_cbCopyRT);
        }

        void RemoveCommandBuffers()
        {
            GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cbCopyRT);
        }


        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = fcAPI.CreateFullscreenQuad();
            m_matCopy = new Material(m_shCopy);

            // initialize png context
            fcAPI.fcPngConfig conf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);

            // initialize render targets
            m_rtScratch = new RenderTexture[m_targets.Length];
            for (int i = 0; i < m_rtScratch.Length; ++i)
            {
                var rt = m_targets[i];
                m_rtScratch[i] = new RenderTexture(rt.width, rt.height, 0, rt.format);
                m_rtScratch[i].Create();
            }

            // initialize command buffers
            {
                m_cbCopyRT = new CommandBuffer();
                m_cbCopyRT.name = "PngOffscreenRecorder: Copy";
                for (int i = 0; i < m_targets.Length; ++i)
                {
                    m_cbCopyRT.SetRenderTarget(m_rtScratch[i]);
                    m_cbCopyRT.SetGlobalTexture("_TmpRenderTarget", m_targets[i]);
                    m_cbCopyRT.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 3);
                }
            }
        }

        void OnDisable()
        {
            RemoveCommandBuffers();

            if (m_cbCopyRT != null)
            {
                m_cbCopyRT.Release();
                m_cbCopyRT = null;
            }

            for (int i = 0; i < m_rtScratch.Length; ++i)
            {
                m_rtScratch[i].Release();
            }
            m_rtScratch = null;

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

            if (frame == m_startFrame)
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
            if (frame >= m_startFrame && frame <= m_endFrame)
            {
                yield return new WaitForEndOfFrame();
                DoExport();
            }
        }
    }
}

