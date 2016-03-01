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
        // -------------------------------------------------------------
        // Foundation
        // -------------------------------------------------------------

        public enum fcPixelFormat
        {
            Unknown = 0,

            ChannelMask = 0xF,
            TypeMask = 0xF << 4,
            Type_f16 = 0x1 << 4,
            Type_f32 = 0x2 << 4,
            Type_u8  = 0x3 << 4,
            Type_i32 = 0x4 << 4,
            Type_i16 = 0x5 << 4,

            Rf16     = Type_f16 | 1,
            RGf16    = Type_f16 | 2,
            RGBf16   = Type_f16 | 3,
            RGBAf16  = Type_f16 | 4,
            Rf32     = Type_f32 | 1,
            RGf32    = Type_f32 | 2,
            RGBf32   = Type_f32 | 3,
            RGBAf32  = Type_f32 | 4,
            Ru8      = Type_u8  | 1,
            RGu8     = Type_u8  | 2,
            RGBu8    = Type_u8  | 3,
            RGBAu8   = Type_u8  | 4,
            Ri16     = Type_i16 | 1 ,
            RGi16    = Type_i16 | 2,
            RGBi16   = Type_i16 | 3,
            RGBAi16  = Type_i16 | 4,
            Ri32     = Type_i32 | 1,
            RGi32    = Type_i32 | 2,
            RGBi32   = Type_i32 | 3,
            RGBAi32  = Type_i32 | 4,
        };

        public enum fcDownloadState
        {
            Idle,
            Completed,
            Error,
            InProgress,
        };


        [DllImport ("FrameCapturer")] public static extern void         fcSetModulePath(string path);
        [DllImport ("FrameCapturer")] public static extern double       fcGetTime();

        public struct fcStream { public IntPtr ptr; }
        [DllImport ("FrameCapturer")] public static extern fcStream     fcCreateFileStream(string path);
        [DllImport ("FrameCapturer")] public static extern fcStream     fcCreateMemoryStream();
        [DllImport ("FrameCapturer")] public static extern void         fcDestroyStream(fcStream s);
        [DllImport ("FrameCapturer")] public static extern ulong        fcStreamGetWrittenSize(fcStream s);


        // -------------------------------------------------------------
        // PNG Exporter
        // -------------------------------------------------------------

        public struct fcPngConfig
        {
            public int max_active_tasks;

            public static fcPngConfig default_value
            {
                get
                {
                    return new fcPngConfig
                    {
                        max_active_tasks = 8,
                    };
                }
            }
        };
        public struct fcPNGContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcPNGContext fcPngCreateContext(ref fcPngConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcPngDestroyContext(fcPNGContext ctx);
        [DllImport ("FrameCapturer")] public static extern bool         fcPngExportTexture(fcPNGContext ctx, string path, IntPtr tex, int width, int height, RenderTextureFormat f, bool flipY);
        [DllImport ("FrameCapturer")] public static extern bool         fcPngExportPixels(fcPNGContext ctx, string path, IntPtr pixels, int width, int height, fcPixelFormat f, bool flipY);

        public static bool fcPngExportTexture(fcPNGContext ctx, string path, RenderTexture tex, bool flipY)
        {
            return fcPngExportTexture(ctx, path, tex.GetNativeTexturePtr(), tex.width, tex.height, tex.format, flipY);
        }


        // -------------------------------------------------------------
        // EXR Exporter
        // -------------------------------------------------------------

        public struct fcExrConfig
        {
            public int max_active_tasks;

            public static fcExrConfig default_value
            {
                get
                {
                    return new fcExrConfig
                    {
                        max_active_tasks = 8,
                    };
                }
            }
        };
        public struct fcEXRContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcEXRContext fcExrCreateContext(ref fcExrConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcExrDestroyContext(fcEXRContext ctx);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrBeginFrame(fcEXRContext ctx, string path, int width, int height);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrAddLayerTexture(fcEXRContext ctx, IntPtr tex, RenderTextureFormat f, int ch, string name, bool flipY);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrAddLayerPixels(fcEXRContext ctx, IntPtr pixels, fcPixelFormat f, int ch, string name, bool flipY);
        [DllImport ("FrameCapturer")] public static extern bool         fcExrEndFrame(fcEXRContext ctx);

        public static bool fcExrAddLayerTexture(fcEXRContext ctx, RenderTexture tex, int ch, string name, bool flipY)
        {
            return fcExrAddLayerTexture(ctx, tex.GetNativeTexturePtr(), tex.format, ch, name, flipY);
        }


        // -------------------------------------------------------------
        // GIF Exporter
        // -------------------------------------------------------------

        public struct fcGifConfig
        {
            public int width;
            public int height;
            public int num_colors;
            public int max_active_tasks;

            public static fcGifConfig default_value
            {
                get
                {
                    return new fcGifConfig
                    {
                        width = 320,
                        height = 240,
                        num_colors = 256,
                        max_active_tasks = 8,
                    };
                }
            }
        };
        public struct fcGIFContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcGIFContext fcGifCreateContext(ref fcGifConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcGifDestroyContext(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern bool         fcGifAddFrameTexture(fcGIFContext ctx, IntPtr tex, RenderTextureFormat fmt, bool keyframe = false, double timestamp = -1.0);
        [DllImport ("FrameCapturer")] public static extern bool         fcGifAddFramePixels(fcGIFContext ctx, IntPtr pixels, fcPixelFormat fmt, bool keyframe = false, double timestamp = -1.0);
        [DllImport ("FrameCapturer")] public static extern bool         fcGifWrite(fcGIFContext ctx, fcStream stream, int begin_frame=0, int end_frame=-1);

        [DllImport ("FrameCapturer")] public static extern void         fcGifClearFrame(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern int          fcGifGetFrameCount(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern void         fcGifGetFrameData(fcGIFContext ctx, IntPtr tex, int frame);
        [DllImport ("FrameCapturer")] public static extern int          fcGifGetExpectedDataSize(fcGIFContext ctx, int begin_frame, int end_frame);
        [DllImport ("FrameCapturer")] public static extern void         fcGifEraseFrame(fcGIFContext ctx, int begin_frame, int end_frame);

        public static bool fcGifAddFrameTexture(fcGIFContext ctx, RenderTexture tex, bool keyframe = false, double timestamp = -1.0)
        {
            return fcGifAddFrameTexture(ctx, tex.GetNativeTexturePtr(), tex.format, keyframe, timestamp);
        }

        public static bool fcGifWriteFile(fcGIFContext ctx, string path, int begin_frame = 0, int end_frame = -1)
        {
            fcStream fstream = fcCreateFileStream(path);
            bool ret = fcGifWrite(ctx, fstream, begin_frame, end_frame);
            fcDestroyStream(fstream);
            return ret;
        }


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

        [DllImport ("FrameCapturer")] public static extern bool             fcMP4DownloadCodecBegin();
        [DllImport ("FrameCapturer")] public static extern fcDownloadState  fcMP4DownloadCodecGetState();

        [DllImport ("FrameCapturer")] public static extern fcMP4Context     fcMP4CreateContext(ref fcMP4Config conf);
        [DllImport ("FrameCapturer")] public static extern void             fcMP4DestroyContext(fcMP4Context ctx);
        [DllImport ("FrameCapturer")] public static extern void             fcMP4AddOutputStream(fcMP4Context ctx, fcStream s);
        [DllImport ("FrameCapturer")] public static extern bool             fcMP4AddVideoFrameTexture(fcMP4Context ctx, IntPtr tex, double time = -1.0);
        [DllImport ("FrameCapturer")] public static extern bool             fcMP4AddVideoFramePixels(fcMP4Context ctx, IntPtr pixels, fcColorSpace cs, double time = -1.0);
        [DllImport ("FrameCapturer")] public static extern bool             fcMP4AddAudioFrame(fcMP4Context ctx, float[] samples, int num_samples, double time = -1.0);

        public static bool fcMP4AddVideoFrameTexture(fcMP4Context ctx, RenderTexture tex, double time = -1.0)
        {
            return fcMP4AddVideoFrameTexture(ctx, tex.GetNativeTexturePtr(), time);
        }
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
