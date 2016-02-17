using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

namespace UTJ
{
    [RequireComponent(typeof(TweetMedia))]
    public class TweetMediaAttachGIF : MonoBehaviour
    {
        public GifCapturerHUD m_capturer_hud;
        public UnityEngine.UI.Toggle m_toggle_screenshot;
        TweetMedia m_tweet_media;


        void AttachScreenshot(TweetMedia.TweetStateCode code)
        {
            if (code == TweetMedia.TweetStateCode.Begin)
            {
                if (!m_toggle_screenshot.isOn) { return; }
                m_toggle_screenshot.isOn = false;

                IGifCapturer capturer = m_capturer_hud.m_capturer;
                {
                    int begin = m_capturer_hud.begin_frame;
                    int end = m_capturer_hud.end_frame;
                    int expected_data_size = capturer.GetExpectedFileSize(begin, end);
                    IntPtr data = Marshal.AllocHGlobal(expected_data_size);
                    int actual_data_size = capturer.WriteMemory(data, begin, end);
                    m_tweet_media.AddMedia(data, actual_data_size, TweetMediaPlugin.tmEMediaType.GIF);
                    Marshal.FreeHGlobal(data);
                }
            }
        }

        void Start()
        {
            if (m_capturer_hud == null)
            {
                Debug.LogError("TweetMediaAttachGIF: m_capturer_hud is null");
            }
            m_tweet_media = GetComponent<TweetMedia>();
            m_tweet_media.AddTweetEventHandler(AttachScreenshot);
        }
    }
}
