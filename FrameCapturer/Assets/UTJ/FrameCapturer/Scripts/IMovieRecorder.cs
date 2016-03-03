using UnityEngine;


namespace UTJ
{

    public abstract class IImageSequenceRecorder : MonoBehaviour
    {
        public abstract int beginFrame { get; set; }
        public abstract int endFrame { get; set; }
    }

    public abstract class IMovieRecorder : MonoBehaviour
    {
        public abstract bool IsSeekable();
        public abstract bool IsEditable();

        public abstract bool BeginRecording();
        public abstract bool EndRecording();
        public abstract bool recording { get; set; }
        public abstract string GetOutputPath();

        public abstract bool Flush();
        public abstract RenderTexture GetScratchBuffer();
        public abstract int GetFrameCount();

        // available only if IsSeekable() is true
        public abstract bool Flush(int begin_frame, int end_frame);
        // available only if IsSeekable() is true
        public abstract int GetExpectedFileSize(int begin_frame, int end_frame);
        // available only if IsSeekable() is true
        public abstract void GetFrameData(RenderTexture rt, int frame);

        // available only if IsEditable() is true
        public abstract void EraseFrame(int begin_frame, int end_frame);
    }



    public abstract class IMovieRecoerderUI : MonoBehaviour
    {
        public abstract IMovieRecorder GetRecorder();

        public abstract bool record { get; set; }
        public abstract string GetOutputPath();
        public abstract void Flush();
        public abstract void Restart();
    }
}
