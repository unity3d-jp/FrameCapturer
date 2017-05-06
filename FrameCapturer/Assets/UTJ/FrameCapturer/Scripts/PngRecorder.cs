using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


namespace UTJ.FrameCapturer
{
    [RequireComponent(typeof(Camera))]
    public class PngRecorder : ImageSequenceRecorder
    {
        public bool m_captureFramebuffer = true;
        public bool m_captureGBuffer = true;

        fcAPI.fcPNGContext m_ctx;
        fcAPI.fcDeferredCall[] m_callbacksFB;
        fcAPI.fcDeferredCall[] m_callbacksGB;


        void EraseCallbacks()
        {
            if (m_callbacksFB != null)
            {
                for (int i = 0; i < m_callbacksFB.Length; ++i)
                {
                    m_callbacksFB[i].Release();
                }
                m_callbacksFB = null;
            }

            if (m_callbacksGB != null)
            {
                for (int i = 0; i < m_callbacksGB.Length; ++i)
                {
                    m_callbacksGB[i].Release();
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
            Debug.Log("PngRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetFullPath();
            string ext = Time.frameCount.ToString("0000") + ".png";

            // callback for frame buffer
            {
                string path = dir + "/FrameBuffer_" + ext;
                if(m_callbacksFB == null)
                {
                    m_callbacksFB = new fcAPI.fcDeferredCall[1];
                }
                m_callbacksFB[0] = fcAPI.fcPngExportTexture(m_ctx, path, m_rtFB, m_callbacksFB[0]);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacksFB[0]);
            }

            // callbacks for gbuffer
            {
                string[] path = new string[] {
                    dir + "/Albedo_" + ext,
                    dir + "/Occlusion_" + ext,
                    dir + "/Specular_" + ext,
                    dir + "/Smoothness_" + ext,
                    dir + "/Normal_" + ext,
                    dir + "/Emission_" + ext,
                    dir + "/Depth_" + ext,
                };
                if (m_callbacksGB == null)
                {
                    m_callbacksGB = new fcAPI.fcDeferredCall[m_rtGB.Length];
                }
                for (int i = 0; i < m_callbacksGB.Length; ++i)
                {
                    m_callbacksGB[i] = fcAPI.fcPngExportTexture(m_ctx, path[i], m_rtGB[i], m_callbacksGB[i]);
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
                Debug.LogWarning("PngRecorder: Rendering Path must be deferred to use Capture GBuffer mode.");
                m_captureGBuffer = false;
            }
#endif // UNITY_EDITOR

            // initialize png context
            fcAPI.fcPngConfig conf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);

            // initialize render targets
            {
                m_rtFB = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_rtFB.wrapMode = TextureWrapMode.Repeat;
                m_rtFB.Create();

                var formats = new RenderTextureFormat[7] {
                    RenderTextureFormat.ARGBHalf,   // albedo (RGB)
                    RenderTextureFormat.RHalf,      // occlusion (R)
                    RenderTextureFormat.ARGBHalf,   // specular (RGB)
                    RenderTextureFormat.RHalf,      // smoothness (R)
                    RenderTextureFormat.ARGBHalf,   // normal (RGB)
                    RenderTextureFormat.ARGBHalf,   // emission (RGB)
                    RenderTextureFormat.RHalf,      // depth (R)
                };
                m_rtGB = new RenderTexture[7];
                for (int i = 0; i < m_rtGB.Length; ++i)
                {
                    // last one is depth (1 channel)
                    m_rtGB[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, formats[i]);
                    m_rtGB[i].filterMode = FilterMode.Point;
                    m_rtGB[i].Create();
                }
            }

            // initialize command buffers
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");

                m_cbCopyFB = new CommandBuffer();
                m_cbCopyFB.name = "PngRecorder: Copy FrameBuffer";
                m_cbCopyFB.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cbCopyFB.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cbCopyFB.SetRenderTarget(m_rtFB);
                m_cbCopyFB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                m_cbCopyFB.ReleaseTemporaryRT(tid);

                m_cbCopyGB = new CommandBuffer();
                m_cbCopyGB.name = "PngRecorder: Copy G-Buffer";
                m_cbCopyGB.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_rtGB[0], m_rtGB[1], m_rtGB[2], m_rtGB[3] }, m_rtGB[0]);
                m_cbCopyGB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 4);
                m_cbCopyGB.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_rtGB[4], m_rtGB[5], m_rtGB[6], m_rtGB[3] }, m_rtGB[0]);
                m_cbCopyGB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 5);
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
                m_ctx.Release();
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
