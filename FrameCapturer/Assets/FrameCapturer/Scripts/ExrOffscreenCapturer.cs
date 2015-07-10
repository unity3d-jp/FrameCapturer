using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif // UNITY_EDITOR


[AddComponentMenu("FrameCapturer/ExrOffscreenCapturer")]
[RequireComponent(typeof(Camera))]
public class ExrOffscreenCapturer : MonoBehaviour
{
    [System.Serializable]
    public class ChannelData
    {
        public string name;
        public int channel;
    }

    [System.Serializable]
    public class CaptureData
    {
        public RenderTexture target;
        public ChannelData[] channels;
    }

    public CaptureData[] m_targets;

    public string m_output_directory = "ExrOutput";
    public string m_output_filename = "Offscreen";
    public int m_begin_frame = 0;
    public int m_end_frame = 100;
    public int m_max_active_tasks = 1;
    public Shader m_sh_copy;

    IntPtr m_exr;
    int m_frame;
    Material m_mat_copy;
    Mesh m_quad;
    RenderTexture[] m_scratch_buffers;


#if UNITY_EDITOR
    void Reset()
    {
        m_sh_copy = FrameCapturerUtils.GetFrameBufferCopyShader();
    }
#endif // UNITY_EDITOR

    void OnEnable()
    {
        System.IO.Directory.CreateDirectory(m_output_directory);
        m_quad = FrameCapturerUtils.CreateFullscreenQuad();
        m_mat_copy = new Material(m_sh_copy);

        m_scratch_buffers = new RenderTexture[m_targets.Length];
        for (int i = 0; i < m_scratch_buffers.Length; ++i )
        {
            var rt = m_targets[i].target;
            m_scratch_buffers[i] = new RenderTexture(rt.width, rt.height, 0, rt.format);
        }

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

            var rt = m_targets[0].target;
            string path = m_output_directory + "/" + m_output_filename + "_" + frame.ToString("0000") + ".exr";

            // 上下反転などを行うため、一度スクラッチバッファに内容を移す
            for (int ti = 0; ti < m_targets.Length; ++ti)
            {
                var target = m_targets[ti];
                var scratch = m_scratch_buffers[ti];
                m_mat_copy.SetTexture("_TmpRenderTarget", target.target);
                m_mat_copy.SetPass(3);
                Graphics.SetRenderTarget(scratch);
                Graphics.DrawMeshNow(m_quad, Matrix4x4.identity);
                Graphics.SetRenderTarget(null);
            }

            // 描画結果を CPU 側に移してファイル書き出し
            FrameCapturer.fcExrBeginFrame(m_exr, path, rt.width, rt.height);
            for (int ti = 0; ti < m_targets.Length; ++ti)
            {
                var target = m_targets[ti];
                var scratch = m_scratch_buffers[ti];
                for (int ci = 0; ci < target.channels.Length; ++ci)
                {
                    var ch = target.channels[ci];
                    AddLayer(scratch, ch.channel, ch.name);
                }
            }
            FrameCapturer.fcExrEndFrame(m_exr);
        }
    }
    void AddLayer(RenderTexture rt, int ch, string name)
    {
        FrameCapturer.fcExrAddLayer(m_exr, rt.GetNativeTexturePtr(), rt.format, ch, name, false);
    }
}
