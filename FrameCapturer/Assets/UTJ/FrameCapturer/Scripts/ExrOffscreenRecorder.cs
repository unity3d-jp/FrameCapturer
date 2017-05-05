using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/ExrOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class ExrOffscreenRecorder : ImageSequenceRecorder
    {
        public RenderTexture[] m_targets;

        public string m_outputFilename = "RenderTarget";

        fcAPI.fcEXRContext m_ctx;
        int[] m_callbacks;


        void EraseCallbacks()
        {
            if(m_callbacks != null)
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

        void DoExport()
        {
            Debug.Log("ExrOffscreenRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".exr";

            if (m_callbacks == null)
            {
                m_callbacks = new int[m_rtScratch.Length * 6];
            }
            for (int i = 0; i < m_rtScratch.Length; ++i)
            {
                int i6 = i * 6;
                string path = dir + "/" + m_outputFilename + "[" + i + "]_" + ext;
                var rt = m_rtScratch[i];

                m_callbacks[i6 + 0] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacks[i6 + 0]);
                m_callbacks[i6 + 1] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks[i6 + 1]);
                m_callbacks[i6 + 2] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks[i6 + 2]);
                m_callbacks[i6 + 3] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks[i6 + 3]);
                m_callbacks[i6 + 4] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 3, "A", m_callbacks[i6 + 4]);
                m_callbacks[i6 + 5] = fcAPI.fcExrEndImage(m_ctx, m_callbacks[i6 + 5]);
            }
            for (int i = 0; i < m_callbacks.Length; ++i)
            {
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks[i]);
            }
        }


        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_matCopy = new Material(m_shCopy);

            // initialize exr context
            fcAPI.fcExrConfig conf = fcAPI.fcExrConfig.default_value;
            m_ctx = fcAPI.fcExrCreateContext(ref conf);

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
                fcAPI.fcExrDestroyContext(m_ctx);
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

