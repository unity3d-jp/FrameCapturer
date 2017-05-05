using System.Collections.Generic;
using UnityEngine;


namespace UTJ.FrameCapturer
{
    public class ExrContext : ImageSequenceRecorderContext
    {
        fcAPI.fcEXRContext m_ctx;
        List<int> m_callbacks;


        public override Type type { get { return Type.Exr; } }


        public override void Initialize(ImageSequenceRecorder recorder)
        {

        }

        public override void Release()
        {

        }


        public override void BeginFrame()
        {

        }

        public override void Export(RenderTexture frame, int channels, string path)
        {

        }

        public override void EndFrame()
        {

        }

    }
}
