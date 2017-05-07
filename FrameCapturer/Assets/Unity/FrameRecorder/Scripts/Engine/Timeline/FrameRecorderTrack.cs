using UnityEngine.Timeline;

namespace UnityEngine.Recorder.FrameRecorder.Timeline
{
    [System.Serializable]
    [TrackClipType(typeof(FrameRecorderClip))]
    [TrackMediaType(TimelineAsset.MediaType.Script)]
    [TrackColor(0.53f, 0.0f, 0.08f)]
    public class FrameRecorderTrack : TrackAsset
    {
    }
}
