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
        public int m_resolutionWidth = 640;
        public FrameRateMode m_frameRateMode = FrameRateMode.Variable;
        [Tooltip("relevant only if FrameRateMode is Constant")]
        public int m_framerate = 30;
        public int m_captureEveryNthFrame = 1;
        public int m_videoBitrate = 8192000;
        public int m_audioBitrate = 64000;
        public Shader m_shCopy;

        string m_output_file;
        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcMP4Config m_mp4conf = fcAPI.fcMP4Config.default_value;
        fcAPI.fcStream m_ostream;

        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratch_buffer;
        int m_callback;
        int m_num_video_frames;
        bool m_recording = false;


        void InitializeContext()
        {
            m_num_video_frames = 0;

            // initialize scratch buffer
            UpdateScratchBuffer();

            // initialize context and stream
            {
                m_mp4conf = fcAPI.fcMP4Config.default_value;
                m_mp4conf.video = m_captureVideo;
                m_mp4conf.audio = m_captureAudio;
                m_mp4conf.video_width = m_scratch_buffer.width;
                m_mp4conf.video_height = m_scratch_buffer.height;
                m_mp4conf.video_target_framerate = 60;
                m_mp4conf.video_target_bitrate = m_videoBitrate;
                m_mp4conf.audio_target_bitrate = m_audioBitrate;
                m_mp4conf.audio_sample_rate = AudioSettings.outputSampleRate;
                m_mp4conf.audio_num_channels = fcAPI.fcGetNumAudioChannels();
                m_ctx = fcAPI.fcMP4CreateContext(ref m_mp4conf);

                m_output_file = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
                m_ostream = fcAPI.fcCreateFileStream(GetOutputPath());
                fcAPI.fcMP4AddOutputStream(m_ctx, m_ostream);
            }

            // initialize command buffer
            {
                m_cb = new CommandBuffer();
                m_cb.name = "MP4OffscreenRecorder: copy frame buffer";
                m_cb.SetRenderTarget(m_scratch_buffer);
                m_cb.SetGlobalTexture("_TmpRenderTarget", m_target);
                m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_mat_copy, 0, 3);
            }
        }

        void ReleaseContext()
        {
            if (m_cb != null)
            {
                m_cb.Release();
                m_cb = null;
            }

            // scratch buffer is kept

            fcAPI.fcGuard(() =>
            {
                fcAPI.fcEraseDeferredCall(m_callback);
                m_callback = 0;

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
            });
        }

        void UpdateScratchBuffer()
        {
            var cam = GetComponent<Camera>();
            int capture_width = m_resolutionWidth;
            int capture_height = (int)((float)m_resolutionWidth / ((float)cam.pixelWidth / (float)cam.pixelHeight));

            if (m_scratch_buffer != null)
            {
                if (m_scratch_buffer.IsCreated() &&
                    m_scratch_buffer.width == capture_width && m_scratch_buffer.height == capture_height)
                {
                    // update is not needed
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
            m_recording = true;

            InitializeContext();
            GetComponent<Camera>().AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
            Debug.Log("MP4OffscreenRecorder.BeginRecording(): " + GetOutputPath()); return true;
        }

        public override bool EndRecording()
        {
            if (!m_recording) { return false; }
            m_recording = false;

            GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            ReleaseContext();
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
            m_shCopy = FrameCapturerUtils.GetFrameBufferCopyShader();
        }
#endif // UNITY_EDITOR

        void Start()
        {
            if (m_captureAudio)
            {
                fcAPI.fcMP4SetFAACPackagePath(Application.streamingAssetsPath + "/UTJ/FrameCapturer/FAAC_SelfBuild.zip");
            }
            fcAPI.fcSetModulePath(m_outputDir.GetPath());
            fcAPI.fcMP4DownloadCodecBegin();
        }

        void OnEnable()
        {
#if UNITY_EDITOR
            if (m_captureAudio && m_frameRateMode == FrameRateMode.Constant)
            {
                Debug.LogWarning("MP4OffscreenRecorder: capture audio with Constant frame rate mode will cause desync");
            }
#endif
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_shCopy);
        }

        void OnDisable()
        {
            EndRecording();
            ReleaseScratchBuffer();
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

        IEnumerator OnPostRender()
        {
            if (m_recording && m_captureVideo && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                double timestamp = Time.unscaledTime;
                if (m_frameRateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_framerate * m_num_video_frames;
                }

                m_callback = fcAPI.fcMP4AddVideoFrameTexture(m_ctx, m_scratch_buffer, timestamp, m_callback);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callback);
                m_num_video_frames++;
            }
        }
    }

}
