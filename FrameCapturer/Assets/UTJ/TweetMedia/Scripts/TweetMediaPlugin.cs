using System;
using System.Runtime.InteropServices;
using UnityEngine;

namespace UTJ
{
    public static class TweetMediaPlugin
    {
        public struct tmContext
        {
            public IntPtr ptr;
            public void Clear() { ptr = IntPtr.Zero; }
        }

        public enum tmEStatusCode
        {
            Unknown,
            InProgress,
            Failed,
            Succeeded,
        };

        public enum tmEMediaType
        {
            Unknown,
            PNG,
            JPEG,
            GIF,
            WEBP,
            MP4,
        };

        public struct tmAuthState
        {
            public tmEStatusCode code;
            public IntPtr _error_message;   // 
            public IntPtr _auth_url;        // these fields should be private. but then compiler gives warning.
            public string error_message { get { return Marshal.PtrToStringAnsi(_error_message); } }
            public string auth_url { get { return Marshal.PtrToStringAnsi(_auth_url); } }
        };


        public struct tmTweetState
        {
            public tmEStatusCode code;
            public IntPtr _error_message;   // 
            public string error_message { get { return Marshal.PtrToStringAnsi(_error_message); } }
        };


        [DllImport ("TweetMedia")] public static extern tmContext       tmCreateContext();
        [DllImport ("TweetMedia")] public static extern void            tmDestroyContext(tmContext ctx);

        [DllImport ("TweetMedia")] public static extern bool            tmLoadCredentials(tmContext ctx, string path);
        [DllImport ("TweetMedia")] public static extern bool            tmSaveCredentials(tmContext ctx, string path);

        [DllImport ("TweetMedia")] public static extern tmAuthState     tmVerifyCredentials(tmContext ctx);
        [DllImport ("TweetMedia")] public static extern void            tmVerifyCredentialsAsync(tmContext ctx);
        [DllImport ("TweetMedia")] public static extern tmAuthState     tmGetVerifyCredentialsState(tmContext ctx);

        [DllImport ("TweetMedia")] public static extern tmAuthState     tmRequestAuthURL(tmContext ctx, string consumer_key, string consumer_secret);
        [DllImport ("TweetMedia")] public static extern void            tmRequestAuthURLAsync(tmContext ctx, string consumer_key, string consumer_secret);
        [DllImport ("TweetMedia")] public static extern tmAuthState     tmGetRequestAuthURLState(tmContext ctx);

        [DllImport ("TweetMedia")] public static extern tmAuthState     tmEnterPin(tmContext ctx, string pin);
        [DllImport ("TweetMedia")] public static extern void            tmEnterPinAsync(tmContext ctx, string pin);
        [DllImport ("TweetMedia")] public static extern tmAuthState     tmGetEnterPinState(tmContext ctx);

        [DllImport ("TweetMedia")] public static extern bool            tmAddMedia(tmContext ctx, IntPtr data, int data_size, tmEMediaType mtype);
        [DllImport ("TweetMedia")] public static extern bool            tmAddMediaFile(tmContext ctx, string path);
        [DllImport ("TweetMedia")] public static extern void            tmClearMedia(tmContext ctx);

        [DllImport ("TweetMedia")] public static extern int             tmTweet(tmContext ctx, string message);
        [DllImport ("TweetMedia")] public static extern int             tmTweetAsync(tmContext ctx, string message);
        [DllImport ("TweetMedia")] public static extern tmTweetState    tmGetTweetState(tmContext ctx, int thandle);
        [DllImport ("TweetMedia")] public static extern void            tmReleaseTweetCache(tmContext ctx, int thandle);
    }



    [Serializable]
    public class DataPath
    {
        public enum Root
        {
            CurrentDirectory,
            PersistentDataPath,
            StreamingAssetsPath,
            TemporaryCachePath,
            DataPath,
        }

        public Root m_root;
        public string m_leaf;

        public DataPath() {}
        public DataPath(Root root, string leaf)
        {
            m_root = root;
            m_leaf = leaf;
        }

        public string GetPath()
        {
            string ret = "";
            switch (m_root)
            {
                case Root.CurrentDirectory:
                    ret += ".";
                    break;
                case Root.PersistentDataPath:
                    ret += Application.persistentDataPath;
                    break;
                case Root.StreamingAssetsPath:
                    ret += Application.streamingAssetsPath;
                    break;
                case Root.TemporaryCachePath:
                    ret += Application.temporaryCachePath;
                    break;
                case Root.DataPath:
                    ret += Application.dataPath;
                    break;
            }
            if(m_leaf.Length > 0)
            {
                ret += "/";
                ret += m_leaf;
            }
            return ret;
        }

        public void CreateDirectory()
        {
            System.IO.Directory.CreateDirectory(GetPath());
        }
    }
}
