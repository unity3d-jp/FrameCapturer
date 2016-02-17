using System.Collections;
using UnityEngine;
using UnityEngine.UI;


namespace UTJ
{
    public class MP4CapturerHUD : MonoBehaviour
    {
        public IMP4Capturer m_capturer;
        public Text m_text_info;
        public RawImage m_mp4_preview;
        bool m_update_status;


        public bool record
        {
            get { return m_capturer.record; }
            set
            {
                m_capturer.record = value;
                m_update_status = true;
                if (value)
                {
                    GetComponent<Image>().color = new Color(1.0f, 0.5f, 0.5f, 0.5f);
                    UpdatePreviewImage(m_capturer.GetScratchBuffer());
                }
                else
                {
                    GetComponent<Image>().color = new Color(1.0f, 1.0f, 1.0f, 0.5f);
                }
            }
        }

        public void ResetRecordingState()
        {
            m_capturer.ResetRecordingState();
            if (record)
            {
                UpdatePreviewImage(m_capturer.GetScratchBuffer());
            }
            m_update_status = true;
        }


        void UpdatePreviewImage(RenderTexture rt)
        {
            const float MaxXScale = 1.8f;
            m_mp4_preview.texture = rt;
            float s = (float)rt.width / (float)rt.height;
            float xs = Mathf.Min(s, MaxXScale);
            float ys = MaxXScale / s;
            m_mp4_preview.rectTransform.localScale = new Vector3(xs, ys, 1.0f);
        }


        void OnEnable()
        {
        }

        void OnDisable()
        {
        }

        void Update()
        {
            if (m_update_status || record)
            {
                m_update_status = false;
                int recoded_frames = m_capturer.GetFrameCount();
                m_text_info.text = recoded_frames.ToString() + " recoded frames";
            }
        }
    }

}
