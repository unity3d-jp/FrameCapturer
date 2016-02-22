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
    public class PngRecorder : MonoBehaviour
    {
        public enum DepthFormat
        {
            Half,
            Float,
        }


        public bool m_captureFramebuffer = true;
        public bool m_captureGBuffer = true;
        public DepthFormat m_depthFormat = DepthFormat.Float;

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "ExrOutput");
        public string m_filenameFramebuffer = "FrameBuffer";
        public string m_filenameGBuffer = "GBuffer";
        public int m_beginFrame = 0;
        public int m_endFrame = 100;
        public int m_maxTasks = 1;
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
        Camera m_cam;


#if UNITY_EDITOR
        void Reset()
        {
            m_sh_copy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_cam = GetComponent<Camera>();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_sh_copy);
            if (m_cam.targetTexture != null)
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
                m_cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);

                m_frame_buffer = new RenderTexture(m_cam.pixelWidth, m_cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_frame_buffer.wrapMode = TextureWrapMode.Repeat;
                m_frame_buffer.Create();
            }

#if UNITY_EDITOR
            if (m_captureGBuffer &&
                m_cam.renderingPath != RenderingPath.DeferredShading &&
                (m_cam.renderingPath == RenderingPath.UsePlayerSettings && PlayerSettings.renderingPath != RenderingPath.DeferredShading))
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
                    m_gbuffer[i] = new RenderTexture(m_cam.pixelWidth, m_cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                    m_gbuffer[i].filterMode = FilterMode.Point;
                    m_gbuffer[i].Create();
                    m_rt_gbuffer[i] = m_gbuffer[i].colorBuffer;
                }
                {
                    RenderTextureFormat format = m_depthFormat == DepthFormat.Half ? RenderTextureFormat.RHalf : RenderTextureFormat.RFloat;
                    m_depth = new RenderTexture(m_cam.pixelWidth, m_cam.pixelHeight, 0, format);
                    m_depth.filterMode = FilterMode.Point;
                    m_depth.Create();
                }
            }

            fcAPI.fcPngConfig conf;
            conf.max_active_tasks = m_maxTasks;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);
        }

        void OnDisable()
        {
            if (m_cb != null)
            {
                m_cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
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
        }

        IEnumerator OnPostRender()
        {
            int frame = m_frame++;
            if (frame >= m_beginFrame && frame <= m_endFrame)
            {
                Debug.Log("PngCapturer: frame " + frame);

                if (m_captureGBuffer)
                {
                    m_mat_copy.SetPass(1);
                    Graphics.SetRenderTarget(m_rt_gbuffer, m_gbuffer[0].depthBuffer);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);

                    m_mat_copy.SetPass(2);
                    Graphics.SetRenderTarget(m_depth);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);

                    string path_gb0 = m_outputDir.GetPath() + "/" + m_filenameGBuffer + "_AlbedoOcclusion" + frame.ToString("0000") + ".png";
                    string path_gb1 = m_outputDir.GetPath() + "/" + m_filenameGBuffer + "_SpecularSmoothness" + frame.ToString("0000") + ".png";
                    string path_gb2 = m_outputDir.GetPath() + "/" + m_filenameGBuffer + "_Normal" + frame.ToString("0000") + ".png";
                    string path_gb3 = m_outputDir.GetPath() + "/" + m_filenameGBuffer + "_Emission" + frame.ToString("0000") + ".png";
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

                    string path = m_outputDir.GetPath() + "/" + m_filenameFramebuffer + "_" + frame.ToString("0000") + ".png";
                    fcAPI.fcPngExportTexture(m_ctx, path, m_frame_buffer, false);
                }
            }
        }
    }
}
