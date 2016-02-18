using System;
using System.Runtime.InteropServices;
using UnityEngine;
#if UNITY_EDITOR
using UnityEditor;
#endif

namespace UTJ
{
    public static class fcAPI
    {
        public enum fcPixelFormat
        {
            Unknown,
            RGBA8,
            RGB8,
            RG8,
            R8,
            RGBAHalf,
            RGBHalf,
            RGHalf,
            RHalf,
            RGBAFloat,
            RGBFloat,
            RGFloat,
            RFloat,
            RGBAInt,
            RGBInt,
            RGInt,
            RInt,
        };

        // -------------------------------------------------------------
        // Foundation
        // -------------------------------------------------------------

        [DllImport ("FrameCapturer")] public static extern void     fcSetModulePath(string path);
        [DllImport ("FrameCapturer")] public static extern ulong    fcSecondsToTimestamp(double sec);


        // -------------------------------------------------------------
        // EXR Exporter
        // -------------------------------------------------------------

        public struct fcExrConfig
        {
            public int max_active_tasks;
        };
        public struct fcEXRContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcEXRContext fcExrCreateContext(ref fcExrConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcExrDestroyContext(fcEXRContext ctx);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrBeginFrame(fcEXRContext ctx, string path, int width, int height);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrAddLayerTexture(fcEXRContext ctx, IntPtr tex, RenderTextureFormat f, int ch, string name, bool flipY);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrAddLayerPixels(fcEXRContext ctx, IntPtr pixels, fcPixelFormat f, int ch, string name, bool flipY);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrEndFrame(fcEXRContext ctx);


        // -------------------------------------------------------------
        // GIF Exporter
        // -------------------------------------------------------------

        public struct fcGifConfig
        {
            public int width;
            public int height;
            public int num_colors;
            public int delay_csec; // * centi second! *
            public int keyframe;
            public int max_active_tasks;
            public int max_frame;
            public int max_data_size;
        };
        public struct fcGIFContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcGIFContext fcGifCreateContext(ref fcGifConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcGifDestroyContext(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern void         fcGifAddFrame(fcGIFContext ctx, IntPtr tex);
        [DllImport ("FrameCapturer")] public static extern void         fcGifClearFrame(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern bool         fcGifWriteFile(fcGIFContext ctx, string path, int begin_frame=0, int end_frame=-1);
        [DllImport ("FrameCapturer")] public static extern int          fcGifWriteMemory(fcGIFContext ctx, IntPtr out_buf, int begin_frame=0, int end_frame=-1);
        [DllImport ("FrameCapturer")] public static extern int          fcGifGetFrameCount(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern void         fcGifGetFrameData(fcGIFContext ctx, IntPtr tex, int frame);
        [DllImport ("FrameCapturer")] public static extern int          fcGifGetExpectedDataSize(fcGIFContext ctx, int begin_frame, int end_frame);
        [DllImport ("FrameCapturer")] public static extern void         fcGifEraseFrame(fcGIFContext ctx, int begin_frame, int end_frame);


        // -------------------------------------------------------------
        // MP4 Exporter
        // -------------------------------------------------------------

        public enum fcColorSpace
        {
            RGBA,
            I420
        }
        public struct fcMP4Config
        {
            [MarshalAs(UnmanagedType.U1)] public bool video;
            [MarshalAs(UnmanagedType.U1)] public bool audio;

            [MarshalAs(UnmanagedType.U1)] public bool video_use_hardware_encoder_if_possible;
            public int video_width;
            public int video_height;
            public int video_bitrate;
            public int video_max_framerate;
            public int video_max_buffers;
            public float audio_scale;
            public int audio_sampling_rate;
            public int audio_num_channels;
            public int audio_bitrate;

            public static fcMP4Config default_value
            {
                get
                {
                    return new fcMP4Config
                    {
                        video = true,
                        audio = false,
                        video_use_hardware_encoder_if_possible = true,
                        video_width = 320,
                        video_height = 240,
                        video_bitrate = 256000,
                        video_max_framerate = 30,
                        video_max_buffers = 8,
                        audio_scale = 32767.0f,
                        audio_sampling_rate = 48000,
                        audio_num_channels = 2,
                        audio_bitrate = 64000,
                    };
                }
            }
        };
        public struct fcMP4Context { public IntPtr ptr; }
        public struct fcStream { public IntPtr ptr; }
        public delegate void fcDownloadCallback(bool is_complete, IntPtr message);

        [DllImport ("FrameCapturer")] public static extern ulong        fcGetTime();
        [DllImport ("FrameCapturer")] public static extern fcStream     fcCreateFileStream(string path);
        [DllImport ("FrameCapturer")] public static extern fcStream     fcCreateMemoryStream();
        [DllImport ("FrameCapturer")] public static extern void         fcDestroyStream(fcStream s);
        [DllImport ("FrameCapturer")] public static extern ulong        fcStreamGetWrittenSize(fcStream s);

        [DllImport ("FrameCapturer")] public static extern bool         fcMP4DownloadCodec(fcDownloadCallback cb);
        [DllImport ("FrameCapturer")] public static extern fcMP4Context fcMP4CreateContext(ref fcMP4Config conf);
        [DllImport ("FrameCapturer")] public static extern void         fcMP4DestroyContext(fcMP4Context ctx);
        [DllImport ("FrameCapturer")] public static extern void         fcMP4AddOutputStream(fcMP4Context ctx, fcStream s);
        [DllImport ("FrameCapturer")] public static extern bool         fcMP4AddVideoFrameTexture(fcMP4Context ctx, IntPtr tex, ulong time = ~0LU);
        [DllImport ("FrameCapturer")] public static extern bool         fcMP4AddVideoFramePixels(fcMP4Context ctx, IntPtr pixels, fcColorSpace cs, ulong time = ~0LU);
        [DllImport ("FrameCapturer")] public static extern bool         fcMP4AddAudioFrame(fcMP4Context ctx, float[] samples, int num_samples, ulong time = ~0LU);
    }


    public static class FrameCapturerUtils
    {
        public static Mesh CreateFullscreenQuad()
        {
            Vector3[] vertices = new Vector3[4] {
                    new Vector3( 1.0f, 1.0f, 0.0f),
                    new Vector3(-1.0f, 1.0f, 0.0f),
                    new Vector3(-1.0f,-1.0f, 0.0f),
                    new Vector3( 1.0f,-1.0f, 0.0f),
                };
            int[] indices = new int[6] { 0, 1, 2, 2, 3, 0 };

            Mesh r = new Mesh();
            r.vertices = vertices;
            r.triangles = indices;
            return r;
        }

    #if UNITY_EDITOR
        public static Shader GetFrameBufferCopyShader()
        {
            return AssetDatabase.LoadAssetAtPath<Shader>(AssetDatabase.GUIDToAssetPath("2283fb92223c7914c9096670e29202c8"));
        }
    #endif
    }
}
