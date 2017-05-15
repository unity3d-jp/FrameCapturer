using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;


namespace UTJ.FrameCapturer
{
    [AddComponentMenu("UTJ/FrameCapturer/Audio Recorder")]
    [RequireComponent(typeof(AudioListener))]
    [ExecuteInEditMode]
    public class AudioRecorder : RecorderBase
    {
        #region fields
        [SerializeField] AudioEncoderConfigs m_encoderConfigs = new AudioEncoderConfigs();
        AudioEncoder m_encoder;
        #endregion


        public override bool BeginRecording()
        {
            if (m_recording) { return false; }

            m_outputDir.CreateDirectory();

            // initialize encoder
            {
                string outPath = m_outputDir.GetFullPath() + "/" + DateTime.Now.ToString("yyyyMMdd_HHmmss");

                m_encoderConfigs.Setup();
                m_encoder = AudioEncoder.Create(m_encoderConfigs, outPath);
                if (m_encoder == null)
                {
                    EndRecording();
                    return false;
                }
            }

            m_initialTime = Time.unscaledTime;
            m_recordedFrames = 0;
            m_recordedSamples = 0;
            m_recording = true;

            Debug.Log("AudioMRecorder: BeginRecording()");
            return true;
        }

        public override void EndRecording()
        {
            if(m_encoder != null)
            {
                m_encoder.Release();
                m_encoder = null;
            }

            if(m_recording)
            {
                m_recording = false;
                m_aborted = true;
                Debug.Log("AudioMRecorder: EndRecording()");
            }
        }


        #region impl
        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_encoder != null)
            {
                m_encoder.AddAudioSamples(samples);
                m_recordedSamples += samples.Length;
            }
        }
        #endregion
    }
}
