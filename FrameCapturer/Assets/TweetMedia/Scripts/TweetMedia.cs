using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

public class TweetMedia : MonoBehaviour
{
    public enum AuthStateCode
    {
        VerifyingCredentialsBegin,
        VerifyingCredentialsSucceeded,
        VerifyingCredentialsFailed,
        RequestAuthURLBegin,
        RequestAuthURLSucceeded,
        RequestAuthURLFailed,
        EnterPinBegin,
        EnterPinSucceeded,
        EnterPinFailed,
    }

    public enum TweetStateCode
    {
        Begin,
        Succeeded,
        Failed,
    }


    public string m_consumer_key;
    public string m_consumer_secret;
    public string m_path_to_savedata;

    TweetMediaPlugin.tmContext m_ctx;
    Action<AuthStateCode> m_on_change_auth_state;
    Action<TweetStateCode> m_on_change_tweet_state;
    List<Action> m_on_tweet_handlers = new List<Action>();
    string m_auth_url = "";
    string m_error_message = "";
    string m_pin = "";


    public void AddOnTweetHandler(Action act)
    {
        m_on_tweet_handlers.Add(act);
    }

    public void RemoveOnTweetHandler(Action act)
    {
        m_on_tweet_handlers.Remove(act);
    }


    public Action<AuthStateCode> on_change_auth_state
    {
        get { return m_on_change_auth_state; }
        set { m_on_change_auth_state = value; }
    }

    public Action<TweetStateCode> on_change_tweet_state
    {
        get { return m_on_change_tweet_state; }
        set { m_on_change_tweet_state = value; }
    }

    public string auth_url
    {
        get { return m_auth_url; }
    }

    public string error_message
    {
        get { return m_error_message; }
    }

    public string pin
    {
        set { m_pin = value; }
    }

    AuthStateCode auth_state
    {
        set
        {
            if (m_on_change_auth_state != null)
            {
                m_on_change_auth_state.Invoke(value);
            }

        }
    }

    TweetStateCode tweet_state
    {
        set
        {
            if (m_on_change_tweet_state != null)
            {
                m_on_change_tweet_state.Invoke(value);
            }

        }
    }


    public void BeginAuthorize()
    {
        if (m_ctx.ptr != IntPtr.Zero)
        {
            StartCoroutine(Authorize());
        }
    }

    IEnumerator Authorize()
    {
        TweetMediaPlugin.tmVerifyCredentialsAsync(m_ctx);

        bool authorized = false;

        // 保存された token が有効かチェック
        while (enabled)
        {
            var state = TweetMediaPlugin.tmGetVerifyCredentialsState(m_ctx);
            auth_state = AuthStateCode.VerifyingCredentialsBegin;
            if (state.code == TweetMediaPlugin.tmEStatusCode.InProgress)
            {
                yield return 0;
            }
            else
            {
                if (state.code == TweetMediaPlugin.tmEStatusCode.Succeeded)
                {
                    authorized = true;
                    auth_state = AuthStateCode.VerifyingCredentialsSucceeded;
                }
                else
                {
                    m_error_message = state.error_message;
                    auth_state = AuthStateCode.VerifyingCredentialsFailed;
                }
                break;
            }
        }

        // token が無効な場合認証処理開始
        while (enabled && !authorized)
        {
            // 認証 URL を取得
            TweetMediaPlugin.tmRequestAuthURLAsync(m_ctx, m_consumer_key, m_consumer_secret);
            auth_state = AuthStateCode.RequestAuthURLBegin;
            while (enabled)
            {
                var state = TweetMediaPlugin.tmGetRequestAuthURLState(m_ctx);
                if (state.code == TweetMediaPlugin.tmEStatusCode.InProgress)
                {
                    yield return 0;
                }
                else
                {
                    if (state.code == TweetMediaPlugin.tmEStatusCode.Succeeded)
                    {
                        m_auth_url = state.auth_url;
                        auth_state = AuthStateCode.RequestAuthURLSucceeded;
                    }
                    else
                    {
                        m_error_message = state.error_message;
                        auth_state = AuthStateCode.RequestAuthURLFailed;
                        // ここで失敗したらほとんど続けようがない (consumer key & secret が無効かネットワーク障害)
                        yield break;
                    }
                    break;
                }
            }

            // pin の入力を待って送信
            while (enabled && m_pin.Length == 0) { yield return 0; }

            m_error_message = "";
            TweetMediaPlugin.tmEnterPinAsync(m_ctx, m_pin);
            m_pin = "";
            auth_state = AuthStateCode.EnterPinBegin;
            while (enabled)
            {
                var state = TweetMediaPlugin.tmGetEnterPinState(m_ctx);
                if (state.code == TweetMediaPlugin.tmEStatusCode.InProgress)
                {
                    yield return 0;
                }
                else
                {
                    if (state.code == TweetMediaPlugin.tmEStatusCode.Succeeded)
                    {
                        authorized = true;
                        TweetMediaPlugin.tmSaveCredentials(m_ctx, m_path_to_savedata);
                        auth_state = AuthStateCode.EnterPinSucceeded;
                    }
                    else
                    {
                        m_error_message = state.error_message;
                        auth_state = AuthStateCode.EnterPinFailed;
                    }
                    break;
                }
            }

        }
    }




    public void AddMediaFile(string path)
    {
        TweetMediaPlugin.tmAddMediaFile(m_ctx, path);
    }

    public void AddMedia(IntPtr data, int datasize, TweetMediaPlugin.tmEMediaType mtype)
    {
        TweetMediaPlugin.tmAddMedia(m_ctx, data, datasize, mtype);
    }

    public void ClearMedia()
    {
        TweetMediaPlugin.tmClearMedia(m_ctx);
    }

    public void BeginTweet(string message)
    {
        m_on_tweet_handlers.ForEach((act) => { act.Invoke(); });
        StartCoroutine(Tweet(message));
    }

    IEnumerator Tweet(string message)
    {
        int handle = TweetMediaPlugin.tmTweetAsync(m_ctx, message);
        tweet_state = TweetStateCode.Begin;
        while(enabled) {
            TweetMediaPlugin.tmTweetState state = TweetMediaPlugin.tmGetTweetState(m_ctx, handle);
            if (state.code == TweetMediaPlugin.tmEStatusCode.InProgress)
            {
                yield return 0;
            }
            else
            {
                if (state.code == TweetMediaPlugin.tmEStatusCode.Succeeded)
                {
                    tweet_state = TweetStateCode.Succeeded;
                }
                else
                {
                    m_error_message = state.error_message;
                    tweet_state = TweetStateCode.Failed;
                }
                TweetMediaPlugin.tmReleaseTweetCache(m_ctx, handle);
                break;
            }
        }
    }



    void OnEnable()
    {
        if(m_consumer_key=="" || m_consumer_secret=="")
        {
            Debug.LogError("TweetMedia: set consumer_key and consumer_secret!");
        }
        m_ctx = TweetMediaPlugin.tmCreateContext();
        TweetMediaPlugin.tmLoadCredentials(m_ctx, m_path_to_savedata);
    }

    void OnDisable()
    {
        TweetMediaPlugin.tmDestroyContext(m_ctx);
        m_ctx.Clear();
    }
}
