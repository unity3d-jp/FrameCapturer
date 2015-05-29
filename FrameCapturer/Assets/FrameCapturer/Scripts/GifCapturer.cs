using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;


[RequireComponent(typeof(Camera))]
public class GifCapturer : MonoBehaviour
{
    public RenderTexture m_rt;
    public bool m_capture_on_postrender = true;
    public float m_resolution_scale = 1.0f;
    public int m_delay_csec = 3;
    public int m_max_frame = 0;
    public int m_max_data_size = 0;
    public int m_max_active_tasks = 5;
    public int m_keyframe = 0;
    IntPtr m_gif;
    int m_frame;


    public void WriteFile(string path)
    {
        if (m_gif != IntPtr.Zero)
        {
            FrameCapturer.fcGifWriteFile(m_gif, path);
        }
    }


    void OnEnable()
    {
        FrameCapturer.AddLibraryPath();

        m_rt = new RenderTexture(256, 256, 32, RenderTextureFormat.ARGB32);
        m_rt.Create();
        GetComponent<Camera>().targetTexture = m_rt;

        FrameCapturer.fcGifConfig conf;
        conf.width = m_rt.width;
        conf.height = m_rt.height;
        conf.delay_csec = m_delay_csec;
        conf.keyframe = m_keyframe;
        conf.max_frame = m_max_frame;
        conf.max_data_size = m_max_data_size;
        conf.max_active_tasks = m_max_active_tasks;
        m_gif = FrameCapturer.fcGifCreateContext(ref conf);
    }

    void OnDisable()
    {
        WriteFile("hoge.gif");

        FrameCapturer.fcGifDestroyContext(m_gif);
        if (m_rt == null) { return; }

    }

    void OnPostRender()
    {
        if (!m_capture_on_postrender) { return; }

        if (m_rt == null) { return; }
        FrameCapturer.fcGifAddFrame(m_gif, m_rt.GetNativeTexturePtr());
        ++m_frame;
    }

}
