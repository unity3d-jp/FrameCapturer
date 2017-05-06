using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public class PngContext : ImageSequenceRecorderContext
    {
        ImageSequenceRecorder m_recorder;
        fcAPI.fcPNGContext m_ctx;


        public override Type type { get { return Type.Png; } }

        public override void Initialize(ImageSequenceRecorder recorder)
        {
            m_recorder = recorder;
            var pngconf = fcAPI.fcPngConfig.default_value;
            m_ctx = fcAPI.fcPngCreateContext(ref pngconf);
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void Export(RenderTexture frame, int channels, string name)
        {
            string path = m_recorder.outputDir.GetFullPath() + "/" + name + "_" + Time.frameCount.ToString("0000") + ".png";

            fcAPI.fcLock(frame, (data, fmt) =>
            {
                fcAPI.fcPngExportPixels(m_ctx, path, data, frame.width, frame.height, fmt, false);
            });
        }
    }
}
