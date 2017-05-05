using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


namespace UTJ.FrameCapturer
{
    [AddComponentMenu("UTJ/FrameCapturer/ExrRecorder")]
    [RequireComponent(typeof(Camera))]
    public class ExrRecorder : ImageSequenceRecorder
    {
        public bool m_captureFramebuffer = true;
        public bool m_captureGBuffer = true;

        fcAPI.fcEXRContext m_ctx;
        int[] m_callbacksFB;
        int[] m_callbacksGB;


        void EraseCallbacks()
        {
            if (m_callbacksFB != null)
            {
                for (int i = 0; i < m_callbacksFB.Length; ++i)
                {
                    fcAPI.fcEraseDeferredCall(m_callbacksFB[i]);
                }
                m_callbacksFB = null;
            }

            if(m_callbacksGB != null)
            {
                for (int i = 0; i < m_callbacksGB.Length; ++i)
                {
                    fcAPI.fcEraseDeferredCall(m_callbacksGB[i]);
                }
                m_callbacksGB = null;
            }
        }

        void AddCommandBuffers()
        {
            var cam = GetComponent<Camera>();
            if (m_captureFramebuffer)
            {
                cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);
            }
            if (m_captureGBuffer)
            {
                cam.AddCommandBuffer(CameraEvent.BeforeLighting, m_cbCopyGB);
            }
        }

        void RemoveCommandBuffers()
        {
            var cam = GetComponent<Camera>();
            if (m_captureFramebuffer)
            {
                cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);
            }
            if (m_captureGBuffer)
            {
                cam.RemoveCommandBuffer(CameraEvent.BeforeLighting, m_cbCopyGB);
            }
        }

        void DoExport()
        {
            Debug.Log("ExrRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetFullPath();
            string ext = Time.frameCount.ToString("0000") + ".exr";

            if (m_captureFramebuffer)
            {
                // callback for frame buffer
                if (m_callbacksFB == null)
                {
                    m_callbacksFB = new int[5];
                }
                {
                    string path = dir + "/FrameBuffer_" + ext;
                    var rt = m_rtFB;
                    m_callbacksFB[0] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksFB[0]);
                    m_callbacksFB[1] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacksFB[1]);
                    m_callbacksFB[2] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacksFB[2]);
                    m_callbacksFB[3] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacksFB[3]);
                    m_callbacksFB[4] = fcAPI.fcExrEndImage(m_ctx, m_callbacksFB[4]);
                }
                for (int i = 0; i < m_callbacksFB.Length; ++i)
                {
                    GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacksFB[i]);
                }
            }

            if (m_captureGBuffer)
            {
                // callbacks for gbuffer
                if (m_callbacksGB == null)
                {
                    m_callbacksGB = new int[29];
                }
                {
                    string path = dir + "/Albedo_" + ext;
                    var rt = m_rtGB[0];
                    m_callbacksGB[0] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[0]);
                    m_callbacksGB[1] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacksGB[1]);
                    m_callbacksGB[2] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacksGB[2]);
                    m_callbacksGB[3] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacksGB[3]);
                    m_callbacksGB[4] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[4]);
                }
                {
                    string path = dir + "/Occlusion_" + ext;
                    var rt = m_rtGB[0];
                    m_callbacksGB[5] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[5]);
                    m_callbacksGB[6] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 3, "R", m_callbacksGB[6]);
                    m_callbacksGB[7] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[7]);
                }
                {
                    string path = dir + "/Specular_" + ext;
                    var rt = m_rtGB[1];
                    m_callbacksGB[8] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[8]);
                    m_callbacksGB[9] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacksGB[9]);
                    m_callbacksGB[10] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacksGB[10]);
                    m_callbacksGB[11] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacksGB[11]);
                    m_callbacksGB[12] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[12]);
                }
                {
                    string path = dir + "/Smoothness_" + ext;
                    var rt = m_rtGB[1];
                    m_callbacksGB[13] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[13]);
                    m_callbacksGB[14] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 3, "R", m_callbacksGB[14]);
                    m_callbacksGB[15] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[15]);
                }
                {
                    string path = dir + "/Normal_" + ext;
                    var rt = m_rtGB[2];
                    m_callbacksGB[16] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[16]);
                    m_callbacksGB[17] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacksGB[17]);
                    m_callbacksGB[18] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacksGB[18]);
                    m_callbacksGB[19] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacksGB[19]);
                    m_callbacksGB[20] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[20]);
                }
                {
                    string path = dir + "/Emission_" + ext;
                    var rt = m_rtGB[3];
                    m_callbacksGB[21] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[21]);
                    m_callbacksGB[22] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacksGB[22]);
                    m_callbacksGB[23] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacksGB[23]);
                    m_callbacksGB[24] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacksGB[24]);
                    m_callbacksGB[25] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[25]);
                }
                {
                    string path = dir + "/Depth_" + ext;
                    var rt = m_rtGB[4];
                    m_callbacksGB[26] = fcAPI.fcExrBeginImage(m_ctx, path, rt.width, rt.height, m_callbacksGB[26]);
                    m_callbacksGB[27] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacksGB[27]);
                    m_callbacksGB[28] = fcAPI.fcExrEndImage(m_ctx, m_callbacksGB[28]);
                }
                for (int i = 0; i < m_callbacksGB.Length; ++i)
                {
                    GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacksGB[i]);
                }
            }
        }

        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = fcAPI.CreateFullscreenQuad();
            m_matCopy = new Material(m_shCopy);

            var cam = GetComponent<Camera>();
            if (cam.targetTexture != null)
            {
                m_matCopy.EnableKeyword("OFFSCREEN");
            }

#if UNITY_EDITOR
            if (m_captureGBuffer && !fcAPI.IsRenderingPathDeferred(cam))
            {
                Debug.LogWarning("ExrRecorder: Rendering Path must be deferred to use Capture GBuffer mode.");
                m_captureGBuffer = false;
            }
#endif // UNITY_EDITOR

            // initialize exr context
            fcAPI.fcExrConfig conf = fcAPI.fcExrConfig.default_value;
            m_ctx = fcAPI.fcExrCreateContext(ref conf);

            // initialize render targets
            {
                m_rtFB = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_rtFB.wrapMode = TextureWrapMode.Repeat;
                m_rtFB.Create();

                m_rtGB = new RenderTexture[5];
                for (int i = 0; i < m_rtGB.Length; ++i)
                {
                    // last one is depth (1 channel)
                    m_rtGB[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0,
                        i == 4 ? RenderTextureFormat.RHalf : RenderTextureFormat.ARGBHalf);
                    m_rtGB[i].filterMode = FilterMode.Point;
                    m_rtGB[i].Create();
                }
            }

            // initialize command buffers
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");

                m_cbCopyFB = new CommandBuffer();
                m_cbCopyFB.name = "ExrRecorder: Copy FrameBuffer";
                m_cbCopyFB.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cbCopyFB.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cbCopyFB.SetRenderTarget(m_rtFB);
                m_cbCopyFB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                m_cbCopyFB.ReleaseTemporaryRT(tid);

                m_cbCopyGB = new CommandBuffer();
                m_cbCopyGB.name = "ExrRecorder: Copy G-Buffer";
                m_cbCopyGB.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_rtGB[0], m_rtGB[1], m_rtGB[2], m_rtGB[3] }, m_rtGB[0]);
                m_cbCopyGB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 1);
                m_cbCopyGB.SetRenderTarget(m_rtGB[4]); // depth
                m_cbCopyGB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 2);
            }
        }

        void OnDisable()
        {
            RemoveCommandBuffers();

            if (m_cbCopyGB != null)
            {
                m_cbCopyGB.Release();
                m_cbCopyGB = null;
            }

            if (m_cbCopyFB != null)
            {
                m_cbCopyFB.Release();
                m_cbCopyFB = null;
            }

            if (m_rtFB != null)
            {
                m_rtFB.Release();
                m_rtFB = null;
            }
            if (m_rtGB != null)
            {
                for (int i = 0; i < m_rtGB.Length; ++i)
                {
                    m_rtGB[i].Release();
                }
                m_rtGB = null;
            }

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
