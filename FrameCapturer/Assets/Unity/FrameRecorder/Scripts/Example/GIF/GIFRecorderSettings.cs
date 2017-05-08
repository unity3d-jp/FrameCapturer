using UTJ.FrameCapturer;

namespace UnityEngine.Recorder.FrameRecorder.Example
{
    [ExecuteInEditMode]
    public class GIFRecorderSettings : ImageRecorderSettings
    {
        public string m_BaseFileName  = "webmFile";
        public string m_DestinationPath = "Recorder";
        public fcAPI.fcGifConfig m_GifEncoderSettings = fcAPI.fcGifConfig.default_value;

        public override bool isValid
        {
            get
            {
                return base.isValid && !string.IsNullOrEmpty(m_DestinationPath) && !string.IsNullOrEmpty(m_BaseFileName);
            }
        }
    }
}
