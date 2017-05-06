using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public class ExrContext : ImageSequenceRecorderContext
    {
        static readonly string[] s_channelNames = { "R", "G", "B", "A" };
        ImageSequenceRecorder m_recorder;
        fcAPI.fcEXRContext m_ctx;


        public override Type type { get { return Type.Exr; } }


        public override void Initialize(ImageSequenceRecorder recorder)
        {
            m_recorder = recorder;
            var exrconf = fcAPI.fcExrConfig.default_value;
            m_ctx = fcAPI.fcExrCreateContext(ref exrconf);
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void Export(RenderTexture frame, int channels, string name)
        {
            string path = m_recorder.outputDir.GetFullPath() + "/" + name + "_" + Time.frameCount.ToString("0000") + ".exr";

            fcAPI.fcExrBeginImage(m_ctx, path, frame.width, frame.height);
            fcAPI.fcLock(frame, (data, fmt) => {
                channels = System.Math.Min(channels, (int)fmt & 7);
                for (int i = 0; i < channels; ++i)
                {
                    fcAPI.fcExrAddLayerPixels(m_ctx, data, fmt, i, s_channelNames[i], false);
                }
            });
            fcAPI.fcExrEndImage(m_ctx);
        }
    }
}
