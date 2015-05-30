using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


[AddComponentMenu("FrameCapturer/ExrGBufferCapturer")]
[RequireComponent(typeof(Camera))]
public class ExrGBufferCapturer : MonoBehaviour
{
    public string m_output_folder;
    public bool m_capture_on_postrender = true;
    public int m_max_active_tasks = 10;

    public Shader m_sh_copy;

    IntPtr m_exr;
    int m_frame;
    Mesh m_quad;
    Material m_mat_copy;

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


#if UNITY_EDITOR
    void Reset()
    {
        m_sh_copy = AssetDatabase.LoadAssetAtPath("Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader", typeof(Shader)) as Shader;
    }
#endif // UNITY_EDITOR

    void OnEnable()
    {
        m_quad = FrameCapturerUtils.CreateFullscreenQuad();

        Camera cam = GetComponent<Camera>();
        if (cam.depthTextureMode == DepthTextureMode.None)
        {
            cam.depthTextureMode = DepthTextureMode.Depth;
        }

        m_mat_copy = new Material(m_sh_copy);

        m_gbuffer = new RenderTexture[4];
        m_rt_gbuffer = new RenderBuffer[5];
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
            m_rt_gbuffer[4] = m_depth.colorBuffer;
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

        int frame = m_frame++;

        m_mat_copy.SetPass(2);
        Graphics.SetRenderTarget(m_rt_gbuffer, m_gbuffer[0].depthBuffer);
        Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);

        string sframe = frame.ToString("0000");
        WriteFile( m_gbuffer[0], m_output_folder + "/diffuse_"    + sframe + ".exr", 0x7);
        WriteFile2(m_gbuffer[0], m_output_folder + "/occulusion_" + sframe + ".exr", 0x8);
        WriteFile( m_gbuffer[1], m_output_folder + "/specular_"   + sframe + ".exr", 0x7);
        WriteFile2(m_gbuffer[1], m_output_folder + "/smoothness_" + sframe + ".exr", 0x8);
        WriteFile( m_gbuffer[2], m_output_folder + "/normal_"     + sframe + ".exr", 0x7);
        WriteFile( m_gbuffer[3], m_output_folder + "/emission_"   + sframe + ".exr", 0x7);
        WriteFile(      m_depth, m_output_folder + "/depth_"      + sframe + ".exr");
    }
}
