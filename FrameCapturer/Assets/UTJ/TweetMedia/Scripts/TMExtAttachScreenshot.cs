using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;
using UTJ;

[RequireComponent(typeof(TweetMedia))]
public class TMExtAttachScreenshot : MonoBehaviour
{
    public GifCapturerHUD m_capturer_hud;
    public UnityEngine.UI.Toggle m_toggle_screenshot;
    TweetMedia m_tweet_media;


    public static TweetMediaPlugin.tmEMediaType GetMediaType(IGifCapturer capturer)
    {
        if (capturer.GetType()==typeof(GifCapturer))
        {
            return TweetMediaPlugin.tmEMediaType.GIF;
        }
        else if (capturer.GetType() == typeof(MP4Capturer))
        {
            return TweetMediaPlugin.tmEMediaType.MP4;
        }
        return TweetMediaPlugin.tmEMediaType.Unknown;
    }

    void AttachScreenshot(TweetMedia.TweetStateCode code)
    {
        if (code == TweetMedia.TweetStateCode.Begin)
        {
            if (!m_toggle_screenshot.isOn) { return; }
            m_toggle_screenshot.isOn = false;

            IGifCapturer capturer = m_capturer_hud.m_capturer;
            var mtype = GetMediaType(capturer);
            if (mtype != TweetMediaPlugin.tmEMediaType.Unknown)
            {
                int begin = m_capturer_hud.begin_frame;
                int end = m_capturer_hud.end_frame;
                int expected_data_size = capturer.GetExpectedFileSize(begin, end);
                IntPtr data = Marshal.AllocHGlobal(expected_data_size);
                int actual_data_size = capturer.WriteMemory(data, begin, end);
                m_tweet_media.AddMedia(data, actual_data_size, mtype);
                Marshal.FreeHGlobal(data);
            }
        }
    }

    void Start()
    {
        if (m_capturer_hud == null)
        {
            Debug.LogError("TMExtAttachScreenshot: m_capturer_hud is null");
        }
        m_tweet_media = GetComponent<TweetMedia>();
        m_tweet_media.AddTweetEventHandler(AttachScreenshot);
    }
}
