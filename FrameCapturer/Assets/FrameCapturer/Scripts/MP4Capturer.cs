using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


[AddComponentMenu("FrameCapturer/MP4Capturer")]
[RequireComponent(typeof(Camera))]
public class MP4Capturer : MovieCapturer
{
    public bool m_video = true;
    public bool m_audio = false;
    public int m_resolution_width = 300;
    public int m_capture_every_n_frames = 2;
    public int m_frame_rate = 30;
    public int m_max_frame = 1800;
    public int m_keyframe = 60;
    public int m_video_bitrate = 264000;
    public int m_audio_bitrate = 64000;
    public int m_max_data_size = 0;
    public int m_max_active_tasks = 0;
    public Shader m_sh_copy;

    FrameCapturer.fcMP4Context m_ctx;
    Material m_mat_copy;
    Mesh m_quad;
    CommandBuffer m_cb;
    RenderTexture m_scratch_buffer;
    Camera m_cam;
    int m_frame;
    bool m_recode = false;



    MP4Capturer()
    {
        // Plugins/* を dll サーチパスに追加
        try { FrameCapturer.AddLibraryPath(); } catch { }
    }

    public override bool recode
    {
        get { return m_recode; }
        set { m_recode = value; }
    }

    public override bool WriteFile(string path = "", int begin_frame = 0, int end_frame = -1)
    {
        bool ret = false;
        if (m_ctx.ptr != IntPtr.Zero)
        {
            if (path.Length==0)
            {
                path = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
            }
            ret = FrameCapturer.fcMP4WriteFile(m_ctx, path, begin_frame, end_frame);
            Debug.Log("MP4Capturer.WriteFile() : " + path);
        }
        return false;
    }

    public override int WriteMemory(System.IntPtr dst_buf, int begin_frame = 0, int end_frame = -1)
    {
        int ret = 0;
        if (m_ctx.ptr != IntPtr.Zero)
        {
            ret = FrameCapturer.fcMP4WriteMemory(m_ctx, dst_buf, begin_frame, end_frame);
            Debug.Log("MP4Capturer.WriteMemry()");
        }
        return ret;
    }

    public override RenderTexture GetScratchBuffer() { return m_scratch_buffer; }

    public override void ResetRecordingState()
    {
        FrameCapturer.fcMP4DestroyContext(m_ctx);
        m_ctx.ptr = IntPtr.Zero;
        if(m_scratch_buffer != null)
        {
            m_scratch_buffer.Release();
            m_scratch_buffer = null;
        }

        int capture_width = m_resolution_width;
        int capture_height = (int)((float)m_resolution_width / ((float)m_cam.pixelWidth / (float)m_cam.pixelHeight));
        m_scratch_buffer = new RenderTexture(capture_width, capture_height, 0, RenderTextureFormat.ARGB32);
        m_scratch_buffer.wrapMode = TextureWrapMode.Repeat;
        m_scratch_buffer.Create();

        m_frame = 0;
        FrameCapturer.fcMP4Config conf = default(FrameCapturer.fcMP4Config);
        conf.setDefaults();
        conf.video = m_video ? 1 : 0;
        conf.audio = m_audio ? 1 : 0;
        conf.video_width = m_scratch_buffer.width;
        conf.video_height = m_scratch_buffer.height;
        conf.video_framerate = m_frame_rate;
        conf.video_bitrate = m_video_bitrate;
        conf.video_max_frame = m_max_frame;
        conf.video_max_data_size = m_max_data_size;
        conf.audio_bitrate = m_audio_bitrate;
        conf.audio_sampling_rate = AudioSettings.outputSampleRate;
        switch (AudioSettings.speakerMode)
        {
            case AudioSpeakerMode.Mono: conf.audio_num_channels = 1; break;
            case AudioSpeakerMode.Stereo: conf.audio_num_channels = 2; break;
            // todo: maybe need more case
        }
        m_ctx = FrameCapturer.fcMP4CreateContext(ref conf);
    }

    public override void EraseFrame(int begin_frame, int end_frame)
    {
        FrameCapturer.fcMP4EraseFrame(m_ctx, begin_frame, end_frame);
    }

    public override int GetExpectedFileSize(int begin_frame = 0, int end_frame = -1)
    {
        return FrameCapturer.fcMP4GetExpectedDataSize(m_ctx, begin_frame, end_frame);
    }

    public override int GetFrameCount()
    {
        return FrameCapturer.fcMP4GetFrameCount(m_ctx);
    }

    public override void GetFrameData(RenderTexture rt, int frame)
    {
        FrameCapturer.fcMP4GetFrameData(m_ctx, rt.GetNativeTexturePtr(), frame);
    }

    public FrameCapturer.fcMP4Context GetGifContext() { return m_ctx; }

#if UNITY_EDITOR
    void Reset()
    {
        m_sh_copy = AssetDatabase.LoadAssetAtPath("Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader", typeof(Shader)) as Shader;
    }
#endif // UNITY_EDITOR

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
        FrameCapturer.fcMP4DestroyContext(m_ctx);
        m_ctx.ptr = IntPtr.Zero;

        m_cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
        m_cb.Release();
        m_cb = null;

        m_scratch_buffer.Release();
        m_scratch_buffer = null;
    }

    IEnumerator OnPostRender()
    {
        if (m_recode)
        {
            yield return new WaitForEndOfFrame();

            int frame = m_frame++;
            if (frame % m_capture_every_n_frames == 0)
            {
                m_mat_copy.SetPass(0);
                Graphics.SetRenderTarget(m_scratch_buffer);
                Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                Graphics.SetRenderTarget(null);
                FrameCapturer.fcMP4AddVideoFrameTexture(m_ctx, m_scratch_buffer.GetNativeTexturePtr());
            }
        }
    }

    void OnAudioFilterRead(float[] samples, int num_samples)
    {
        if (m_audio)
        {
            FrameCapturer.fcMP4AddAudioSamples(m_ctx, samples, num_samples);
        }
    }
}
