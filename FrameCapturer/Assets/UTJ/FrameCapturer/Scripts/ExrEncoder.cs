using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public class ExrEncoder : ImageSequenceEncoder
    {
        static readonly string[] s_channelNames = { "R", "G", "B", "A" };
        [SerializeField] ImageSequenceRecorder m_recorder;
        fcAPI.fcExrContext m_ctx;


        public override Type type { get { return Type.Exr; } }


        public override void Initialize(ImageSequenceRecorder recorder)
        {
            m_recorder = recorder;
            var exrconf = m_recorder.exrConfig;
            m_ctx = fcAPI.fcExrCreateContext(ref exrconf);
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void Export(RenderTexture frame, int channels, string name)
        {
            if (!m_recorder || !m_ctx) { return; }
            string path = m_recorder.outputDir.GetFullPath() + "/" + name + "_" + m_recorder.frame.ToString("0000") + ".exr";

            fcAPI.fcLock(frame, (data, fmt) => {
                fcAPI.fcExrBeginImage(m_ctx, path, frame.width, frame.height);
                channels = System.Math.Min(channels, (int)fmt & 7);
                for (int i = 0; i < channels; ++i)
                {
                    fcAPI.fcExrAddLayerPixels(m_ctx, data, fmt, i, s_channelNames[i]);
                }
                fcAPI.fcExrEndImage(m_ctx);
            });
        }
    }
}
