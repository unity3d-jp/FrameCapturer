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
            Type_i16 = 0x4 << 4,
            Type_i32 = 0x5 << 4,

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
            Ri16     = Type_i16 | 1,
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

        [DllImport ("FrameCapturer")] public static extern void         fcGuardBegin();
        [DllImport ("FrameCapturer")] public static extern void         fcGuardEnd();
        [DllImport ("FrameCapturer")] public static extern void         fcEraseDeferredCall(int id);
        [DllImport ("FrameCapturer")] public static extern IntPtr       fcGetRenderEventFunc();

        public static void fcGuard(Action body)
        {
            fcGuardBegin();
            body.Invoke();
            fcGuardEnd();
        }

        public static fcPixelFormat fcGetPixelFormat(RenderTextureFormat v)
        {
            switch (v)
            {
                case RenderTextureFormat.ARGB32:    return fcPixelFormat.RGBAu8;
                case RenderTextureFormat.ARGBHalf:  return fcPixelFormat.RGBAf16;
                case RenderTextureFormat.RGHalf:    return fcPixelFormat.RGf16;
                case RenderTextureFormat.RHalf:     return fcPixelFormat.Rf16;
                case RenderTextureFormat.ARGBFloat: return fcPixelFormat.RGBAf32;
                case RenderTextureFormat.RGFloat:   return fcPixelFormat.RGf32;
                case RenderTextureFormat.RFloat:    return fcPixelFormat.Rf32;
                case RenderTextureFormat.ARGBInt:   return fcPixelFormat.RGBAi32;
                case RenderTextureFormat.RGInt:     return fcPixelFormat.RGi32;
                case RenderTextureFormat.RInt:      return fcPixelFormat.Ri32;
            }
            return fcPixelFormat.Unknown;
        }

        public static fcPixelFormat fcGetPixelFormat(TextureFormat v)
        {
            switch (v)
            {
                case TextureFormat.Alpha8:      return fcPixelFormat.Ru8;
                case TextureFormat.RGB24:       return fcPixelFormat.RGBu8;
                case TextureFormat.RGBA32:      return fcPixelFormat.RGBAu8;
                case TextureFormat.ARGB32:      return fcPixelFormat.RGBAu8;
                case TextureFormat.RGBAHalf:    return fcPixelFormat.RGBAf16;
                case TextureFormat.RGHalf:      return fcPixelFormat.RGf16;
                case TextureFormat.RHalf:       return fcPixelFormat.Rf16;
                case TextureFormat.RGBAFloat:   return fcPixelFormat.RGBAf32;
                case TextureFormat.RGFloat:     return fcPixelFormat.RGf32;
                case TextureFormat.RFloat:      return fcPixelFormat.Rf32;
            }
            return fcPixelFormat.Unknown;
        }

        public static int fcGetNumAudioChannels()
        {
            switch (AudioSettings.speakerMode)
            {
                case AudioSpeakerMode.Mono:         return 1;
                case AudioSpeakerMode.Stereo:       return 2;
                case AudioSpeakerMode.Quad:         return 4;
                case AudioSpeakerMode.Surround:     return 5;
                case AudioSpeakerMode.Mode5point1:  return 6;
                case AudioSpeakerMode.Mode7point1:  return 8;
                case AudioSpeakerMode.Prologic:     return 6;
            }
            return 0;
        }


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
                        max_active_tasks = 0,
                    };
                }
            }
        };
        public struct fcPNGContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcPNGContext fcPngCreateContext(ref fcPngConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcPngDestroyContext(fcPNGContext ctx);
        [DllImport ("FrameCapturer")] private static extern int         fcPngExportTextureDeferred(fcPNGContext ctx, string path, IntPtr tex, int width, int height, fcPixelFormat f, Bool flipY, int id);

        public static int fcPngExportTexture(fcPNGContext ctx, string path, RenderTexture tex, int pos)
        {
            return fcPngExportTextureDeferred(ctx, path,
                tex.GetNativeTexturePtr(), tex.width, tex.height, fcGetPixelFormat(tex.format), false, pos);
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
                        max_active_tasks = 0,
                    };
                }
            }
        };
        public struct fcEXRContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcEXRContext fcExrCreateContext(ref fcExrConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcExrDestroyContext(fcEXRContext ctx);
        [DllImport ("FrameCapturer")] private static extern int         fcExrBeginFrameDeferred(fcEXRContext ctx, string path, int width, int height, int id);
        [DllImport ("FrameCapturer")] private static extern int         fcExrAddLayerTextureDeferred(fcEXRContext ctx, IntPtr tex, fcPixelFormat f, int ch, string name, Bool flipY, int id);
        [DllImport ("FrameCapturer")] private static extern int         fcExrEndFrameDeferred(fcEXRContext ctx, int id);

        public static int fcExrBeginFrame(fcEXRContext ctx, string path, int width, int height, int id)
        {
            return fcExrBeginFrameDeferred(ctx, path, width, height, id);
        }

        public static int fcExrEndFrame(fcEXRContext ctx, int id)
        {
            return fcExrEndFrameDeferred(ctx, id);
        }

        public static int fcExrAddLayerTexture(fcEXRContext ctx, RenderTexture tex, int ch, string name, int id)
        {
            return fcExrAddLayerTextureDeferred(ctx, tex.GetNativeTexturePtr(), fcGetPixelFormat(tex.format), ch, name, false, id);
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
                        max_active_tasks = 0,
                    };
                }
            }
        };
        public struct fcGIFContext { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern fcGIFContext fcGifCreateContext(ref fcGifConfig conf);
        [DllImport ("FrameCapturer")] public static extern void         fcGifDestroyContext(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] private static extern int         fcGifAddFrameTextureDeferred(fcGIFContext ctx, IntPtr tex, fcPixelFormat fmt, Bool keyframe, double timestamp, int id);
        [DllImport ("FrameCapturer")] public static extern Bool         fcGifWrite(fcGIFContext ctx, fcStream stream, int begin_frame=0, int end_frame=-1);

        [DllImport ("FrameCapturer")] public static extern void         fcGifClearFrame(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern int          fcGifGetFrameCount(fcGIFContext ctx);
        [DllImport ("FrameCapturer")] public static extern void         fcGifGetFrameData(fcGIFContext ctx, IntPtr tex, int frame);
        [DllImport ("FrameCapturer")] public static extern int          fcGifGetExpectedDataSize(fcGIFContext ctx, int begin_frame, int end_frame);
        [DllImport ("FrameCapturer")] public static extern void         fcGifEraseFrame(fcGIFContext ctx, int begin_frame, int end_frame);

        public static int fcGifAddFrameTexture(fcGIFContext ctx, RenderTexture tex, bool keyframe, double timestamp, int id)
        {
            return fcGifAddFrameTextureDeferred(ctx, tex.GetNativeTexturePtr(), fcGetPixelFormat(tex.format), keyframe, timestamp, id);
        }

        public static Bool fcGifWriteFile(fcGIFContext ctx, string path, int begin_frame = 0, int end_frame = -1)
        {
            fcStream fstream = fcCreateFileStream(path);
            Bool ret = fcGifWrite(ctx, fstream, begin_frame, end_frame);
            fcDestroyStream(fstream);
            return ret;
        }


        // -------------------------------------------------------------
        // MP4 Exporter
        // -------------------------------------------------------------

        public enum fcMP4VideoFlags
        {
            H264NVIDIA  = 1 << 1,
            H264AMD     = 1 << 2,
            H264IntelHW = 1 << 3,
            H264IntelSW = 1 << 4,
            H264OpenH264= 1 << 5,
            H264Mask = H264NVIDIA | H264AMD | H264IntelHW | H264IntelSW | H264OpenH264,
        };

        public enum fcMP4AudioFlags
        {
            AACIntel = 1 << 1,
            AACFAAC  = 1 << 2,
            AACMask = AACIntel | AACFAAC,
        };

        public struct fcMP4Config
        {
            public Bool video;
            public Bool audio;

            public int video_width;
            public int video_height;
            public int video_target_bitrate;
            public int video_target_framerate;
            public int video_flags;

            public int audio_sample_rate;
            public int audio_num_channels;
            public int audio_target_bitrate;
            public int audio_flags;

            public static fcMP4Config default_value
            {
                get
                {
                    return new fcMP4Config
                    {
                        video = true,
                        audio = true,

                        video_width = 0,
                        video_height = 0,
                        video_target_bitrate = 1024 * 1000,
                        video_target_framerate = 30,
                        video_flags = (int)fcMP4VideoFlags.H264Mask,

                        audio_sample_rate = 48000,
                        audio_num_channels = 2,
                        audio_target_bitrate = 64000,
                        audio_flags = (int)fcMP4AudioFlags.AACMask,
                    };
                }
            }
        };
        public struct fcMP4Context { public IntPtr ptr; }

        [DllImport ("FrameCapturer")] public static extern void             fcMP4SetFAACPackagePath(string path);
        [DllImport ("FrameCapturer")] public static extern Bool             fcMP4DownloadCodecBegin();
        [DllImport ("FrameCapturer")] public static extern fcDownloadState  fcMP4DownloadCodecGetState();

        [DllImport ("FrameCapturer")] public static extern fcMP4Context     fcMP4CreateContext(ref fcMP4Config conf);
        [DllImport ("FrameCapturer")] public static extern void             fcMP4DestroyContext(fcMP4Context ctx);
        [DllImport ("FrameCapturer")] public static extern void             fcMP4AddOutputStream(fcMP4Context ctx, fcStream s);
        [DllImport ("FrameCapturer")] private static extern IntPtr          fcMP4GetAudioEncoderInfo(fcMP4Context ctx);
        [DllImport ("FrameCapturer")] private static extern IntPtr          fcMP4GetVideoEncoderInfo(fcMP4Context ctx);
        [DllImport ("FrameCapturer")] private static extern int             fcMP4AddVideoFrameTextureDeferred(fcMP4Context ctx, IntPtr tex, fcPixelFormat fmt, double time, int id);
        [DllImport ("FrameCapturer")] public static extern Bool             fcMP4AddAudioFrame(fcMP4Context ctx, float[] samples, int num_samples, double time = -1.0);

        public static string fcMP4GetAudioEncoderInfoS(fcMP4Context ctx)
        {
            return Marshal.PtrToStringAnsi(fcMP4GetAudioEncoderInfo(ctx));
        }

        public static string fcMP4GetVideoEncoderInfoS(fcMP4Context ctx)
        {
            return Marshal.PtrToStringAnsi(fcMP4GetVideoEncoderInfo(ctx));
        }

        public static int fcMP4AddVideoFrameTexture(fcMP4Context ctx, RenderTexture tex, double time, int id)
        {
            return fcMP4AddVideoFrameTextureDeferred(ctx, tex.GetNativeTexturePtr(), fcGetPixelFormat(tex.format), time, id);
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
        public static bool IsRenderingPathDeferred(Camera cam)
        {
            if (cam.renderingPath == RenderingPath.DeferredShading ||
                (cam.renderingPath == RenderingPath.UsePlayerSettings && PlayerSettings.renderingPath == RenderingPath.DeferredShading))
            {
                return true;
            }
            return false;
        }
#endif // UNITY_EDITOR

#if UNITY_EDITOR
        public static Shader GetFrameBufferCopyShader()
        {
            return AssetDatabase.LoadAssetAtPath<Shader>(AssetDatabase.GUIDToAssetPath("2283fb92223c7914c9096670e29202c8"));
        }
    #endif
    }
}
