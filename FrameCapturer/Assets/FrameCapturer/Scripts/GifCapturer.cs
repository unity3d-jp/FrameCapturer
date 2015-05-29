using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;


[RequireComponent(typeof(Camera))]
public class GifCapturer : MonoBehaviour
{
    public struct fcGifConfig
    {
        public int width;
        public int height;
        public int delay_csec; // * centi second! *
        public int keyframe;
        public int max_active_tasks;
        public int max_frame;
        public int max_data_size;

        public void SetDefault()
        {
            delay_csec = 3; // 30ms
            keyframe = 0;
            max_active_tasks = 5;
            max_frame = 0;
            max_data_size = 1024 * 1024 * 3;
        }
    };

    [DllImport ("AddLibraryPath")] public static extern void    AddLibraryPath();
    [DllImport ("FrameCapturer")] public static extern IntPtr   fcGifCreateContext(ref fcGifConfig conf);
    [DllImport ("FrameCapturer")] public static extern void     fcGifDestroyContext(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifAddFrame(IntPtr ctx, IntPtr tex);
    [DllImport ("FrameCapturer")] public static extern void     fcGifClearFrame(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifWriteFile(IntPtr ctx, string path);


    public RenderTexture m_rt;
    public float m_resolution_scale = 1.0f;
    public float m_delay_csec = 3;
    public float m_keyframe = 0;
    public int m_max_frame = 0;
    public int m_max_data_size = 0;
    public int m_max_active_tasks = 5;
    IntPtr m_gif;
    int m_frame;


    public void WriteFile(string path)
    {
        if (m_gif != IntPtr.Zero)
        {
            fcGifWriteFile(m_gif, path);
        }
    }


    void OnEnable()
    {
        AddLibraryPath();

        m_rt = new RenderTexture(256, 256, 32, RenderTextureFormat.ARGB32);
        m_rt.Create();
        GetComponent<Camera>().targetTexture = m_rt;

        if (m_rt == null) { return; }

        fcGifConfig conf = new fcGifConfig();
        conf.SetDefault();
        conf.width = m_rt.width;
        conf.height = m_rt.height;
        conf.max_frame = m_max_frame;
        conf.max_data_size = m_max_data_size;
        conf.max_active_tasks = m_max_active_tasks;
        m_gif = fcGifCreateContext(ref conf);
    }

    void OnDisable()
    {
        WriteFile("hoge.gif");

        fcGifDestroyContext(m_gif);
        if (m_rt == null) { return; }

    }

    void OnPostRender()
    {
        if (m_rt == null) { return; }
        fcGifAddFrame(m_gif, m_rt.GetNativeTexturePtr());
    }

}
