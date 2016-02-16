using UnityEngine;


namespace UTJ
{
    public abstract class IMP4Capturer : MonoBehaviour
    {
        public abstract bool recode { get; set; }
        public abstract RenderTexture GetScratchBuffer();
        public abstract void ResetRecordingState();
    }
}
