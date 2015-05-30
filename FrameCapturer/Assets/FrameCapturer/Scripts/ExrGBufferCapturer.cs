using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;

[AddComponentMenu("FrameCapturer/ExrGBufferCapturer")]
[RequireComponent(typeof(Camera))]
public class ExrGBufferCapturer : MonoBehaviour
{
    public string m_output_folder;
    public bool m_capture_on_postrender = true;
    public int m_max_active_tasks = 10;

    public Shader m_copy_gbuffer;
    public Shader m_copy_depth;

    IntPtr m_exr;
    int m_frame;
    Mesh m_quad;
    Material m_mat_gbuffer;
    Material m_mat_depth;

    /// m_gbuffer[0] : rgb: diffuse   a: occulusion
    /// m_gbuffer[1] : rgb: specular  a: smoothness
    /// m_gbuffer[2] : rgb: normal
    /// m_gbuffer[3] : rgb: emission
    public RenderTexture[] m_gbuffer;
    public RenderTexture m_depth;
    RenderBuffer[] m_rt_gbuffer;


    public void WriteFile(RenderTexture rt, string path, int mask = 0xF)
    {
        if (m_exr != IntPtr.Zero)
        {
            IntPtr tex = rt.GetNativeTexturePtr();
            FrameCapturer.fcExrWriteFile(m_exr, path, tex, rt.width, rt.height, rt.format, mask);
        }
    }

    public void WriteFile2(RenderTexture rt, string path, int mask = 0xF)
    {
        if (m_exr != IntPtr.Zero)
        {
            FrameCapturer.fcExrWriteFile(m_exr, path, IntPtr.Zero, rt.width, rt.height, rt.format, mask);
        }
    }

    void OnEnable()
    {
        m_quad = FrameCapturerUtils.CreateFullscreenQuad();

        Camera cam = GetComponent<Camera>();
        if (cam.depthTextureMode == DepthTextureMode.None)
        {
            cam.depthTextureMode = DepthTextureMode.Depth;
        }

        m_mat_gbuffer = new Material(m_copy_gbuffer);
        m_mat_depth = new Material(m_copy_depth);

        m_gbuffer = new RenderTexture[4];
        m_rt_gbuffer = new RenderBuffer[4];
        for (int i = 0; i < m_gbuffer.Length; ++i )
        {
            m_gbuffer[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
            m_gbuffer[i].filterMode = FilterMode.Point;
            m_gbuffer[i].Create();
            m_rt_gbuffer[i] = m_gbuffer[i].colorBuffer;
        }
        {
            m_depth = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.RFloat);
            m_depth.filterMode = FilterMode.Point;
            m_depth.Create();
        }

        FrameCapturer.AddLibraryPath();

        FrameCapturer.fcExrConfig conf;
        conf.max_active_tasks = m_max_active_tasks;
        m_exr = FrameCapturer.fcExrCreateContext(ref conf);
    }

    void OnDisable()
    {
        FrameCapturer.fcExrDestroyContext(m_exr);

        for (int i = 0; i < m_gbuffer.Length; ++i) { m_gbuffer[i].Release(); }
        m_gbuffer = null;
    }

    void OnPostRender()
    {
        if (!m_capture_on_postrender) { return; }

        m_mat_gbuffer.SetPass(0);
        Graphics.SetRenderTarget(m_rt_gbuffer, m_gbuffer[0].depthBuffer);
        Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);

        m_mat_depth.SetPass(0);
        Graphics.SetRenderTarget(m_depth);
        Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
        Graphics.SetRenderTarget(null);

        WriteFile( m_gbuffer[0], m_output_folder + "/diffuse_"    + m_frame.ToString("0000") + ".exr", 0x7);
        WriteFile2(m_gbuffer[0], m_output_folder + "/occulusion_" + m_frame.ToString("0000") + ".exr", 0x8);
        WriteFile( m_gbuffer[1], m_output_folder + "/specular_"   + m_frame.ToString("0000") + ".exr", 0x7);
        WriteFile2(m_gbuffer[1], m_output_folder + "/smoothness_" + m_frame.ToString("0000") + ".exr", 0x8);
        WriteFile( m_gbuffer[2], m_output_folder + "/normal_"     + m_frame.ToString("0000") + ".exr", 0x7);
        WriteFile( m_gbuffer[3], m_output_folder + "/emission_"   + m_frame.ToString("0000") + ".exr", 0x7);
        WriteFile(      m_depth, m_output_folder + "/depth_"      + m_frame.ToString("0000") + ".exr");

        ++m_frame;
    }
}
