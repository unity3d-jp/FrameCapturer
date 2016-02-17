using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

namespace UTJ
{
    [RequireComponent(typeof(TweetMedia))]
    public class TweetMediaAttachMP4 : MonoBehaviour
    {
        public MP4CapturerHUD m_capturer_hud;
        public UnityEngine.UI.Toggle m_toggle_screenshot;
        TweetMedia m_tweet_media;


        void AttachScreenshot(TweetMedia.TweetStateCode code)
        {
            if (code == TweetMedia.TweetStateCode.Begin)
            {
                if (!m_toggle_screenshot.isOn) { return; }
                m_toggle_screenshot.isOn = false;

                IMP4Capturer capturer = m_capturer_hud.m_capturer;
                capturer.FlushMP4();
                m_tweet_media.AddMediaFile(capturer.GetOutputPath());
            }
        }

        void Start()
        {
            if (m_capturer_hud == null)
            {
                Debug.LogError("TweetMediaAttachMP4: m_capturer_hud is null");
            }
            m_tweet_media = GetComponent<TweetMedia>();
            m_tweet_media.AddTweetEventHandler(AttachScreenshot);
        }
    }
}
