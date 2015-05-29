using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;


[AddComponentMenu("FrameCapturer/ExrCapturer")]
[RequireComponent(typeof(Camera))]
public class ExrCapturer : MonoBehaviour
{
    public RenderTexture[] m_render_targets;
    public RenderTextureFormat m_format;

    public string m_output_folder;
    public string m_output_filename;
    public bool m_capture_on_postrender = true;
    public int m_max_active_tasks = 5;
    IntPtr m_exr;
    int m_frame;


    public void WriteFile(RenderTexture rt, string path)
    {
        if (m_exr != IntPtr.Zero)
        {
            FrameCapturer.fcExrWriteFile(m_exr, path, rt.GetNativeTexturePtr(), rt.width, rt.height, rt.format);
        }
    }

    void OnEnable()
    {
        // for test
        if (m_render_targets == null || m_render_targets.Length==0)
        {
            m_render_targets = new RenderTexture[]
            {
                new RenderTexture(256, 256, 32, m_format)
            };
            m_render_targets[0].Create();
            GetComponent<Camera>().targetTexture = m_render_targets[0];
        }

        FrameCapturer.AddLibraryPath();
        FrameCapturer.fcExrConfig conf;
        conf.max_active_tasks = m_max_active_tasks;
        m_exr = FrameCapturer.fcExrCreateContext(ref conf);
    }

    void OnDisable()
    {
        FrameCapturer.fcExrDestroyContext(m_exr);
    }

    void OnPostRender()
    {
        if (m_capture_on_postrender && m_render_targets != null)
        {
            for (int i = 0; i < m_render_targets.Length; ++i )
            {
                string path = m_output_folder + "/" + m_output_filename + i.ToString() + "_" + m_frame.ToString("0000") + ".exr";
                WriteFile(m_render_targets[i], path);
            }
        }
        ++m_frame;
    }
}
