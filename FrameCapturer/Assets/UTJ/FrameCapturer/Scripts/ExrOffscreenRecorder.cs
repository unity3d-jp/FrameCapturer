using System.Collections;
using UnityEngine;


namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/ExrOffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class ExrOffscreenRecorder : IImageSequenceRecorder
    {
        public RenderTexture[] m_targets;

        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.CurrentDirectory, "ExrOutput");
        public string m_outputFilename = "RenderTarget";
        public int m_beginFrame = 0;
        public int m_endFrame = 100;
        public Shader m_sh_copy;

        fcAPI.fcEXRContext m_ctx;
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

            fcAPI.fcExrConfig conf = fcAPI.fcExrConfig.default_value;
            m_ctx = fcAPI.fcExrCreateContext(ref conf);
        }

        void OnDisable()
        {
            fcAPI.fcExrDestroyContext(m_ctx);
            m_ctx.ptr = System.IntPtr.Zero;
        }


        IEnumerator OnPostRender()
        {
            int frame = m_frame++;
            if (frame >= m_beginFrame && frame <= m_endFrame)
            {
                yield return new WaitForEndOfFrame();

                Debug.Log("ExrOffscreenRecorder: frame " + frame);

                string[] channel_names = new string[] {"R", "G", "B", "A"};

                for (int ti = 0; ti < m_targets.Length; ++ti)
                {
                    var target = m_targets[ti];
                    var scratch = m_scratch_buffers[ti];
                    var fmt = fcAPI.fcGetPixelFormat(target.format);
                    if (fmt == fcAPI.fcPixelFormat.Unknown) { continue; }

                    m_mat_copy.SetTexture("_TmpRenderTarget", target);
                    m_mat_copy.SetPass(3);
                    Graphics.SetRenderTarget(scratch);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);

                    string path = m_outputDir.GetPath() + "/" + m_outputFilename + "[" + ti + "]_" + frame.ToString("0000") + ".exr";
                    int num_channels = (int)fmt & (int)fcAPI.fcPixelFormat.ChannelMask;
                    fcAPI.fcExrBeginFrame(m_ctx, path, target.width, target.height);
                    for (int i=0; i<num_channels; ++i)
                    {
                        fcAPI.fcExrAddLayerTexture(m_ctx, scratch, i, channel_names[i]);
                    }
                    fcAPI.fcExrEndFrame(m_ctx);
                }
            }
        }
    }
}

