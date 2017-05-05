using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/ExrRecorder")]
    [RequireComponent(typeof(Camera))]
    public class ExrRecorder : ImageSequenceRecorder
    {
        public bool m_captureFramebuffer = true;
        public bool m_captureGBuffer = true;

        public int m_beginFrame = 1;
        public int m_endFrame = 100;
        public Shader m_shCopy;

        fcAPI.fcEXRContext m_ctx;
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

            if(m_callbacks_gb != null)
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
            Debug.Log("ExrRecorder: exporting frame " + Time.frameCount);

            string dir = m_outputDir.GetPath();
            string ext = Time.frameCount.ToString("0000") + ".exr";

            if (m_captureFramebuffer)
            {
                // callback for frame buffer
                if (m_callbacks_fb == null)
                {
                    m_callbacks_fb = new int[5];
                }
                {
                    string path = dir + "/FrameBuffer_" + ext;
                    var rt = m_frame_buffer;
                    m_callbacks_fb[0] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_fb[0]);
                    m_callbacks_fb[1] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks_fb[1]);
                    m_callbacks_fb[2] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks_fb[2]);
                    m_callbacks_fb[3] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks_fb[3]);
                    m_callbacks_fb[4] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_fb[4]);
                }
                for (int i = 0; i < m_callbacks_fb.Length; ++i)
                {
                    GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callbacks_fb[i]);
                }
            }

            if (m_captureGBuffer)
            {
                // callbacks for gbuffer
                if (m_callbacks_gb == null)
                {
                    m_callbacks_gb = new int[29];
                }
                {
                    string path = dir + "/Albedo_" + ext;
                    var rt = m_gbuffer[0];
                    m_callbacks_gb[0] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[0]);
                    m_callbacks_gb[1] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks_gb[1]);
                    m_callbacks_gb[2] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks_gb[2]);
                    m_callbacks_gb[3] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks_gb[3]);
                    m_callbacks_gb[4] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[4]);
                }
                {
                    string path = dir + "/Occlusion_" + ext;
                    var rt = m_gbuffer[0];
                    m_callbacks_gb[5] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[5]);
                    m_callbacks_gb[6] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 3, "R", m_callbacks_gb[6]);
                    m_callbacks_gb[7] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[7]);
                }
                {
                    string path = dir + "/Specular_" + ext;
                    var rt = m_gbuffer[1];
                    m_callbacks_gb[8] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[8]);
                    m_callbacks_gb[9] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks_gb[9]);
                    m_callbacks_gb[10] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks_gb[10]);
                    m_callbacks_gb[11] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks_gb[11]);
                    m_callbacks_gb[12] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[12]);
                }
                {
                    string path = dir + "/Smoothness_" + ext;
                    var rt = m_gbuffer[1];
                    m_callbacks_gb[13] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[13]);
                    m_callbacks_gb[14] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 3, "R", m_callbacks_gb[14]);
                    m_callbacks_gb[15] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[15]);
                }
                {
                    string path = dir + "/Normal_" + ext;
                    var rt = m_gbuffer[2];
                    m_callbacks_gb[16] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[16]);
                    m_callbacks_gb[17] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks_gb[17]);
                    m_callbacks_gb[18] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks_gb[18]);
                    m_callbacks_gb[19] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks_gb[19]);
                    m_callbacks_gb[20] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[20]);
                }
                {
                    string path = dir + "/Emission_" + ext;
                    var rt = m_gbuffer[3];
                    m_callbacks_gb[21] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[21]);
                    m_callbacks_gb[22] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks_gb[22]);
                    m_callbacks_gb[23] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 1, "G", m_callbacks_gb[23]);
                    m_callbacks_gb[24] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 2, "B", m_callbacks_gb[24]);
                    m_callbacks_gb[25] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[25]);
                }
                {
                    string path = dir + "/Depth_" + ext;
                    var rt = m_gbuffer[4];
                    m_callbacks_gb[26] = fcAPI.fcExrBeginFrame(m_ctx, path, rt.width, rt.height, m_callbacks_gb[26]);
                    m_callbacks_gb[27] = fcAPI.fcExrAddLayerTexture(m_ctx, rt, 0, "R", m_callbacks_gb[27]);
                    m_callbacks_gb[28] = fcAPI.fcExrEndFrame(m_ctx, m_callbacks_gb[28]);
                }
                for (int i = 0; i < m_callbacks_gb.Length; ++i)
                {
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
                Debug.LogWarning("ExrRecorder: Rendering Path must be deferred to use Capture GBuffer mode.");
                m_captureGBuffer = false;
            }
#endif // UNITY_EDITOR

            // initialize exr context
            fcAPI.fcExrConfig conf = fcAPI.fcExrConfig.default_value;
            m_ctx = fcAPI.fcExrCreateContext(ref conf);

            // initialize render targets
            {
                m_frame_buffer = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_frame_buffer.wrapMode = TextureWrapMode.Repeat;
                m_frame_buffer.Create();

                m_gbuffer = new RenderTexture[5];
                for (int i = 0; i < m_gbuffer.Length; ++i)
                {
                    // last one is depth (1 channel)
                    m_gbuffer[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0,
                        i == 4 ? RenderTextureFormat.RHalf : RenderTextureFormat.ARGBHalf);
                    m_gbuffer[i].filterMode = FilterMode.Point;
                    m_gbuffer[i].Create();
                }
            }

            // initialize command buffers
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");

                m_cb_copy_fb = new CommandBuffer();
                m_cb_copy_fb.name = "ExrRecorder: Copy FrameBuffer";
                m_cb_copy_fb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cb_copy_fb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cb_copy_fb.SetRenderTarget(m_frame_buffer);
                m_cb_copy_fb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                m_cb_copy_fb.ReleaseTemporaryRT(tid);

                m_cb_copy_gb = new CommandBuffer();
                m_cb_copy_gb.name = "ExrRecorder: Copy G-Buffer";
                m_cb_copy_gb.SetRenderTarget(
                    new RenderTargetIdentifier[] { m_gbuffer[0], m_gbuffer[1], m_gbuffer[2], m_gbuffer[3] }, m_gbuffer[0]);
                m_cb_copy_gb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 1);
                m_cb_copy_gb.SetRenderTarget(m_gbuffer[4]); // depth
                m_cb_copy_gb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 2);
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
