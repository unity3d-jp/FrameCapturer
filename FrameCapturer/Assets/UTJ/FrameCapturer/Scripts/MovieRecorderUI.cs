using System;
using System.Collections;
using UnityEngine;
using UnityEngine.UI;


namespace UTJ
{
    public class MovieRecorderUI : IMovieRecoerderUI
    {
        public IMovieRecorder m_recorder;
        public Text m_textInfo;
        public RawImage m_imagePreview;
        bool m_updateStatus;


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
            return m_recorder.FlushFile();
        }

        public void ResetRecordingState()
        {
            m_recorder.ResetRecordingState();
            if (record)
            {
                UpdatePreviewImage(m_recorder.GetScratchBuffer());
            }
            m_updateStatus = true;
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



        void Update()
        {
            if (m_updateStatus || record)
            {
                m_updateStatus = false;
                int recoded_frames = m_recorder.GetFrameCount();
                m_textInfo.text = recoded_frames.ToString() + " recoded frames";
            }
        }
    }

}
