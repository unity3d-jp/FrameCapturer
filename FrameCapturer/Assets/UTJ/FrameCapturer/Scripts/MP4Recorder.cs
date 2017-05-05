using System;
using System.Collections;
using UnityEngine;
using UnityEngine.Rendering;

namespace UTJ
{
    [AddComponentMenu("UTJ/FrameCapturer/MP4Recorder")]
    [RequireComponent(typeof(Camera))]
    public class MP4Recorder : MovieRecorderBase
    {
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
                m_ostream = fcAPI.fcCreateFileStream(m_outputPath);
                fcAPI.fcMP4AddOutputStream(m_ctx, m_ostream);
            }

            // initialize command buffer
            {
                int tid = Shader.PropertyToID("_TmpFrameBuffer");
                m_cb = new CommandBuffer();
                m_cb.name = "MP4Recorder: copy frame buffer";
                m_cb.GetTemporaryRT(tid, -1, -1, 0, FilterMode.Bilinear);
                m_cb.Blit(BuiltinRenderTextureType.CurrentActive, tid);
                m_cb.SetRenderTarget(m_scratchBuffer);
                m_cb.DrawMesh(m_quad, Matrix4x4.identity, m_matCopy, 0, 0);
                m_cb.ReleaseTemporaryRT(tid);
            }
        }

        void ReleaseContext()
        {
            if(m_cb != null)
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
            Debug.Log("MP4Recorder.BeginRecording(): " + outputPath);
            return true;
        }

        public override bool EndRecording()
        {
            if (!m_recording) { return false; }
            m_recording = false;

            GetComponent<Camera>().RemoveCommandBuffer(CameraEvent.AfterEverything, m_cb);
            ReleaseContext();
            Debug.Log("MP4Recorder.EndRecording(): " + outputPath);
            return true;
        }



        void OnEnable()
        {
#if UNITY_EDITOR
            if(m_captureAudio && m_frameRateMode == FrameRateMode.Constant)
            {
                Debug.LogWarning("MP4Recorder: capture audio with Constant frame rate mode will cause desync");
            }
#endif
            m_outputDir.CreateDirectory();
            m_quad = FrameCapturerUtils.CreateFullscreenQuad();
            m_matCopy = new Material(m_shCopy);

            if (GetComponent<Camera>().targetTexture != null)
            {
                m_matCopy.EnableKeyword("OFFSCREEN");
            }
        }

        void OnDisable()
        {
            EndRecording();
            ReleaseContext();
            ReleaseScratchBuffer();
        }

        void OnAudioFilterRead(float[] samples, int channels)
        {
            if (m_recording && m_captureAudio)
            {
                if(channels != m_mp4conf.audio_num_channels) {
                    Debug.LogError("MP4Recorder: audio channels mismatch!");
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
