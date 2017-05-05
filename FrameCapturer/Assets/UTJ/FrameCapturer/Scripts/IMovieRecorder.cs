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
        public abstract bool BeginRecording();
        public abstract bool EndRecording();
        public abstract bool recording { get; }
        public abstract string GetOutputPath();
    }


    public abstract class IMovieRecoerderUI : MonoBehaviour
    {
        public abstract IMovieRecorder GetRecorder();

        public abstract bool record { get; set; }
        public abstract string GetOutputPath();
    }
}
