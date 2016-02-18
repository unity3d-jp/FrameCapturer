using UnityEngine;
using UnityEngine.UI;


namespace UTJ
{

    public class MovieRecorderEditorUI : IMovieRecoerderUI
    {
        public IEditableMovieRecorder m_recorder;
        public Text m_textInfo;
        public RawImage m_imagePreview;
        public Slider m_timeSlider;
        public InputField m_inputCurrentFrame;
        public RenderTexture m_rt;
        int m_currentFrame = 0;
        int m_beginFrame = 0;
        int m_endFrame = -1;
        bool m_updateStatus;
        bool m_updatePreview;


        public override bool record
        {
            get { return m_recorder.record; }
            set
            {
                m_recorder.record = value;
                m_updateStatus = true;
                if (value)
                {
                    GetComponent<Image>().color = new Color(1.0f, 0.5f, 0.5f, 0.5f);
                    UpdatePreviewImage(m_recorder.GetScratchBuffer());
                }
                else
                {
                    GetComponent<Image>().color = new Color(1.0f, 1.0f, 1.0f, 0.5f);
                    m_endFrame = -1;
                }
            }
        }

        public override IMovieRecorder GetRecorder()
        {
            return m_recorder;
        }

        public override string GetOutputPath()
        {
            return m_recorder.GetOutputPath();
        }

        public override bool FlushFile()
        {
            return m_recorder.FlushFile(m_beginFrame, m_endFrame);
        }

        public override void ResetRecordingState()
        {
            m_recorder.ResetRecordingState();
            if (record)
            {
                UpdatePreviewImage(m_recorder.GetScratchBuffer());
            }
            m_updateStatus = true;
        }


        public int beginFrame
        {
            get { return m_beginFrame; }
            set
            {
                m_beginFrame = m_currentFrame;
                if (m_endFrame >= 0)
                {
                    m_endFrame = Mathf.Max(m_endFrame, m_beginFrame);
                }
                m_updateStatus = true;
            }
        }

        public int endFrame
        {
            get { return m_endFrame; }
            set
            {
                m_endFrame = m_currentFrame;
                m_beginFrame = Mathf.Min(m_endFrame, m_beginFrame);
                m_updateStatus = true;
            }
        }

        public int currentFrame
        {
            get { return m_currentFrame; }
            set
            {
                m_currentFrame = (int)value;
                m_updateStatus = true;
                m_updatePreview = true;
                m_inputCurrentFrame.text = value.ToString();
            }
        }

        public void EraseFrames()
        {
            if (m_endFrame >= 0)
            {
                m_recorder.EraseFrame(m_beginFrame, m_endFrame);
                m_beginFrame = 0;
                m_endFrame = -1;
                m_updateStatus = true;
            }
        }


        void UpdatePreviewImage(RenderTexture rt)
        {
            const float MaxXScale = 1.8f;
            m_imagePreview.texture = rt;
            float s = (float)rt.width / (float)rt.height;
            float xs = Mathf.Min(s, MaxXScale);
            float ys = MaxXScale / s;
            m_imagePreview.rectTransform.localScale = new Vector3(xs, ys, 1.0f);
        }



        void OnEnable()
        {
        }

        void OnDisable()
        {
            if (m_rt != null)
            {
                m_rt.Release();
                m_rt = null;
            }
        }

        void Update()
        {
            if (m_rt == null)
            {
                var gif = m_recorder.GetScratchBuffer();
                m_rt = new RenderTexture(gif.width, gif.height, 0, RenderTextureFormat.ARGB32);
                m_rt.wrapMode = TextureWrapMode.Repeat;
                m_rt.Create();
            }
            if (m_updatePreview)
            {
                m_updatePreview = false;
                m_recorder.GetFrameData(m_rt, m_currentFrame);
                UpdatePreviewImage(m_rt);
            }
            if (m_updateStatus || record)
            {
                m_updateStatus = false;
                int recoded_frames = m_recorder.GetFrameCount();
                m_timeSlider.maxValue = recoded_frames;
                int begin_frame = m_beginFrame;
                int end_frame = m_endFrame == -1 ? recoded_frames : m_endFrame;
                int frame_count = end_frame - begin_frame;
                m_textInfo.text =
                    recoded_frames.ToString() + " recoded frames\n" +
                    frame_count.ToString() + " output frames (" + begin_frame + " - " + end_frame + ")\n" +
                    "expected file size: " + m_recorder.GetExpectedFileSize(begin_frame, end_frame);
            }
        }
    }

}
