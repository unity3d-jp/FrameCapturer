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
            get { return m_recorder.recording; }
            set
            {
                m_updateStatus = true;
                if (value)
                {
                    m_recorder.BeginRecording();
                    m_recorder.recording = true;
                    GetComponent<Image>().color = new Color(1.0f, 0.5f, 0.5f, 0.5f);
                    UpdatePreviewImage(m_recorder.GetScratchBuffer());
                }
                else
                {
                    m_recorder.EndRecording();
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

        public override void Flush()
        {
            m_recorder.Flush();
        }

        public override void Restart()
        {
        }


        void UpdatePreviewImage(RenderTexture rt)
        {
            if(rt == null) { return; }

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
