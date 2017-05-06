using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    public class ExrContext : ImageSequenceRecorderContext
    {
        public class Command
        {
            static readonly string[] s_channelNames = { "R", "G", "B", "A" };
            DataPath m_path;
            string m_name;
            RenderTexture m_target;
            int m_channels;
            fcAPI.fcDeferredCall[] m_calls;

            public fcAPI.fcDeferredCall[] calls { get { return m_calls; } }


            public Command(DataPath path, string name, RenderTexture rt, int ch)
            {
                m_path = path;
                m_target = rt;
                m_channels = ch;

                m_calls = new fcAPI.fcDeferredCall[m_channels + 2];
                for (int i = 0; i < m_calls.Length; ++i)
                {
                    m_calls[i] = fcAPI.fcAllocateDeferredCall();
                }
            }

            public void Release()
            {
                if (m_calls != null)
                {
                    for (int i = 0; i < m_calls.Length; ++i) { m_calls[i].Release(); }
                    m_calls = null;
                }
            }

            public void Update(fcAPI.fcEXRContext ctx)
            {
                string path = m_path.GetFullPath() + "/" + m_name + "_" + Time.frameCount.ToString("0000") + ".exr";
                int ci = 0;

                m_calls[ci] = fcAPI.fcExrBeginImage(ctx, path, m_target.width, m_target.height, m_calls[ci]); ++ci;
                for (int i = 0; i < m_channels; ++i)
                {
                    m_calls[ci] = fcAPI.fcExrAddLayerTexture(ctx, m_target, i, s_channelNames[i], m_calls[ci]); ++ci;
                }
                m_calls[ci] = fcAPI.fcExrEndImage(ctx, m_calls[ci]); ++ci;
            }
        }

        fcAPI.fcEXRContext m_ctx;
        ImageSequenceRecorder m_recorder;
        List<Command> m_commands = new List<Command>();


        public override Type type { get { return Type.Exr; } }


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
            foreach(var c in cmd.calls)
            {
                cb.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), c);
            }
            m_commands.Add(cmd);
        }

        public override void Update()
        {
            foreach(var cmd in m_commands)
            {
                cmd.Update(m_ctx);
            }
        }

    }
}
