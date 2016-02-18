using UnityEngine;


namespace UTJ
{
    public abstract class IMovieRecorder : MonoBehaviour
    {
        public abstract bool record { get; set; }
        public abstract string GetOutputPath();

        public abstract bool FlushFile();
        public abstract RenderTexture GetScratchBuffer();
        public abstract void ResetRecordingState();
        public abstract int GetFrameCount();
    }

    public abstract class IEditableMovieRecorder : IMovieRecorder
    {
        public abstract bool FlushFile(int begin_frame, int end_frame);

        public abstract void EraseFrame(int begin_frame, int end_frame);
        public abstract int GetExpectedFileSize(int begin_frame, int end_frame);
        public abstract void GetFrameData(RenderTexture rt, int frame);
    }



    public abstract class IMovieRecoerderUI : MonoBehaviour
    {
        public abstract IMovieRecorder GetRecorder();

        public abstract bool record { get; set; }
        public abstract string GetOutputPath();
        public abstract void FlushFile();
        public abstract void ResetRecordingState();
    }
}
