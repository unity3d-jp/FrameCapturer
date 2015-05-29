using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

[RequireComponent(typeof(Camera))]
public class ExrGBufferCapturer : MonoBehaviour
{

    public RenderTexture m_rt;
    public RenderTextureFormat m_format;

    public string m_output_folder;
    public bool m_capture_on_postrender = true;
    public int m_max_active_tasks = 5;
    IntPtr m_exr;
    int m_frame;


    public void WriteFile(RenderTexture rt, string path)
    {
        if (m_exr != IntPtr.Zero)
        {
            FrameCapturer.fcExrWriteFile(m_exr, rt.GetNativeTexturePtr(), rt.width, rt.height, rt.format, path);
        }
    }

    void OnEnable()
    {
        FrameCapturer.AddLibraryPath();

        m_rt = new RenderTexture(256, 256, 32, m_format);
        m_rt.Create();
        GetComponent<Camera>().targetTexture = m_rt;

        FrameCapturer.fcExrConfig conf;
        conf.max_active_tasks = m_max_active_tasks;
        m_exr = FrameCapturer.fcExrCreateContext(ref conf);
    }

    void OnDisable()
    {
        FrameCapturer.fcExrDestroyContext(m_exr);
        if (m_rt == null) { return; }

    }

    void OnPostRender()
    {
        if (!m_capture_on_postrender) { return; }

        if (m_rt == null) { return; }
        WriteFile(m_rt, m_output_folder + "/frame_"+m_frame+".exr");

        ++m_frame;
    }
}
