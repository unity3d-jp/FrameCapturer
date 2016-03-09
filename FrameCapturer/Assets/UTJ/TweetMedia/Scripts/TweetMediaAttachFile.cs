using UnityEngine;

namespace UTJ
{
    [RequireComponent(typeof(TweetMedia))]
    public class TweetMediaAttachFile : MonoBehaviour
    {
        public IMovieRecoerderUI m_recorderUI;
        public UnityEngine.UI.Toggle m_toggle_screenshot;
        TweetMedia m_tweet_media;


        void AttachFile(TweetMedia.TweetStateCode code)
        {
            if (code == TweetMedia.TweetStateCode.Begin)
            {
                if (!m_toggle_screenshot.isOn) { return; }
                if (m_recorderUI == null)
                {
                    Debug.LogError("TweetMediaAttachFile: m_recorderUI is null");
                    return;
                }
                m_toggle_screenshot.isOn = false;

                m_recorderUI.Flush();
                m_tweet_media.AddMediaFile(m_recorderUI.GetOutputPath());
            }
        }

        void Start()
        {
            m_tweet_media = GetComponent<TweetMedia>();
            m_tweet_media.AddTweetEventHandler(AttachFile);
        }
    }
}
