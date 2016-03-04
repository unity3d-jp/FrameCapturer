using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;

namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/MP4OffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class MP4OffscreenRecorder : IMovieRecorder
    {
        public enum FrameRateMode
        {
            Variable,
            Constant,
        }

        public RenderTexture m_target;
        [Tooltip("output directory. filename is generated automatically.")]
        public DataPath m_outputDir = new DataPath(DataPath.Root.PersistentDataPath, "");
        public bool m_captureVideo = true;
        public bool m_captureAudio = true;
        public int m_resolutionWidth = 300;
        public FrameRateMode m_frameRateMode = FrameRateMode.Variable;
        [Tooltip("relevant only if FrameRateMode is Constant")]
        public int m_framerate = 30;
        public int m_captureEveryNthFrame = 1;
        public int m_videoBitrate = 1024000;
        public int m_audioBitrate = 64000;
        public Shader m_sh_copy;

        string m_output_file;
        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcMP4Config m_mp4conf = fcAPI.fcMP4Config.default_value;
        fcAPI.fcStream m_ostream;

        Material m_mat_copy;
        Mesh m_quad;
        RenderTexture m_scratch_buffer;
        int m_num_video_frames;
        bool m_recording = false;

        void UpdateScratchBuffer()
        {
            int capture_width = m_resolutionWidth;
            int capture_height = (int)(m_resolutionWidth / ((float)m_target.width / (float)m_target.height));

            if ( m_scratch_buffer != null)
            {
                // update is not needed
                if( m_scratch_buffer.IsCreated() &&
                    m_scratch_buffer.width == capture_width && m_scratch_buffer.height == capture_height)
                {
                    return;
                }
                else
                {
                    ReleaseScratchBuffer();
                }
            }

            m_scratch_buffer = new RenderTexture(capture_width, capture_height, 0, RenderTextureFormat.ARGB32);
            m_scratch_buffer.wrapMode = TextureWrapMode.Repeat;
            m_scratch_buffer.Create();
        }

        void ReleaseScratchBuffer()
        {
            if (m_scratch_buffer != null)
            {
                m_scratch_buffer.Release();
                m_scratch_buffer = null;
            }
        }

        public override bool IsSeekable() { return false; }
        public override bool IsEditable() { return false; }

        public override bool BeginRecording()
        {
            if (m_recording) { return false; }

            UpdateScratchBuffer();

            m_num_video_frames = 0;
            m_mp4conf = fcAPI.fcMP4Config.default_value;
            m_mp4conf.video = m_captureVideo;
            m_mp4conf.audio = m_captureAudio;
            m_mp4conf.video_width = m_scratch_buffer.width;
            m_mp4conf.video_height = m_scratch_buffer.height;
            m_mp4conf.video_max_framerate = 60;
            m_mp4conf.video_bitrate = m_videoBitrate;
            m_mp4conf.audio_bitrate = m_audioBitrate;
            m_mp4conf.audio_sampling_rate = AudioSettings.outputSampleRate;
            m_mp4conf.audio_num_channels = fcAPI.fcGetNumAudioChannels();
            m_ctx = fcAPI.fcMP4CreateContext(ref m_mp4conf);

            m_output_file = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            m_ostream = fcAPI.fcCreateFileStream(GetOutputPath());
            fcAPI.fcMP4AddOutputStream(m_ctx, m_ostream);

            Debug.Log("MP4OffscreenRecorder.BeginRecording(): " + GetOutputPath()); return true;
        }

        public override bool EndRecording()
        {
            if (!m_recording) { return false; }
            m_recording = false;

            if (m_ctx.ptr != IntPtr.Zero)
            {
                fcAPI.fcMP4DestroyContext(m_ctx);
                m_ctx.ptr = IntPtr.Zero;
            }
            if (m_ostream.ptr != IntPtr.Zero)
            {
                fcAPI.fcDestroyStream(m_ostream);
                m_ostream.ptr = IntPtr.Zero;
            }

            Debug.Log("MP4OffscreenRecorder.EndRecording(): " + GetOutputPath());
            return true;
        }

        public override bool recording
        {
            get { return m_recording; }
            set { m_recording = value; }
        }


        public override string GetOutputPath()
        {
            string ret = m_outputDir.GetPath();
            if(ret.Length > 0) { ret += "/"; }
            ret += m_output_file;
            return ret;
        }
        public override RenderTexture GetScratchBuffer() { return m_scratch_buffer; }
        public override int GetFrameCount() { return m_num_video_frames; }

        public override bool Flush()
        {
            return EndRecording();
        }

        public override bool Flush(int begin_frame, int end_frame)
        {
            return EndRecording();
        }

        // N/A
        public override int GetExpectedFileSize(int begin_frame, int end_frame)
        {
            return 0;
        }

        // N/A
        public override void GetFrameData(RenderTexture rt, int frame)
        {
        }

        // N/A
        public override void EraseFrame(int begin_frame, int end_frame)
        {
        }


        public fcAPI.fcMP4Context GetMP4Context() { return m_ctx; }

#if UNITY_EDITOR
        void Reset()
        {
            m_sh_copy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        void Start()
        {
            fcAPI.fcSetModulePath(Application.persistentDataPath);
            fcAPI.fcMP4DownloadCodecBegin();
        }

        void OnEnable()
        {
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_sh_copy);
        }

        void OnDisable()
        {
            EndRecording();
            ReleaseScratchBuffer();
        }

        IEnumerator OnPostRender()
        {
            if (m_recording && m_captureVideo)
            {
                yield return new WaitForEndOfFrame();

                if (Time.frameCount % m_captureEveryNthFrame == 0)
                {
                    double timestamp = -1.0;
                    if(m_frameRateMode == FrameRateMode.Constant)
                    {
                        timestamp = 1.0 / m_framerate * m_num_video_frames;
                    }

                    m_mat_copy.SetTexture("_TmpRenderTarget", m_target);
                    m_mat_copy.SetPass(3);
                    Graphics.SetRenderTarget(m_scratch_buffer);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);

                    fcAPI.fcMP4AddVideoFrameTexture(m_ctx, m_scratch_buffer, timestamp);
                    m_num_video_frames++;
                }
            }
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_captureAudio)
            {
                if(channels != m_mp4conf.audio_num_channels) {
                    Debug.LogError("MP4OffscreenRecorder: audio channels mismatch!");
                    return;
                }

                fcAPI.fcMP4AddAudioFrame(m_ctx, samples, samples.Length);
            }
        }
    }

}
