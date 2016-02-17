using UnityEngine;


namespace UTJ
{
    public abstract class IMP4Capturer : MonoBehaviour
    {
        public abstract bool record { get; set; }
        public abstract string GetOutputPath();
        public abstract RenderTexture GetScratchBuffer();
        public abstract void FlushMP4();
        public abstract void ResetRecordingState();
        public abstract int GetFrameCount();
    }
}
