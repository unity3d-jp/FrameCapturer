using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;

namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/MP4OffscreenRecorder")]
    [RequireComponent(typeof(Camera))]
    public class MP4OffscreenRecorder : MovieRecorderBase
    {
        public RenderTexture m_target;

        fcAPI.fcMP4Context m_ctx;
        fcAPI.fcMP4Config m_mp4conf = fcAPI.fcMP4Config.default_value;
        fcAPI.fcStream m_ostream;
        int m_callback;
        int m_numVideoFrames;


        void InitializeContext()
        {
            m_numVideoFrames = 0;

            // initialize scratch buffer
            var cam = GetComponent<Camera>();
            UpdateScratchBuffer(cam.pixelWidth, cam.pixelHeight);

            // initialize context and stream
            {
                m_mp4conf = fcAPI.fcMP4Config.default_value;
                m_mp4conf.video = m_captureVideo;
                m_mp4conf.audio = m_captureAudio;
                m_mp4conf.video_width = m_scratchBuffer.width;
                m_mp4conf.video_height = m_scratchBuffer.height;
                m_mp4conf.video_target_framerate = 60;
                m_mp4conf.video_target_bitrate = m_videoBitrate;
                m_mp4conf.audio_target_bitrate = m_audioBitrate;
                m_mp4conf.audio_sample_rate = AudioSettings.outputSampleRate;
                m_mp4conf.audio_num_channels = fcAPI.fcGetNumAudioChannels();
                m_ctx = fcAPI.fcMP4OSCreateContext(ref m_mp4conf);

                m_outputPath = DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".mp4";
                m_ostream = fcAPI.fcCreateFileStream(outputPath);
                fcAPI.fcMP4AddOutputStream(m_ctx, m_ostream);
            }

            // initialize command buffer
            {
                m_cb = new CommandBuffer();
                m_cb.name = "MP4OffscreenRecorder: copy frame buffer";
                m_cb.SetRenderTarget(m_scratchBuffer);
                m_cb.SetGlobalTexture("_TmpRenderTarget", m_target);
                m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 3);
            }
        }

        void ReleaseContext()
        {
            if (m_cb != null)
            {
                m_cb.Release();
                m_cb = null;
            }

            // scratch buffer is kept

            fcAPI.fcGuard(() =>
            {
                fcAPI.fcEraseDeferredCall(m_callback);
                m_callback = 0;

                if (m_ctx.ptr != IntPtr.Zero)
                {
                    fcAPI.fcMP4DestroyContext(m_ctx);
                    m_ctx.ptr = IntPtr.Zero;
                }
                if (m_ostream.ptr != IntPtr.Zero)
                {
                    fcAPI.fcDestroyStream(m_ostream);
                    m_ostream.ptr = IntPtr.Zero;
                }
            });
        }

        public override bool BeginRecording()
        {
            if (m_recording) { return false; }
            m_recording = true;

            InitializeContext();
            GetComponent<Camera>().AddCommandBuffer(CameraEvent.AfterEverything, m_cb);
            Debug.Log("MP4OffscreenRecorder.BeginRecording(): " + outputPath); return true;
        }

        public override bool EndRecording()
        {
            if (!m_recording) { return false; }
            m_recording = false;

            GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            ReleaseContext();
            Debug.Log("MP4OffscreenRecorder.EndRecording(): " + outputPath);
            return true;
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_captureAudio)
            {
                if(channels != m_mp4conf.audio_num_channels) {
                    Debug.LogError("MP4OffscreenRecorder: audio channels mismatch!");
                    return;
                }

                fcAPI.fcMP4AddAudioFrame(m_ctx, samples, samples.Length);
            }
        }

        IEnumerator OnPostRender()
        {
            if (m_recording && m_captureVideo && Time.frameCount % m_captureEveryNthFrame == 0)
            {
                yield return new WaitForEndOfFrame();

                double timestamp = Time.unscaledTime;
                if (m_frameRateMode == FrameRateMode.Constant)
                {
                    timestamp = 1.0 / m_framerate * m_numVideoFrames;
                }

                m_callback = fcAPI.fcMP4AddVideoFrameTexture(m_ctx, m_scratchBuffer, timestamp, m_callback);
                GL.IssuePluginEvent(fcAPI.fcGetRenderEventFunc(), m_callback);
                m_numVideoFrames++;
            }
        }
    }

}
