using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/PngRecorder")]
    [RequireComponent(typeof(Camera))]
    public class PngRecorder : ImageSequenceRecorder
    {
        public bool m_captureFramebuffer = true;
        public bool m_captureGBuffer = true;

        public int m_beginFrame = 1;
        public int m_endFrame = 100;
        public Shader m_shCopy;

        fcAPI.fcPNGContext m_ctx;
        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cb_copy_fb;
        CommandBuffer m_cb_copy_gb;
        RenderTexture m_frame_buffer;
        RenderTexture[] m_gbuffer;
        int[] m_callbacks_fb;
        int[] m_callbacks_gb;


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
            if (m_callbacks_fb != null)
            {
                for (int i = 0; i < m_callbacks_fb.Length; ++i)
                {
                    fcAPI.fcEraseDeferredCall(m_callbacks_fb[i]);
                }
                m_callbacks_fb = null;
            }

            if (m_callbacks_gb != null)
            {
                for (int i = 0; i < m_callbacks_gb.Length; ++i)
                {
                    fcAPI.fcEraseDeferredCall(m_callbacks_gb[i]);
                }
                m_callbacks_gb = null;
            }
        }

        void AddCommandBuffers()
        {
            var cam = GetComponent<Camera>();
            if (m_captureFramebuffer)
            {
                cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb_copy_fb);
            }
            if (m_captureGBuffer)
            {
                cam.AddCommandBuffer(CameraEvent.BeforeLighting, m_cb_copy_gb);
            }
        }

        void RemoveCommandBuffers()
        {
            var cam = GetComponent<Camera>();
            if (m_captureFramebuffer)
            {
                cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb_copy_fb);
            }
            if (m_captureGBuffer)
            {
                cam.RemoveCommandBuffer(CameraEvent.BeforeLighting, m_cb_copy_gb);
            }
        }

        void DoExport()
        {
            Debug.Log("PngRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".png";

            // callback for frame buffer
            {
                string path = dir + "/FrameBuffer_" + ext;
                if(m_callbacks_fb == null)
                {
                    m_callbacks_fb = new int[1];
                }
                m_callbacks_fb[0] = fcAPI.fcPngExportTexture(m_ctx, path, m_frame_buffer, m_callbacks_fb[0]);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks_fb[0]);
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
                if (m_callbacks_gb == null)
                {
                    m_callbacks_gb = new int[m_gbuffer.Length];
                }
                for (int i = 0; i < m_callbacks_gb.Length; ++i)
                {
                    m_callbacks_gb[i] = fcAPI.fcPngExportTexture(m_ctx, path[i], m_gbuffer[i], m_callbacks_gb[i]);
                    GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks_gb[i]);
                }
            }
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

            var cam = GetComponent<Camera>();
            if (cam.targetTexture != null)
            {
                m_matCopy.EnableKeyword("OFFSCREEN");
            }

#if UNITY_EDITOR
            if (m_captureGBuffer && !FrameCapturerUtils.IsRenderingPathDeferred(cam))
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
                m_frame_buffer = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_frame_buffer.wrapMode = TextureWrapMode.Repeat;
                m_frame_buffer.Create();

                var formats = new RenderTextureFormat[7] {
                    RenderTextureFormat.ARGBHalf,   // albedo (RGB)
                    RenderTextureFormat.RHalf,      // occlusion (R)
                    RenderTextureFormat.ARGBHalf,   // specular (RGB)
                    RenderTextureFormat.RHalf,      // smoothness (R)
                    RenderTextureFormat.ARGBHalf,   // normal (RGB)
                    RenderTextureFormat.ARGBHalf,   // emission (RGB)
                    RenderTextureFormat.RHalf,      // depth (R)
                };
                m_gbuffer = new RenderTexture[7];
                for (int i = 0; i < m_gbuffer.Length; ++i)
                {
                    // last one is depth (1 channel)
                    m_gbuffer[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, formats[i]);
                    m_gbuffer[i].filterMode = FilterMode.Point;
                    m_gbuffer[i].Create();
                }
            }

            // initialize command buffers
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");

                m_cb_copy_fb = new CommandBuffer();
                m_cb_copy_fb.name = "PngRecorder: Copy FrameBuffer";
                m_cb_copy_fb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cb_copy_fb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cb_copy_fb.SetRenderTarget(m_frame_buffer);
                m_cb_copy_fb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                m_cb_copy_fb.ReleaseTemporaryRT(tid);

                m_cb_copy_gb = new CommandBuffer();
                m_cb_copy_gb.name = "PngRecorder: Copy G-Buffer";
                m_cb_copy_gb.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_gbuffer[0], m_gbuffer[1], m_gbuffer[2], m_gbuffer[3] }, m_gbuffer[0]);
                m_cb_copy_gb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 4);
                m_cb_copy_gb.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_gbuffer[4], m_gbuffer[5], m_gbuffer[6], m_gbuffer[3] }, m_gbuffer[0]);
                m_cb_copy_gb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 5);
            }
        }

        void OnDisable()
        {
            RemoveCommandBuffers();

            if (m_cb_copy_gb != null)
            {
                m_cb_copy_gb.Release();
                m_cb_copy_gb = null;
            }

            if (m_cb_copy_fb != null)
            {
                m_cb_copy_fb.Release();
                m_cb_copy_fb = null;
            }

            if (m_frame_buffer != null)
            {
                m_frame_buffer.Release();
                m_frame_buffer = null;
            }
            if (m_gbuffer != null)
            {
                for (int i = 0; i < m_gbuffer.Length; ++i)
                {
                    m_gbuffer[i].Release();
                }
                m_gbuffer = null;
            }

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
