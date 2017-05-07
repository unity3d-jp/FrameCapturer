using System;
using System.Collections;

namespace UnityEngine.Recorder.FrameRecorder.Timeline
{
    // the purpose of this class is to signal the FrameRecorderPlayable when frame is done.
    [ExecuteInEditMode]
    class WaitForEndOfFrameComponent : MonoBehaviour
    {
        [NonSerialized]
        public FrameRecorderPlayable m_playable;

        public IEnumerator WaitForEndOfFrame()
        {
            yield return new WaitForEndOfFrame();
            m_playable.FrameEnded();
        }

        public void LateUpdate()
        {
            StartCoroutine(WaitForEndOfFrame());
        }
    }
}
