using System.Collections;
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
        public int m_beginFrame = 0;
        public int m_endFrame = 100;
        public bool m_fillAlpha = false;
        public Shader m_sh_copy;

        fcAPI.fcPNGContext m_ctx;
        int m_frame;
        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_frame_buffer;
        RenderTexture[] m_gbuffer;
        RenderTexture m_depth;
        RenderBuffer[] m_rt_gbuffer;


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

            if (m_captureFramebuffer)
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");
                m_cb = new CommandBuffer();
                m_cb.name = "ExrCapturer: copy frame buffer";
                m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                // tid は意図的に開放しない
                cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);

                m_frame_buffer = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_frame_buffer.wrapMode = TextureWrapMode.Repeat;
                m_frame_buffer.Create();
            }

#if UNITY_EDITOR
            if (m_captureGBuffer &&
                cam.renderingPath != RenderingPath.DeferredShading &&
                (cam.renderingPath == RenderingPath.UsePlayerSettings && PlayerSettings.renderingPath != RenderingPath.DeferredShading))
            {
                Debug.Log("ExrCapturer: Rendering path must be deferred to use capture_gbuffer mode.");
                m_captureGBuffer = false;
            }
#endif // UNITY_EDITOR

            if (m_captureGBuffer)
            {
                m_gbuffer = new RenderTexture[4];
                m_rt_gbuffer = new RenderBuffer[4];
                for (int i = 0; i < m_gbuffer.Length; ++i)
                {
                    m_gbuffer[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                    m_gbuffer[i].filterMode = FilterMode.Point;
                    m_gbuffer[i].Create();
                    m_rt_gbuffer[i] = m_gbuffer[i].colorBuffer;
                }
                {
                    m_depth = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.RHalf);
                    m_depth.filterMode = FilterMode.Point;
                    m_depth.Create();
                }
            }

            fcAPI.fcPngConfig conf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);
        }

        void OnDisable()
        {
            if (m_cb != null)
            {
                GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
                m_cb.Release();
                m_cb = null;
            }
            if (m_frame_buffer != null)
            {
                m_frame_buffer.Release();
                m_frame_buffer = null;
            }
            if (m_depth != null)
            {
                for (int i = 0; i < m_gbuffer.Length; ++i)
                {
                    m_gbuffer[i].Release();
                }
                m_depth.Release();
                m_gbuffer = null;
                m_depth = null;
                m_rt_gbuffer = null;
            }

            fcAPI.fcPngDestroyContext(m_ctx);
            m_ctx.ptr = System.IntPtr.Zero;
        }

        IEnumerator OnPostRender()
        {
            int frame = m_frame++;
            if (frame >= m_beginFrame && frame <= m_endFrame)
            {
                Debug.Log("PngCapturer: frame " + frame);

                if(m_fillAlpha)
                {
                    m_mat_copy.EnableKeyword("FILL_ALPHA");
                }
                else
                {
                    m_mat_copy.DisableKeyword("FILL_ALPHA");
                }

                if (m_captureGBuffer)
                {
                    m_mat_copy.SetPass(1);
                    Graphics.SetRenderTarget(m_rt_gbuffer, m_gbuffer[0].depthBuffer);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);

                    m_mat_copy.SetPass(2);
                    Graphics.SetRenderTarget(m_depth);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);

                    string dir = m_outputDir.GetPath();
                    string ext = frame.ToString("0000") + ".png";
                    string path_gb0 = dir + "/AlbedoOcclusion_" + ext;
                    string path_gb1 = dir + "/SpecularSmoothness_" + ext;
                    string path_gb2 = dir + "/Normal_" + ext;
                    string path_gb3 = dir + "/Emission_" + ext;
                    fcAPI.fcPngExportTexture(m_ctx, path_gb0, m_gbuffer[0], false);
                    fcAPI.fcPngExportTexture(m_ctx, path_gb1, m_gbuffer[1], false);
                    fcAPI.fcPngExportTexture(m_ctx, path_gb2, m_gbuffer[2], false);
                    fcAPI.fcPngExportTexture(m_ctx, path_gb3, m_gbuffer[3], false);
                }

                yield return new WaitForEndOfFrame();
                if (m_captureFramebuffer)
                {
                    m_mat_copy.SetPass(0);
                    Graphics.SetRenderTarget(m_frame_buffer);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);

                    string path = m_outputDir.GetPath() + "/FrameBuffer_" + frame.ToString("0000") + ".png";
                    fcAPI.fcPngExportTexture(m_ctx, path, m_frame_buffer);
                }
            }
        }
    }
}
