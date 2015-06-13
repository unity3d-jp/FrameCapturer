using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

public static class FrameCapturer
{
    [DllImport ("AddLibraryPath")] public static extern void    AddLibraryPath();

    
    public struct fcExrConfig
    {
        public int max_active_tasks;
    };

    [DllImport ("FrameCapturer")] public static extern IntPtr   fcExrCreateContext(ref fcExrConfig conf);
    [DllImport ("FrameCapturer")] public static extern void     fcExrDestroyContext(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern bool     fcExrBeginFrame(IntPtr ctx, string path, int width, int height);
    [DllImport ("FrameCapturer")] public static extern bool     fcExrAddLayer(IntPtr ctx, IntPtr tex, RenderTextureFormat f, int ch, string name);
    [DllImport ("FrameCapturer")] public static extern bool     fcExrEndFrame(IntPtr ctx);


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

    [DllImport ("FrameCapturer")] public static extern IntPtr   fcGifCreateContext(ref fcGifConfig conf);
    [DllImport ("FrameCapturer")] public static extern void     fcGifDestroyContext(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifAddFrame(IntPtr ctx, IntPtr tex);
    [DllImport ("FrameCapturer")] public static extern void     fcGifClearFrame(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifWriteFile(IntPtr ctx, string path, int begin_frame=0, int end_frame=-1);
    [DllImport ("FrameCapturer")] public static extern void     fcGifWriteMemory(IntPtr ctx, IntPtr out_buf, int begin_frame=0, int end_frame=-1);
    [DllImport ("FrameCapturer")] public static extern int      fcGifGetFrameCount(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifGetFrameData(IntPtr ctx, IntPtr tex, int frame);
    [DllImport ("FrameCapturer")] public static extern int      fcGifGetExpectedDataSize(IntPtr ctx, int begin_frame, int end_frame);
    [DllImport ("FrameCapturer")] public static extern void     fcGifEraseFrame(IntPtr ctx, int begin_frame, int end_frame);


    public enum fcEColorSpace
    {
        RGBA,
        I420
    }
    public struct fcMP4Config
    {
        public int width;
        public int height;
        public int bitrate;
        public int framerate;
        public int max_active_tasks;
        public int max_frame;
        public int max_data_size;
    };
    public struct fcMP4Context
    {
        public IntPtr ptr;
    }
    [DllImport ("FrameCapturer")] public static extern fcMP4Context fcMP4CreateContext(ref fcMP4Config conf);
    [DllImport ("FrameCapturer")] public static extern void         fcMP4DestroyContext(fcMP4Context ctx);
    [DllImport ("FrameCapturer")] public static extern bool         fcMP4AddFrameTexture(fcMP4Context ctx, IntPtr tex);
    [DllImport ("FrameCapturer")] public static extern bool         fcMP4AddFramePixels(fcMP4Context ctx, IntPtr pixels, fcEColorSpace cs=fcEColorSpace.RGBA);
    [DllImport ("FrameCapturer")] public static extern void         fcMP4ClearFrame(fcMP4Context ctx);
    [DllImport ("FrameCapturer")] public static extern bool         fcMP4WriteFile(fcMP4Context ctx, string path, int begin_frame, int end_frame);
    [DllImport ("FrameCapturer")] public static extern int          fcMP4WriteMemory(fcMP4Context ctx, IntPtr dst, int begin_frame, int end_frame);
    [DllImport ("FrameCapturer")] public static extern int          fcMP4GetFrameCount(fcMP4Context ctx);
    [DllImport ("FrameCapturer")] public static extern void         fcMP4GetFrameData(fcMP4Context ctx, IntPtr tex, int frame);
    [DllImport ("FrameCapturer")] public static extern int          fcMP4GetExpectedDataSize(fcMP4Context ctx, int begin_frame, int end_frame);
    [DllImport ("FrameCapturer")] public static extern void         fcMP4EraseFrame(fcMP4Context ctx, int begin_frame, int end_frame);

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
}
