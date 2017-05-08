using UTJ.FrameCapturer;

namespace UnityEngine.Recorder.FrameRecorder.Example
{
    [ExecuteInEditMode]
    public class EXRRecorderSettings : ImageRecorderSettings
    {
        public string m_BaseFileName  = "exrFile";
        public string m_DestinationPath = "Recorder";
        public fcAPI.fcExrConfig m_ExrEncoderSettings = fcAPI.fcExrConfig.default_value;

        public override bool isValid
        {
            get
            {
                return base.isValid && !string.IsNullOrEmpty(m_DestinationPath) && !string.IsNullOrEmpty(m_BaseFileName);
            }
        }
    }
}
