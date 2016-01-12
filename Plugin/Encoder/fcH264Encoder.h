#ifndef fcMP4Encoder_h
#define fcMP4Encoder_h

#define OpenH264Version "1.5.0"
#ifdef fcWindows
    #if defined(_M_AMD64)
        #define OpenH264URL "http://ciscobinary.openh264.org/openh264-" OpenH264Version "-win64msvc.dll.bz2"
        #define OpenH264DLL "openh264-" OpenH264Version "-win64msvc.dll"
    #elif defined(_M_IX86)
        #define OpenH264URL "http://ciscobinary.openh264.org/openh264-" OpenH264Version "-win32msvc.dll.bz2"
        #define OpenH264DLL "openh264-" OpenH264Version "-win32msvc.dll"
    #endif
#else 
    // Mac
    #define OpenH264URL "http://ciscobinary.openh264.org/libopenh264-" OpenH264Version "-osx64.dylib.bz2"
    #define OpenH264DLL "libopenh264-" OpenH264Version "-osx64.dylib"
#endif

class ISVCEncoder;

class fcH264Encoder
{
public:
    static bool loadModule();

    fcH264Encoder(int width, int height, float frame_rate, int target_bitrate);
    ~fcH264Encoder();
    operator bool() const;
    fcH264Frame encodeI420(const void *src_y, const void *src_u, const void *src_v);

private:
    ISVCEncoder *m_encoder;
    int m_width;
    int m_height;
};

#endif // fcMP4Encoder_h
