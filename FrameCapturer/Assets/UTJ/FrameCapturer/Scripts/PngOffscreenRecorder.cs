using System.Collections;
using UnityEngine;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/PngOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class PngOffscreenRecorder : IImageSequenceRecorder
    {
        public RenderTexture[] m_targets;

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "PngOutput");
        public string m_outputFilename = "RenderTarget";
        public int m_beginFrame = 0;
        public int m_endFrame = 100;
        public Shader m_sh_copy;

        fcAPI.fcPNGContext m_ctx;
        int m_frame;
        Material m_mat_copy;
        Mesh m_quad;
        RenderTexture[] m_scratch_buffers;


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

            m_scratch_buffers = new RenderTexture[m_targets.Length];
            for (int i = 0; i < m_scratch_buffers.Length; ++i)
            {
                var rt = m_targets[i];
                m_scratch_buffers[i] = new RenderTexture(rt.width, rt.height, 0, rt.format);
            }

            fcAPI.fcPngConfig conf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref conf);
        }

        void OnDisable()
        {
            fcAPI.fcPngDestroyContext(m_ctx);
            m_ctx.ptr = System.IntPtr.Zero;
        }


        IEnumerator OnPostRender()
        {
            int frame = m_frame++;
            if (frame >= m_beginFrame && frame <= m_endFrame)
            {
                yield return new WaitForEndOfFrame();

                Debug.Log("PngOffscreenRecorder: frame " + frame);

                for (int ti = 0; ti < m_targets.Length; ++ti)
                {
                    var target = m_targets[ti];
                    var scratch = m_scratch_buffers[ti];
                    var fmt = fcAPI.fcGetPixelFormat(target.format);
                    if (fmt == fcAPI.fcPixelFormat.Unknown) {
                        continue;
                    }

                    m_mat_copy.SetTexture("_TmpRenderTarget", target);
                    m_mat_copy.SetPass(3);
                    Graphics.SetRenderTarget(scratch);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);

                    string path = m_outputDir.GetPath() + "/" + m_outputFilename + "[" + ti + "]_" + frame.ToString("0000") + ".png";
                    fcAPI.fcPngExportTexture(m_ctx, path, scratch);
                }
            }
        }
    }
}

