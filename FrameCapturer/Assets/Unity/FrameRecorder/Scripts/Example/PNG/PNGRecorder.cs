using System;
using System.IO;
using UnityEngine.Recorder.FrameRecorder.DataSource;
using UnityEngine.Recorder.FrameRecorder.Utilities;

namespace UnityEngine.Recorder.FrameRecorder
{
    [FrameRecorderClass]
    public class PNGRecorder : RenderTextureRecorder<PNGRecorderSettings>
    {
        public static RecorderInfo GetRecorderInfo()
        {
            return RecorderInfo.Instantiate<PNGRecorder, PNGRecorderSettings>("Video", "PNG Recorder");
        }

        public override void RecordFrame(RecordingSession ctx)
        {
            if (m_BoxedSources.Count != 1)
                throw new Exception("Unsupported number of sources");

            var source = (RenderTextureSource)m_BoxedSources[0].m_Source;

            var width = source.buffer.width;
            var height = source.buffer.height;
            var tex = new Texture2D(width, height, TextureFormat.RGB24, false);
            var backupActive = RenderTexture.active;
            RenderTexture.active = source.buffer;
            tex.ReadPixels(new Rect(0, 0, width, height), 0, 0);
            tex.Apply();
            RenderTexture.active = backupActive;

            var bytes = tex.EncodeToPNG();
            UnityHelpers.Destroy(tex);

            if (!Directory.Exists(m_Settings.m_DestinationPath))
                Directory.CreateDirectory(m_Settings.m_DestinationPath);

            File.WriteAllBytes(BuildOutputPath(ctx), bytes);
        }

        string BuildOutputPath(RecordingSession ctx)
        {
            var outputPath = m_Settings.m_DestinationPath;
            if (outputPath.Length > 0 && !outputPath.EndsWith("/"))
                outputPath += "/";
            outputPath += m_OutputFile + (settings as PNGRecorderSettings).m_BaseFileName + recordedFramesCount + ".png";
            return outputPath;
        }
    }
}
