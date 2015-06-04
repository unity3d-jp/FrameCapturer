using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


[AddComponentMenu("FrameCapturer/ExrCapturer")]
[RequireComponent(typeof(Camera))]
public class ExrCapturer : MonoBehaviour
{
    public bool m_capture_framebuffer;
    public bool m_capture_gbuffer;

    public string m_output_directory = "ExrOutput";
    public int m_begin_frame = 0;
    public int m_end_frame = 100;
    public int m_max_active_tasks = 1;
    public Shader m_sh_copy;

    IntPtr m_exr;
    int m_frame;
    Material m_mat_copy;
    Mesh m_quad;
    CommandBuffer m_cb;
    RenderTexture m_frame_buffer;
    RenderTexture[] m_gbuffer;
    RenderTexture m_depth;
    RenderBuffer[] m_rt_gbuffer;
    Camera m_cam;


#if UNITY_EDITOR
    void Reset()
    {
        m_sh_copy = AssetDatabase.LoadAssetAtPath("Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader", typeof(Shader)) as Shader;
    }
#endif // UNITY_EDITOR

    void OnEnable()
    {
        System.IO.Directory.CreateDirectory(m_output_directory);
        m_cam = GetComponent<Camera>();
        m_quad = FrameCapturerUtils.CreateFullscreenQuad();
        m_mat_copy = new Material(m_sh_copy);

        if (m_capture_framebuffer)
        {
            int tid = Shader.PropertyToID("_TmpFrameBuffer");
            m_cb = new CommandBuffer();
            m_cb.name = "ExrCapturer: copy frame buffer";
            m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
            m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
            // tid は意図的に開放しない
            m_cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);

            m_frame_buffer = new RenderTexture(m_cam.pixelWidth, m_cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
            m_frame_buffer.wrapMode = TextureWrapMode.Repeat;
            m_frame_buffer.Create();
        }

        if(m_capture_gbuffer)
        {
            m_gbuffer = new RenderTexture[4];
            m_rt_gbuffer = new RenderBuffer[4];
            for (int i = 0; i < m_gbuffer.Length; ++i)
            {
                m_gbuffer[i] = new RenderTexture(m_cam.pixelWidth, m_cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                m_gbuffer[i].filterMode = FilterMode.Point;
                m_gbuffer[i].Create();
                m_rt_gbuffer[i] = m_gbuffer[i].colorBuffer;
            }
            {
                m_depth = new RenderTexture(m_cam.pixelWidth, m_cam.pixelHeight, 0, RenderTextureFormat.RFloat);
                m_depth.filterMode = FilterMode.Point;
                m_depth.Create();
            }
        }

        FrameCapturer.fcExrConfig conf;
        conf.max_active_tasks = m_max_active_tasks;
        m_exr = FrameCapturer.fcExrCreateContext(ref conf);
    }

    void OnDisable()
    {
        if (m_cb != null)
        {
            m_cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            m_cb.Release();
            m_cb = null;
        }
        if (m_frame_buffer != null)
        {
            m_frame_buffer.Release();
            m_frame_buffer = null;
        }
        if (m_depth != null)
        {
            for (int i = 0; i < m_gbuffer.Length; ++i)
            {
                m_gbuffer[i].Release();
            }
            m_depth.Release();
            m_gbuffer = null;
            m_depth = null;
            m_rt_gbuffer = null;
        }

        FrameCapturer.fcExrDestroyContext(m_exr);
    }

    IEnumerator OnPostRender()
    {
        int frame = m_frame++;
        if (frame >= m_begin_frame && frame <= m_end_frame)
        {
            Debug.Log("ExrCapturer: frame " + frame);

            if(m_capture_gbuffer)
            {
                m_mat_copy.SetPass(2);
                Graphics.SetRenderTarget(m_rt_gbuffer, m_gbuffer[0].depthBuffer);
                Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);

                m_mat_copy.SetPass(3);
                Graphics.SetRenderTarget(m_depth);
                Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);

                string path = m_output_directory + "/gbuffer_" + frame.ToString("0000") + ".exr";
                FrameCapturer.fcExrBeginFrame(m_exr, path, m_gbuffer[0].width, m_gbuffer[0].height);
                AddLayer(m_gbuffer[0], 0, "Albedo.R");
                AddLayer(m_gbuffer[0], 1, "Albedo.G");
                AddLayer(m_gbuffer[0], 2, "Albedo.B");
                AddLayer(m_gbuffer[0], 3, "Occlusion");
                AddLayer(m_gbuffer[1], 0, "Specular.R");
                AddLayer(m_gbuffer[1], 1, "Specular.G");
                AddLayer(m_gbuffer[1], 2, "Specular.B");
                AddLayer(m_gbuffer[1], 3, "Smoothness");
                AddLayer(m_gbuffer[2], 0, "Normal.X");
                AddLayer(m_gbuffer[2], 1, "Normal.Y");
                AddLayer(m_gbuffer[2], 2, "Normal.Z");
                AddLayer(m_gbuffer[3], 0, "Emission.R");
                AddLayer(m_gbuffer[3], 1, "Emission.G");
                AddLayer(m_gbuffer[3], 2, "Emission.B");
                AddLayer(m_depth, 0, "Depth");
                FrameCapturer.fcExrEndFrame(m_exr);
            }

            yield return new WaitForEndOfFrame();
            if(m_capture_framebuffer)
            {
                m_mat_copy.SetPass(1);
                Graphics.SetRenderTarget(m_frame_buffer);
                Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                Graphics.SetRenderTarget(null);

                string path = m_output_directory + "/frame_" + frame.ToString("0000") + ".exr";
                FrameCapturer.fcExrBeginFrame(m_exr, path, m_frame_buffer.width, m_frame_buffer.height);
                AddLayer(m_frame_buffer, 0, "R");
                AddLayer(m_frame_buffer, 1, "G");
                AddLayer(m_frame_buffer, 2, "B");
                AddLayer(m_frame_buffer, 3, "A");
                FrameCapturer.fcExrEndFrame(m_exr);
            }
        }
    }

    void AddLayer(RenderTexture rt, int ch, string name)
    {
        FrameCapturer.fcExrAddLayer(m_exr, rt.GetNativeTexturePtr(), rt.format, ch, name);
    }
}
