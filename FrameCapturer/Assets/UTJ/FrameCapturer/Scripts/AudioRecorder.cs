using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [AddComponentMenu("UTJ/FrameCapturer/Audio Recorder")]
    [RequireComponent(typeof(AudioListener))]
    public class AudioRecorder : MonoBehaviour
    {
        #region inner_types
        public enum CaptureControl
        {
            Manual,
            SpecifiedRange,
        }
        #endregion

        #region fields
        // base settings
        [SerializeField] DataPath m_outputDir = new DataPath(DataPath.Root.Current, "Capture");
        [SerializeField] AudioEncoderConfigs m_encoderConfigs = new AudioEncoderConfigs();

        // capture control
        [SerializeField] CaptureControl m_captureControl = CaptureControl.SpecifiedRange;
        [SerializeField] int m_startFrame = 0;
        [SerializeField] int m_endFrame = 100;

        // internal
        bool m_recording = false;
        AudioEncoder m_encoder;
        #endregion


        #region properties
        public DataPath outputDir
        {
            get { return m_outputDir; }
            set { m_outputDir = value; }
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

        public bool isRecording { get { return m_recording; } }
        #endregion


        public bool BeginRecording()
        {
            if (m_recording) { return false; }

            m_outputDir.CreateDirectory();

            // initialize encoder
            {
                string outPath = m_outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss");

                m_encoderConfigs.Setup();
                m_encoder = AudioEncoder.Create(m_encoderConfigs, outPath);
                if (!m_encoder)
                {
                    EndRecording();
                    return false;
                }
            }

            m_recording = true;
            Debug.Log("AudioMRecorder: BeginRecording()");
            return true;
        }

        public void EndRecording()
        {
            if(m_encoder != null)
            {
                m_encoder.Release();
                m_encoder = null;
            }
            m_recording = false;
            Debug.Log("AudioMRecorder: EndRecording()");
        }


        #region impl

#if UNITY_EDITOR
        void Reset()
        {
        }

        void OnValidate()
        {
            m_startFrame = Mathf.Max(0, m_startFrame);
            m_endFrame = Mathf.Max(m_startFrame, m_endFrame);
        }

#endif // UNITY_EDITOR

        void OnDisable()
        {
            EndRecording();
        }

        void Update()
        {
            int frame = Time.frameCount;
            if (m_captureControl == CaptureControl.SpecifiedRange)
            {
                if (frame >= m_startFrame && frame <= m_endFrame)
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
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_encoder != null)
            {
                m_encoder.AddAudioFrame(samples);
            }
        }
        #endregion
    }
}
