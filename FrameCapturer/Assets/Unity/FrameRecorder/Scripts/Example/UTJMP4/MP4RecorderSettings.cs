using UnityEngine;
using UnityEngine.Recorder.FrameRecorder;

namespace UTJ.FrameRecorder
{
    [ExecuteInEditMode]
    public class MP4RecorderSettings : ImageRecorderSettings
    {
        public int m_VideoBitrate = 8192000;
        public string m_BaseFileName  = "mp4File";
        public string m_DestinationPath = "Recorder";

        public override bool isValid
        {
            get
            {
                return base.isValid && !string.IsNullOrEmpty(m_DestinationPath) && !string.IsNullOrEmpty(m_BaseFileName);
            }
        }
    }
}
