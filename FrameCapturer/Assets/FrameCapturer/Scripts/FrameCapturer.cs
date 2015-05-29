using System;
using System.Collections;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using UnityEngine;

public static class FrameCapturer
{
    public struct fcExrConfig
    {
        public int max_active_tasks;
    };

    public struct fcGifConfig
    {
        public int width;
        public int height;
        public int delay_csec; // * centi second! *
        public int keyframe;
        public int max_active_tasks;
        public int max_frame;
        public int max_data_size;
    };

    [DllImport ("AddLibraryPath")] public static extern void    AddLibraryPath();

    [DllImport ("FrameCapturer")] public static extern IntPtr   fcExrCreateContext(ref fcExrConfig conf);
    [DllImport ("FrameCapturer")] public static extern void     fcExrDestroyContext(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcExrWriteFile(IntPtr ctx, IntPtr tex, int width, int height, RenderTextureFormat f, string path);

    [DllImport ("FrameCapturer")] public static extern IntPtr   fcGifCreateContext(ref fcGifConfig conf);
    [DllImport ("FrameCapturer")] public static extern void     fcGifDestroyContext(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifAddFrame(IntPtr ctx, IntPtr tex);
    [DllImport ("FrameCapturer")] public static extern void     fcGifClearFrame(IntPtr ctx);
    [DllImport ("FrameCapturer")] public static extern void     fcGifWriteFile(IntPtr ctx, string path);

}
