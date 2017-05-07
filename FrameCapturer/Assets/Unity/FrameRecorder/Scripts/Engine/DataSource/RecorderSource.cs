using System;

namespace UnityEngine.Recorder.FrameRecorder.DataSource
{
    public class RecorderSource : IDisposable
    {
        public int SourceID { get; set; }

        ~RecorderSource()
        {
            Dispose(false);
        }

        public void Dispose()
        {
            Dispose(true);
        }

        protected virtual void Dispose(bool disposing)
        {
            GC.SuppressFinalize(this);
        }
    }
}
