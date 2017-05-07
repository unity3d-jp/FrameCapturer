using System;

namespace UnityEngine.Recorder.FrameRecorder.DataSource
{
    public abstract class RenderTextureSource : RecorderSource
    {
        public RenderTexture buffer { get; set; }

        public RenderTexture UnlinkBuffer()
        {
            var temp = buffer;
            buffer = null;
            return temp;
        }

        public void ReleaseBuffer()
        {
            if (buffer != null)
            {
                buffer.Release();
                buffer = null;
            }
        }

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);

            if (disposing)
                ReleaseBuffer();
        }
    }
}
