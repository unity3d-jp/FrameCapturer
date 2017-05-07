using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class PngContext : ImageSequenceRecorderContext
    {
        [SerializeField] ImageSequenceRecorder m_recorder;
        fcAPI.fcPngContext m_ctx;


        public override Type type { get { return Type.Png; } }

        public override void Initialize(ImageSequenceRecorder recorder)
        {
            m_recorder = recorder;
            var pngconf = m_recorder.pngConfig;
            m_ctx = fcAPI.fcPngCreateContext(ref pngconf);
        }

        public override void Release()
        {
            m_ctx.Release();
        }

        public override void Export(RenderTexture frame, int channels, string name)
        {
            if (!m_recorder || !m_ctx) { return; }
            string path = m_recorder.outputDir.GetFullPath() + "/" + name + "_" + m_recorder.frame.ToString("0000") + ".png";

            fcAPI.fcLock(frame, (data, fmt) =>
            {
                fcAPI.fcPngExportPixels(m_ctx, path, data, frame.width, frame.height, fmt, channels);
            });
        }
    }
}
