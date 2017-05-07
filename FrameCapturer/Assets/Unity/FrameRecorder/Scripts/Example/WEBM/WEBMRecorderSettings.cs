using UTJ.FrameCapturer;

namespace UnityEngine.Recorder.FrameRecorder
{
    [ExecuteInEditMode]
    public class WEBMRecorderSettings : ImageRecorderSettings
    {
        public string m_BaseFileName  = "webmFile";
        public string m_DestinationPath = "Recorder";
        public fcAPI.fcWebMConfig m_WebMConfig = fcAPI.fcWebMConfig.default_value;

        public override bool isValid
        {
            get
            {
                return base.isValid && !string.IsNullOrEmpty(m_DestinationPath) && !string.IsNullOrEmpty(m_BaseFileName);
            }
        }
    }
}
