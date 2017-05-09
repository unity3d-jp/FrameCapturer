using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{

    [AddComponentMenu("UTJ/FrameCapturer/GBuffer Recorder")]
    [RequireComponent(typeof(Camera))]
    public class GBufferRecorder : MonoBehaviour
    {
        #region inner_types
        public enum CaptureControl
        {
            Manual,
            SpecifiedRange,
        }

        [Serializable]
        public struct FrameBufferConponents
        {
            public bool frameBuffer;
            public bool GBuffer;
            public bool albedo;
            public bool occlusion;
            public bool specular;
            public bool smoothness;
            public bool normal;
            public bool emission;
            public bool depth;
            public bool velocity;

            public static FrameBufferConponents default_value
            {
                get
                {
                    var ret = new FrameBufferConponents
                    {
                        frameBuffer = true,
                        GBuffer = true,
                        albedo = true,
                        occlusion = true,
                        specular = true,
                        smoothness = true,
                        normal = true,
                        emission = true,
                        depth = true,
                        velocity = true,
                    };
                    return ret;
                }
            }
        }

        class BufferRecorder
        {
            RenderTexture m_rt;
            int m_channels;
            string m_name;
            MovieEncoder m_encoder;

            public BufferRecorder(RenderTexture rt, int ch, string name)
            {
                m_rt = rt;
                m_channels = ch;
                m_name = name;
            }

            public void Initialize()
            {

            }

            public void Release()
            {
                m_encoder.Release();
            }

            public void Update(double time)
            {
                fcAPI.fcLock(m_rt, (data, fmt) => {
                    m_encoder.AddVideoFrame(data, fmt, time);
                });
            }
        }

        #endregion


        #region fields
        [SerializeField] DataPath m_outputDir = new DataPath(DataPath.Root.Current, "Capture");
        [SerializeField] MovieEncoder.Type m_format = MovieEncoder.Type.Exr;
        [SerializeField] FrameBufferConponents m_fbComponents = FrameBufferConponents.default_value;
        [SerializeField] bool m_fixDeltaTime = true;
        [SerializeField] int m_targetFramerate = 30;
        [SerializeField] CaptureControl m_captureControl = CaptureControl.SpecifiedRange;
        [SerializeField] int m_startFrame = 0;
        [SerializeField] int m_endFrame = 100;

        [SerializeField] fcAPI.fcPngConfig m_pngConfig = fcAPI.fcPngConfig.default_value;
        [SerializeField] fcAPI.fcExrConfig m_exrConfig = fcAPI.fcExrConfig.default_value;

        [SerializeField] Shader m_shCopy;
        Material m_matCopy;
        Mesh m_quad;
        CommandBuffer m_cbCopyFB;
        CommandBuffer m_cbCopyGB;
        CommandBuffer m_cbClearGB;
        CommandBuffer m_cbCopyVelocity;
        RenderTexture m_rtFB;
        RenderTexture[] m_rtGB;
        int m_frame;
        bool m_recording;
        bool m_oneShot;

        List<BufferRecorder> m_recorders = new List<BufferRecorder>();
        #endregion


        #region properties
        public DataPath outputDir
        {
            get { return m_outputDir; }
            set { m_outputDir = value; }
        }
        public MovieEncoder.Type format
        {
            get { return m_format; }
            set { m_format = value; }
        }
        public FrameBufferConponents fbComponents
        {
            get { return m_fbComponents; }
            set { m_fbComponents = value; }
        }
        public bool fixDeltaTime
        {
            get { return m_fixDeltaTime; }
            set { m_fixDeltaTime = value; }
        }
        public int targetFramerate
        {
            get { return m_targetFramerate; }
            set { m_targetFramerate = value; }
        }

        public CaptureControl captureControl
        {
            get { return m_captureControl; }
            set { m_captureControl = value; }
        }
        public int startFrame
        {
            get { return m_startFrame; }
            set { m_startFrame = value; }
        }
        public int endFrame
        {
            get { return m_endFrame; }
            set { m_endFrame = value; }
        }

        public fcAPI.fcPngConfig pngConfig { get { return m_pngConfig; } }
        public fcAPI.fcExrConfig exrConfig { get { return m_exrConfig; } }

        public bool isRecording { get { return m_recording; } }
        public int frame { get { return m_frame; } }
        #endregion



        public bool BeginRecording()
        {
            if (m_recording) { return false; }
            if (m_shCopy == null)
            {
                Debug.LogError("ImageSequenceRecorder: copy shader is missing!");
                return false;
            }

            m_recording = true;

            m_outputDir.CreateDirectory();
            if (m_quad == null) m_quad = fcAPI.CreateFullscreenQuad();
            if (m_matCopy == null) m_matCopy = new Material(m_shCopy);

            var cam = GetComponent<Camera>();
            if (m_fbComponents.frameBuffer)
            {
                if (m_cbCopyFB == null)
                {
                    m_rtFB = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                    m_rtFB.wrapMode = TextureWrapMode.Repeat;
                    m_rtFB.Create();

                    int tid = Shader.PropertyToID("_TmpFrameBuffer");
                    m_cbCopyFB = new CommandBuffer();
                    m_cbCopyFB.name = "ImageSequenceRecorder: Copy FrameBuffer";
                    m_cbCopyFB.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Point);
                    m_cbCopyFB.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                    m_cbCopyFB.SetRenderTarget(m_rtFB);
                    m_cbCopyFB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                    m_cbCopyFB.ReleaseTemporaryRT(tid);
                }
                cam.AddCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);
            }
            if (m_fbComponents.GBuffer)
            {
                if (m_cbCopyGB == null)
                {
                    m_rtGB = new RenderTexture[8];
                    for (int i = 0; i < m_rtGB.Length; ++i)
                    {
                        m_rtGB[i] = new RenderTexture(cam.pixelWidth, cam.pixelHeight, 0, RenderTextureFormat.ARGBHalf);
                        m_rtGB[i].filterMode = FilterMode.Point;
                        m_rtGB[i].Create();
                    }

                    // clear gbuffer (Unity doesn't clear emission buffer - it is not needed usually)
                    m_cbClearGB = new CommandBuffer();
                    m_cbClearGB.name = "ImageSequenceRecorder: Cleanup GBuffer";
                    if (cam.allowHDR)
                    {
                        m_cbClearGB.SetRenderTarget(BuiltinRenderTextureType.CameraTarget);
                    }
                    else
                    {
                        m_cbClearGB.SetRenderTarget(BuiltinRenderTextureType.GBuffer3);
                    }
                    m_cbClearGB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 3);

                    // copy gbuffer
                    m_cbCopyGB = new CommandBuffer();
                    m_cbCopyGB.name = "ImageSequenceRecorder: Copy GBuffer";
                    m_cbCopyGB.SetRenderTarget(new RenderTargetIdentifier[] {
                            m_rtGB[0], m_rtGB[1], m_rtGB[2], m_rtGB[3], m_rtGB[4], m_rtGB[5], m_rtGB[6]
                        }, m_rtGB[0]);
                    m_cbCopyGB.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 2);
                }
                cam.AddCommandBuffer(CameraEvent.BeforeGBuffer, m_cbClearGB);
                cam.AddCommandBuffer(CameraEvent.BeforeLighting, m_cbCopyGB);

                if (m_fbComponents.velocity)
                {
                    if (m_cbCopyVelocity == null)
                    {
                        m_cbCopyVelocity = new CommandBuffer();
                        m_cbCopyVelocity.name = "ImageSequenceRecorder: Copy Velocity";
                        m_cbCopyVelocity.SetRenderTarget(m_rtGB[7]);
                        m_cbCopyVelocity.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 4);
                    }
                    cam.AddCommandBuffer(CameraEvent.BeforeImageEffectsOpaque, m_cbCopyVelocity);
                    cam.depthTextureMode = DepthTextureMode.Depth | DepthTextureMode.MotionVectors;
                }
            }

            if (m_fbComponents.frameBuffer) { m_recorders.Add(new BufferRecorder(m_rtFB, 4, "FrameBuffer")); }
            if (m_fbComponents.GBuffer)
            {
                if (m_fbComponents.albedo)      { m_recorders.Add(new BufferRecorder(m_rtGB[0], 3, "Albedo")); }
                if (m_fbComponents.occlusion)   { m_recorders.Add(new BufferRecorder(m_rtGB[1], 1, "Occlusion")); }
                if (m_fbComponents.specular)    { m_recorders.Add(new BufferRecorder(m_rtGB[2], 3, "Specular")); }
                if (m_fbComponents.smoothness)  { m_recorders.Add(new BufferRecorder(m_rtGB[3], 1, "Smoothness")); }
                if (m_fbComponents.normal)      { m_recorders.Add(new BufferRecorder(m_rtGB[4], 3, "Normal")); }
                if (m_fbComponents.emission)    { m_recorders.Add(new BufferRecorder(m_rtGB[5], 3, "Emission")); }
                if (m_fbComponents.depth)       { m_recorders.Add(new BufferRecorder(m_rtGB[6], 1, "Depth")); }
                if (m_fbComponents.velocity)    { m_recorders.Add(new BufferRecorder(m_rtGB[7], 2, "Velocity")); }
            }
            foreach(var rec in m_recorders) { rec.Initialize(); }

            return true;
        }

        public void EndRecording()
        {
            if (!m_recording) { return; }
            m_recording = false;

            var cam = GetComponent<Camera>();
            if (m_cbCopyFB != null)
            {
                cam.RemoveCommandBuffer(CameraEvent.AfterEverything, m_cbCopyFB);
            }
            if (m_cbClearGB != null)
            {
                cam.RemoveCommandBuffer(CameraEvent.BeforeGBuffer, m_cbCopyGB);
            }
            if (m_cbCopyGB != null)
            {
                cam.RemoveCommandBuffer(CameraEvent.BeforeLighting, m_cbCopyGB);
            }
            if (m_cbCopyVelocity != null)
            {
                cam.RemoveCommandBuffer(CameraEvent.BeforeImageEffectsOpaque, m_cbCopyVelocity);
            }

            foreach(var rec in m_recorders) { rec.Release(); }
            m_recorders.Clear();
        }

        public void OneShot()
        {
            m_oneShot = true;
        }

        #region impl
        public void Export()
        {
            double timestamp = 1.0 / m_targetFramerate * m_frame;
            foreach (var rec in m_recorders) { rec.Update(timestamp); }
        }


        IEnumerator Wait()
        {
            yield return new WaitForEndOfFrame();

            // wait until current dt reaches target dt
            float wt = Time.maximumDeltaTime;
            while (Time.realtimeSinceStartup - Time.unscaledTime < wt)
            {
                System.Threading.Thread.Sleep(1);
            }
        }



#if UNITY_EDITOR
        void Reset()
        {
            m_shCopy = fcAPI.GetFrameBufferCopyShader();
        }

        void OnValidate()
        {
            m_startFrame = Mathf.Max(0, m_startFrame);
            m_endFrame = Mathf.Max(m_startFrame, m_endFrame);
        }
#endif // UNITY_EDITOR

        void OnDisable()
        {
            if (m_cbCopyFB != null)
            {
                m_cbCopyFB.Release();
                m_cbCopyFB = null;
            }
            if (m_cbClearGB != null)
            {
                m_cbClearGB.Release();
                m_cbClearGB = null;
            }
            if (m_cbCopyGB != null)
            {
                m_cbCopyGB.Release();
                m_cbCopyGB = null;
            }
            if (m_cbCopyVelocity != null)
            {
                m_cbCopyVelocity.Release();
                m_cbCopyVelocity = null;
            }

            if (m_rtFB != null)
            {
                m_rtFB.Release();
                m_rtFB = null;
            }
            if (m_rtGB != null)
            {
                foreach (var rt in m_rtGB) { rt.Release(); }
                m_rtGB = null;
            }
        }

        void Update()
        {
            m_frame = Time.frameCount;

            if (m_captureControl == CaptureControl.SpecifiedRange)
            {
                if (m_frame >= m_startFrame && m_frame <= m_endFrame)
                {
                    if (!m_recording) { BeginRecording(); }
                }
                else if(m_recording)
                {
                    EndRecording();
                }
            }
            else if (m_captureControl == CaptureControl.Manual)
            {
                if(m_oneShot)
                {
                    if (!m_recording)
                    {
                        BeginRecording();
                    }
                    else
                    {
                        EndRecording();
                        m_oneShot = false;
                    }
                }
            }

            if (m_fixDeltaTime)
            {
                Time.maximumDeltaTime = (1.0f / m_targetFramerate);
                StartCoroutine(Wait());
            }
        }

        IEnumerator OnPostRender()
        {
            if (m_recording)
            {
                yield return new WaitForEndOfFrame();
                Export();
            }
        }
        #endregion
    }

}
