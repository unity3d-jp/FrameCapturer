using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR

namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/MP4Capturer")]
    [RequireComponent(typeof(Camera))]
    public class MP4Capturer : IMP4Capturer
    {
        public bool m_video = true;
        public bool m_audio = true;
        public int m_resolution_width = 300;
        public int m_capture_every_nth_frames = 2;
        public int m_video_bitrate = 1024000;
        public int m_audio_bitrate = 64000;
        public Shader m_sh_copy;

        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcMP4Config m_mp4conf = fcAPI.fcMP4Config.default_value;
        fcAPI.fcStream m_ostream;

        Material m_mat_copy;
        Mesh m_quad;
        CommandBuffer m_cb;
        RenderTexture m_scratch_buffer;
        Camera m_cam;
        int m_num_video_frame;
        bool m_record = false;



        public override bool recode
        {
            get { return m_record; }
            set
            {
                m_record = value;
                if (m_record && m_ctx.ptr == IntPtr.Zero)
                {
                    ResetRecordingState();
                }
            }
        }

        public override RenderTexture GetScratchBuffer() { return m_scratch_buffer; }

        public override void ResetRecordingState()
        {
            fcAPI.fcMP4DestroyContext(m_ctx);
            fcAPI.fcDestroyStream(m_ostream);

            m_ctx.ptr = IntPtr.Zero;
            if (m_scratch_buffer != null)
            {
                m_scratch_buffer.Release();
                m_scratch_buffer = null;
            }

            int capture_width = m_resolution_width;
            int capture_height = (int)((float)m_resolution_width / ((float)m_cam.pixelWidth / (float)m_cam.pixelHeight));
            m_scratch_buffer = new RenderTexture(capture_width, capture_height, 0, RenderTextureFormat.ARGB32);
            m_scratch_buffer.wrapMode = TextureWrapMode.Repeat;
            m_scratch_buffer.Create();

            m_num_video_frame = 0;
            m_mp4conf = fcAPI.fcMP4Config.default_value;
            m_mp4conf.video = m_video;
            m_mp4conf.audio = m_audio;
            m_mp4conf.video_width = m_scratch_buffer.width;
            m_mp4conf.video_height = m_scratch_buffer.height;
            m_mp4conf.video_max_framerate = 60;
            m_mp4conf.video_bitrate = m_video_bitrate;
            m_mp4conf.audio_bitrate = m_audio_bitrate;
            m_mp4conf.audio_sampling_rate = AudioSettings.outputSampleRate;
            switch (AudioSettings.speakerMode)
            {
                case AudioSpeakerMode.Mono: m_mp4conf.audio_num_channels = 1; break;
                case AudioSpeakerMode.Stereo: m_mp4conf.audio_num_channels = 2; break;
                case AudioSpeakerMode.Quad: m_mp4conf.audio_num_channels = 4; break;
                case AudioSpeakerMode.Surround: m_mp4conf.audio_num_channels = 5; break;
                case AudioSpeakerMode.Mode5point1: m_mp4conf.audio_num_channels = 6; break;
                case AudioSpeakerMode.Mode7point1: m_mp4conf.audio_num_channels = 8; break;
                case AudioSpeakerMode.Prologic: m_mp4conf.audio_num_channels = 6; break;
            }
            m_ctx = fcAPI.fcMP4CreateContext(ref m_mp4conf);


            string path = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            m_ostream = fcAPI.fcCreateFileStream(path);
            fcAPI.fcMP4AddOutputStream(m_ctx, m_ostream);
        }

        public fcAPI.fcMP4Context GetMP4Context() { return m_ctx; }

#if UNITY_EDITOR
        void Reset()
        {
            m_sh_copy = AssetDatabase.LoadAssetAtPath("Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader", typeof(Shader)) as Shader;
        }
#endif // UNITY_EDITOR

        void Start()
        {
#if UNITY_EDITOR
#else
            fcAPI.fcSetModulePath(Application.persistentDataPath);
#endif
            fcAPI.fcMP4DownloadCodec(null);
        }

        void OnEnable()
        {
            m_cam = GetComponent<Camera>();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_mat_copy = new Material(m_sh_copy);
            if (m_cam.targetTexture != null)
            {
                m_mat_copy.EnableKeyword("OFFSCREEN");
            }

            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");
                m_cb = new CommandBuffer();
                m_cb.name = "GifCapturer: copy frame buffer";
                m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                // tid は意図的に開放しない
                m_cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
            }
            ResetRecordingState();
        }

        void OnDisable()
        {
            fcAPI.fcMP4DestroyContext(m_ctx);
            fcAPI.fcDestroyStream(m_ostream);
            m_ctx.ptr = IntPtr.Zero;

            m_cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            m_cb.Release();
            m_cb = null;

            m_scratch_buffer.Release();
            m_scratch_buffer = null;
        }

        IEnumerator OnPostRender()
        {
            if (m_record && m_video)
            {
                yield return new WaitForEndOfFrame();

                int frame = m_num_video_frame++;
                if (frame % m_capture_every_nth_frames == 0)
                {
                    m_mat_copy.SetPass(0);
                    Graphics.SetRenderTarget(m_scratch_buffer);
                    Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                    Graphics.SetRenderTarget(null);
                    fcAPI.fcMP4AddVideoFrameTexture(m_ctx, m_scratch_buffer.GetNativeTexturePtr());
                }
            }
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_record && m_audio)
            {
                if(channels != m_mp4conf.audio_num_channels) {
                    Debug.LogError("MP4Capturer: audio channels mismatch!");
                    return;
                }

                fcAPI.fcMP4AddAudioFrame(m_ctx, samples, samples.Length);
            }
        }
    }

}
