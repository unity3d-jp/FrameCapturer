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
    public class PngRecorder : IImageSequenceRecorder
    {
        public bool m_captureFramebuffer = true;
        public bool m_captureGBuffer = true;

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "PngOutput");
        public int m_beginFrame = 1;
        public int m_endFrame = 100;
        public bool m_fillAlpha = false;
        public Shader m_sh_copy;

        fcAPI.fcPNGContext m_ctx;
        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb_copy_fb;
        CommandBuffer m_cb_copy_gb;
        RenderTexture m_frame_buffer;
        RenderTexture[] m_gbuffer;
        int m_callback_fb;
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


        void UpdateCallbacks()
        {
            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".png";

            // callback for frame buffer
            {
                string path = dir + "/FrameBuffer_" + ext;
                m_callback_fb = fcAPI.fcPngExportTexture(m_ctx, path, m_frame_buffer, m_callback_fb);
            }

            // callbacks for gbuffer
            {
                string[] path = new string[] {
                    dir + "/AlbedoOcclusion_" + ext,
                    dir + "/SpecularSmoothness_" + ext,
                    dir + "/Normal_" + ext,
                    dir + "/Emission_" + ext,
                    dir + "/Depth_" + ext,
                };
                if(m_callbacks_gb == null)
                {
                    m_callbacks_gb = new int[m_gbuffer.Length];
                }
                for (int i = 0; i < m_callbacks_gb.Length; ++i)
                {
                    m_callbacks_gb[i] = fcAPI.fcPngExportTexture(m_ctx, path[i], m_gbuffer[i], m_callbacks_gb[i]);
                }
            }
        }

        void EraseCallbacks()
        {
            fcAPI.fcEraseDeferredCall(m_callback_fb);
            m_callback_fb = 0;

            for (int i = 0; i < m_callbacks_gb.Length; ++i)
            {
                fcAPI.fcEraseDeferredCall(m_callbacks_gb[i]);
            }
            m_callbacks_gb = null;
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
                cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb_copy_gb);
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
                cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb_copy_gb);
            }
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

            var cam = GetComponent<Camera>();
            if (cam.targetTexture != null)
            {
                m_mat_copy.EnableKeyword("OFFSCREEN");
            }

#if UNITY_EDITOR
            if (m_captureGBuffer &&
                FrameCapturerUtils.WarnIfRenderingPassIsNotDeferred(cam,
                "PngRecorder: Rendering Path must be deferred to use Capture GBuffer mode."))
            {
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

                m_gbuffer = new RenderTexture[5];
                for (int i = 0; i < m_gbuffer.Length; ++i)
                {
                    m_gbuffer[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0,
                        i == 4 ? RenderTextureFormat.RHalf : RenderTextureFormat.ARGBHalf);
                    m_gbuffer[i].filterMode = FilterMode.Point;
                    m_gbuffer[i].Create();
                }
            }

            // initialize callbacks
            UpdateCallbacks();

            // initialize command buffers
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");

                m_cb_copy_fb = new CommandBuffer();
                m_cb_copy_fb.name = "PngRecorder: Copy FrameBuffer";
                m_cb_copy_fb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cb_copy_fb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cb_copy_fb.SetRenderTarget(m_frame_buffer);
                m_cb_copy_fb.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 0);
                m_cb_copy_fb.ReleaseTemporaryRT(tid);
                m_cb_copy_fb.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callback_fb);

                m_cb_copy_gb = new CommandBuffer();
                m_cb_copy_gb.name = "PngRecorder: Copy G-Buffer";
                m_cb_copy_gb.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_gbuffer[0], m_gbuffer[1], m_gbuffer[2], m_gbuffer[3] }, m_gbuffer[0]);
                m_cb_copy_gb.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 1);
                m_cb_copy_gb.SetRenderTarget(m_gbuffer[4]); // depth
                m_cb_copy_gb.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 2);
                for (int i = 0; i < m_callbacks_gb.Length; ++i)
                {
                    m_cb_copy_gb.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks_gb[i]);
                }
            }
        }

        void OnDisable()
        {
            RemoveCommandBuffers();
            EraseCallbacks();

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
