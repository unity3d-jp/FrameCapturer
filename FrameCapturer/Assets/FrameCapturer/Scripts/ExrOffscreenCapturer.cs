using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;


[AddComponentMenu("FrameCapturer/ExrOffscreenCapturer")]
[RequireComponent(typeof(Camera))]
public class ExrOffscreenCapturer : MonoBehaviour
{
    public RenderTexture[] m_render_targets;

    public string m_output_directory = "ExrOutput";
    public string m_output_filename;
    public int m_begin_frame = 0;
    public int m_end_frame = 100;
    public int m_max_active_tasks = 1;
    IntPtr m_exr;
    int m_frame;
    Camera m_cam;



    void OnEnable()
    {
        System.IO.Directory.CreateDirectory(m_output_directory);
        m_cam = GetComponent<Camera>();

        FrameCapturer.fcExrConfig conf;
        conf.max_active_tasks = m_max_active_tasks;
        m_exr = FrameCapturer.fcExrCreateContext(ref conf);
    }

    void OnDisable()
    {
        FrameCapturer.fcExrDestroyContext(m_exr);
    }

    IEnumerator OnPostRender()
    {
        int frame = m_frame++;
        if (frame >= m_begin_frame && frame <= m_end_frame)
        {
            yield return new WaitForEndOfFrame();
            Debug.Log("ExrOffscreenCapturer: frame " + frame);

            for (int i = 0; i < m_render_targets.Length; ++i )
            {
                var rt = m_render_targets[i];
                if (rt == null) { continue; }

                string path = m_output_directory + "/" + m_output_filename + i.ToString() + "_" + frame.ToString("0000") + ".exr";
                FrameCapturer.fcExrBeginFrame(m_exr, path, rt.width, rt.height);
                AddLayer(rt, 0, "R");
                AddLayer(rt, 1, "G");
                AddLayer(rt, 2, "B");
                AddLayer(rt, 3, "A");
                FrameCapturer.fcExrEndFrame(m_exr);
            }
        }
    }
    void AddLayer(RenderTexture rt, int ch, string name)
    {
        FrameCapturer.fcExrAddLayer(m_exr, rt.GetNativeTexturePtr(), rt.format, ch, name);
    }
}
