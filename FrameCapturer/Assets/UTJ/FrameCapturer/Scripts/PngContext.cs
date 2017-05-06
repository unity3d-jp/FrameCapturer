using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public class PngContext : ImageSequenceRecorderContext
    {
        class Command
        {
            DataPath m_path;
            string m_name;
            RenderTexture m_target;
            int m_channels;
            fcAPI.fcDeferredCall m_call;

            public fcAPI.fcDeferredCall call { get { return m_call; } }


            public Command(DataPath path, string name, RenderTexture rt, int ch)
            {
                m_path = path;
                m_target = rt;
                m_channels = ch;
                m_call = fcAPI.fcAllocateDeferredCall();
            }

            public void Release()
            {
                m_call.Release();
            }

            public void Update(fcAPI.fcPNGContext ctx)
            {
                string path = m_path.GetFullPath() + "/" + m_name + "_" + Time.frameCount.ToString("0000") + ".png";
                m_call = fcAPI.fcPngExportTexture(ctx, path, m_target, m_call);
            }
        }

        fcAPI.fcPNGContext m_ctx;
        ImageSequenceRecorder m_recorder;
        List<Command> m_commands;


        public override Type type { get { return Type.Png; } }

        public override void Initialize(ImageSequenceRecorder recorder)
        {
            m_recorder = recorder;
        }

        public override void Release()
        {
            foreach (var cb in m_commands) { cb.Release(); }
            m_commands.Clear();
        }

        public override void AddCommand(CommandBuffer cb, RenderTexture frame, int channels, string name)
        {
            var cmd = new Command(m_recorder.outputDir, name, frame, channels);
            cb.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), cmd.call);
            m_commands.Add(cmd);
        }

        public override void Update()
        {
            foreach (var cmd in m_commands)
            {
                cmd.Update(m_ctx);
            }
        }

    }
}
