using UnityEngine;

namespace UTJ
{
    [RequireComponent(typeof(TweetMedia))]
    public class TweetMediaAttachMovie : MonoBehaviour
    {
        public IMovieRecoerderUI m_recorderUI;
        public UnityEngine.UI.Toggle m_toggle_screenshot;
        TweetMedia m_tweet_media;


        void AttachFile(TweetMedia.TweetStateCode code)
        {
            if (code == TweetMedia.TweetStateCode.Begin)
            {
                if (!m_toggle_screenshot.isOn) { return; }
                m_toggle_screenshot.isOn = false;

                m_recorderUI.FlushFile();
                m_tweet_media.AddMediaFile(m_recorderUI.GetOutputPath());
            }
        }

        void Start()
        {
            if (m_recorderUI == null)
            {
                Debug.LogError("TweetMediaAttachMovie: m_recorder is null");
            }
            m_tweet_media = GetComponent<TweetMedia>();
            m_tweet_media.AddTweetEventHandler(AttachFile);
        }
    }
}
