using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;
#if UNITY_EDITOR
using UnityEditor;
#endif


namespace UTJ.FrameCapturer
{
    [ExecuteInEditMode]
    public abstract class RecorderBase : MonoBehaviour
    {
        public enum ResolutionUnit
        {
            Percent,
            Pixels,
        }

        public enum FrameRateMode
        {
            Variable,
            Constant,
        }

        public enum CaptureControl
        {
            Manual,
            FrameRange,
            TimeRange,
        }


        [SerializeField] protected DataPath m_outputDir = new DataPath(DataPath.Root.Current, "Capture");

        [SerializeField] protected ResolutionUnit m_resolutionUnit = ResolutionUnit.Percent;
        [SerializeField] protected int m_resolutionPercent = 100;
        [SerializeField] protected int m_resolutionWidth = 1980;

        [SerializeField] protected FrameRateMode m_framerateMode = FrameRateMode.Constant;
        [SerializeField] protected int m_targetFramerate = 30;
        [SerializeField] protected bool m_fixDeltaTime = true;
        [SerializeField] protected int m_captureEveryNthFrame = 1;

        [SerializeField] protected CaptureControl m_captureControl = CaptureControl.FrameRange;
        [SerializeField] protected int m_startFrame = 0;
        [SerializeField] protected int m_endFrame = 100;
        [SerializeField] protected float m_startTime = 0.0f;
        [SerializeField] protected float m_endTime = 10.0f;

        protected bool m_recording = false;
        protected bool m_aborted = false;
        protected float m_initialTime = 0.0f;
        protected int m_frame = 0;
        protected int m_recordedFrames = 0;
        protected int m_recordedSamples = 0;
#if UNITY_EDITOR
        [SerializeField] bool m_recordOnStart = false;
#endif


        public DataPath outputDir
        {
            get { return m_outputDir; }
            set { m_outputDir = value; }
        }

        public ResolutionUnit resolutionUnit
        {
            get { return m_resolutionUnit; }
            set { m_resolutionUnit = value; }
        }
        public int resolutionPercent
        {
            get { return m_resolutionPercent; }
            set { m_resolutionPercent = value; }
        }
        public int resolutionWidth
        {
            get { return m_resolutionWidth; }
            set { m_resolutionWidth = value; }
        }

        public FrameRateMode framerateMode
        {
            get { return m_framerateMode; }
            set { m_framerateMode = value; }
        }
        public int targetFramerate
        {
            get { return m_targetFramerate; }
            set { m_targetFramerate = value; }
        }
        public bool fixDeltaTime
        {
            get { return m_fixDeltaTime; }
            set { m_fixDeltaTime = value; }
        }
        public int captureEveryNthFrame
        {
            get { return m_captureEveryNthFrame; }
            set { m_captureEveryNthFrame = value; }
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
        public float startTime
        {
            get { return m_startTime; }
            set { m_startTime = value; }
        }
        public float endTime
        {
            get { return m_endTime; }
            set { m_endTime = value; }
        }
        public bool isRecording
        {
            get { return m_recording; }
            set {
                if (value) { BeginRecording(); }
                else { EndRecording(); }
            }
        }
#if UNITY_EDITOR
        public bool recordOnStart { set { m_recordOnStart = value; } }
#endif



        public abstract bool BeginRecording();
        public abstract void EndRecording();


        protected void GetCaptureResolution(ref int w, ref int h)
        {
            if(m_resolutionUnit == ResolutionUnit.Percent)
            {
                float scale = m_resolutionPercent * 0.01f;
                w = (int)(w * scale);
                h = (int)(h * scale);
            }
            else
            {
                float aspect = (float)h / w;
                w = m_resolutionWidth;
                h = (int)(m_resolutionWidth * aspect);
            }
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
        protected virtual void OnValidate()
        {
            m_startFrame = Mathf.Max(0, m_startFrame);
            m_endFrame = Mathf.Max(m_startFrame, m_endFrame);
            m_startTime = Mathf.Max(0.0f, m_startTime);
            m_endTime = Mathf.Max(m_startTime, m_endTime);
        }
#endif // UNITY_EDITOR

        protected virtual void Start()
        {
#if UNITY_EDITOR
            if (EditorApplication.isPlaying && m_recordOnStart)
            {
                BeginRecording();
            }
            m_recordOnStart = false;
#endif
        }

        protected virtual void OnDisable()
        {
#if UNITY_EDITOR
            if (EditorApplication.isPlaying)
#endif
            {
                EndRecording();
            }
        }

        protected virtual void Update()
        {
#if UNITY_EDITOR
            if (EditorApplication.isPlaying)
#endif
            {
                if (m_captureControl == CaptureControl.FrameRange)
                {
                    if (!m_aborted && m_frame >= m_startFrame && m_frame <= m_endFrame)
                    {
                        if (!m_recording) { BeginRecording(); }
                    }
                    else if (m_recording)
                    {
                        EndRecording();
                    }
                }
                else if (m_captureControl == CaptureControl.TimeRange)
                {
                    float time = Time.unscaledTime - m_initialTime;
                    if (!m_aborted && time >= m_startTime && time <= m_endTime)
                    {
                        if (!m_recording) { BeginRecording(); }
                    }
                    else if (m_recording)
                    {
                        EndRecording();
                    }
                }
                else if (m_captureControl == CaptureControl.Manual)
                {
                }

                if (m_fixDeltaTime)
                {
                    Time.maximumDeltaTime = (1.0f / m_targetFramerate);
                    StartCoroutine(Wait());
                }
            }
        }

    }
}
