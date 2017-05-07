using System;

namespace UnityEngine.Recorder.FrameRecorder
{
    public class ImageRecorderSettings : FrameRecorderSettings
    {
        public bool m_PreserveSourceAspectRatio = true;
        public bool m_ScaleImage = false;
        public int m_Width = 1920;
        public int m_Height = 1080;

        public EImageSourceType m_InputType = EImageSourceType.GameDisplay;
        public int m_ScreenID = 0;
        public string m_CameraTag;

        public override bool isValid
        {
            get { return base.isValid && m_Width > 0 && m_Height > 0; }
        }
    }
}
