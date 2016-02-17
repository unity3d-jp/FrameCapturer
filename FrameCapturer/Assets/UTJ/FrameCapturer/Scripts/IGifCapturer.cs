using UnityEngine;


namespace UTJ
{
    public abstract class IGifCapturer : MonoBehaviour
    {
        public abstract bool record { get; set; }
        public abstract bool WriteFile(string path = "", int begin_frame = 0, int end_frame = -1);
        public abstract int WriteMemory(System.IntPtr dst_buf, int begin_frame = 0, int end_frame = -1);
        public abstract RenderTexture GetScratchBuffer();
        public abstract void ResetRecordingState();
        public abstract void EraseFrame(int begin_frame, int end_frame);
        public abstract int GetExpectedFileSize(int begin_frame = 0, int end_frame = -1);
        public abstract int GetFrameCount();
        public abstract void GetFrameData(RenderTexture rt, int frame);
    }
}
