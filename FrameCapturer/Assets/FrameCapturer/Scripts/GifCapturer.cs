using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


[AddComponentMenu("FrameCapturer/GifCapturer")]
[RequireComponent(typeof(Camera))]
public class GifCapturer : MonoBehaviour
{
    public int m_resolution_width = 320;
    public int m_num_colors = 255;
    public int m_capture_every_n_frames = 2;
    public int m_interval_centi_sec = 3;
    public int m_max_frame = 0;
    public int m_max_data_size = 0;
    public int m_max_active_tasks = 0;
    public int m_keyframe = 0;
    public Shader m_sh_copy;

    IntPtr m_gif;
    Material m_mat_copy;
    Mesh m_quad;
    CommandBuffer m_cb;
    RenderTexture m_rt_copy;
    Camera m_cam;
    int m_frame;
    bool m_pause = false;


    public bool pause
    {
        get { return m_pause; }
        set { m_pause = value; }
    }

    public void WriteFile(string path="")
    {
        if (m_gif != IntPtr.Zero)
        {
            if (path.Length==0)
            {
                path = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".gif";
            }
            FrameCapturer.fcGifWriteFile(m_gif, path);
            Debug.Log("GifCapturer.WriteFile() : " + path);
        }
    }


#if UNITY_EDITOR
    void Reset()
    {
        m_sh_copy = AssetDatabase.LoadAssetAtPath("Assets/FrameCapturer/Shaders/CopyFrameBuffer.shader", typeof(Shader)) as Shader;
    }

    void OnValidate()
    {
        m_num_colors = Mathf.Clamp(m_num_colors, 1, 255);
    }
#endif // UNITY_EDITOR

    void OnEnable()
    {
        m_cam = GetComponent<Camera>();
        m_quad = FrameCapturerUtils.CreateFullscreenQuad();
        m_mat_copy = new Material(m_sh_copy);

        int capture_width = m_resolution_width;
        int capture_height = (int)(m_resolution_width / ((float)m_cam.pixelWidth / (float)m_cam.pixelHeight));
        m_rt_copy = new RenderTexture(capture_width, capture_height, 0, RenderTextureFormat.ARGB32);
        m_rt_copy.Create();

        {
            m_cb = new CommandBuffer();
            m_cb.name = "GifCapturer: copy frame buffer";
            int tid = Shader.PropertyToID("_GifFrameBuffer");
            m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
            m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
            // tid は意図的に開放しない
            m_cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
        }
        {
            if(m_max_active_tasks<=0)
            {
                m_max_active_tasks = SystemInfo.processorCount;
            }
            FrameCapturer.fcGifConfig conf;
            conf.width = m_rt_copy.width;
            conf.height = m_rt_copy.height;
            conf.num_colors = m_num_colors;
            conf.delay_csec = m_interval_centi_sec;
            conf.keyframe = m_keyframe;
            conf.max_frame = m_max_frame;
            conf.max_data_size = m_max_data_size;
            conf.max_active_tasks = m_max_active_tasks;
            FrameCapturer.AddLibraryPath();
            m_gif = FrameCapturer.fcGifCreateContext(ref conf);
        }
    }

    void OnDisable()
    {
        WriteFile();
        FrameCapturer.fcGifDestroyContext(m_gif);

        m_cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
        m_cb.Release();
        m_cb = null;

        m_rt_copy.Release();
        m_rt_copy = null;
    }

    IEnumerator OnPostRender()
    {
        if (!m_pause)
        {
            yield return new WaitForEndOfFrame();

            int frame = m_frame++;
            if (frame % m_capture_every_n_frames == 0)
            {
                m_mat_copy.SetPass(0);
                Graphics.SetRenderTarget(m_rt_copy);
                Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                Graphics.SetRenderTarget(null);
                // 最初のフレームは大抵ゴミが入ってるので省略
                if (m_frame > 0)
                {
                    FrameCapturer.fcGifAddFrame(m_gif, m_rt_copy.GetNativeTexturePtr());
                }
            }
        }
    }
}
