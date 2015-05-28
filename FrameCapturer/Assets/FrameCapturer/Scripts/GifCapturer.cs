using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;


[RequireComponent(typeof(Camera))]
public class GifCapturer : MonoBehaviour
{
    [DllImport ("AddLibraryPath")] public static extern void    AddLibraryPath();
    [DllImport ("FrameCapturer")] public static extern IntPtr   fcGifCreateFile(string path, int width, int height);
    [DllImport ("FrameCapturer")] public static extern void     fcGifCloseFile(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifAddFrame(IntPtr ctx, IntPtr tex);


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
        m_gif = fcGifCreateFile("hoge.gif", m_rt.width, m_rt.height);
    }

    void OnDisable()
    {
        fcGifCloseFile(m_gif);
        if (m_rt == null) { return; }

    }

    void OnPostRender()
    {
        if (m_rt == null) { return; }
        fcGifAddFrame(m_gif, m_rt.GetNativeTexturePtr());
    }

}
