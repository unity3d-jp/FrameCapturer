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
        public int interval; // centi second
        public int keyframe;
        public int max_active_tasks;
        public int max_frame;
        public int max_data_size;

        public void SetDefault()
        {
            interval = 3; // 30ms
            keyframe = 0;
            max_active_tasks = 5;
            max_frame = 0;
            max_data_size = 1024 * 1024 * 3;
        }
    };

    [DllImport ("AddLibraryPath")] public static extern void    AddLibraryPath();
    [DllImport ("FrameCapturer")] public static extern IntPtr   fcGifCreateFile(string path, ref fcGifConfig conf);
    [DllImport ("FrameCapturer")] public static extern void     fcGifCloseFile(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifWriteFrame(IntPtr ctx, IntPtr tex);


    public RenderTexture m_rt;
    public float m_resolution_scale = 1.0f;
    IntPtr m_gif;


    void OnEnable()
    {
        AddLibraryPath();

        m_rt = new RenderTexture(256, 256, 32, RenderTextureFormat.ARGB32);
        m_rt.enableRandomWrite = true;
        m_rt.Create();
        GetComponent<Camera>().targetTexture = m_rt;

        if (m_rt == null) { return; }

        fcGifConfig conf = new fcGifConfig();
        conf.SetDefault();
        conf.width = m_rt.width;
        conf.height = m_rt.height;
        m_gif = fcGifCreateFile("hoge.gif", ref conf);
    }

    void OnDisable()
    {
        fcGifCloseFile(m_gif);
        if (m_rt == null) { return; }

    }

    void OnPostRender()
    {
        if (m_rt == null) { return; }
        fcGifWriteFrame(m_gif, m_rt.GetNativeTexturePtr());
    }

}
