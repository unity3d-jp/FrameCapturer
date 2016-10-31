#include "pch.h"
#include "fcFoundation.h"
#include "../fcMP4File.h"
#include "fcMP4_WMF.h"

#include "GraphicsDevice/fcGraphicsDevice.h"
#include "Foundation/TaskQueue.h"
#include "Foundation/LazyInstance.h"

#include <Windows.h>
#include <codecapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <wrl/client.h>

#pragma comment(lib, "mfuuid")

using Microsoft::WRL::ComPtr;


class MFInitializer
{
public:
    MFInitializer();
    ~MFInitializer();
};

class fcMP4ContextWMF : public fcIMP4Context
{
public:
    using VideoBuffer = Buffer;
    using VideoBufferPtr = std::shared_ptr<VideoBuffer>;
    using VideoBufferQueue = ResourceQueue<VideoBufferPtr>;

    using AudioBuffer = RawVector<float>;
    using AudioBufferPtr = std::shared_ptr<AudioBuffer>;
    using AudioBufferQueue = ResourceQueue<AudioBufferPtr>;


    fcMP4ContextWMF(fcMP4Config &conf, fcIGraphicsDevice *dev, const char *path);
    ~fcMP4ContextWMF();

    void release() override;

    const char* getAudioEncoderInfo() override;
    const char* getVideoEncoderInfo() override;

    void addOutputStream(fcStream *s) override;

    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp);

    bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp) override;
    bool addAudioFrameImpl(const float *samples, int num_samples, fcTime timestamp);

    bool isValid() const { return m_mf_writer != nullptr; }

private:
    bool initializeSinkWriter(const char *path);

    fcMP4Config         m_conf;
    fcIGraphicsDevice   *m_gdev = nullptr;

    TaskQueue           m_video_tasks;
    VideoBufferQueue    m_video_buffers;
    Buffer              m_rgba_image;
    I420Image           m_i420_image;

    TaskQueue           m_audio_tasks;
    AudioBufferQueue    m_audio_buffers;

    ComPtr<IMFSinkWriter> m_mf_writer;
    DWORD               m_mf_video_index = 0;
    DWORD               m_mf_audio_index = 0;
};



static HMODULE g_MFPlat;
static HMODULE g_MFReadWrite;
static HRESULT(STDAPICALLTYPE *MFStartup_)(ULONG Version, DWORD dwFlags);
static HRESULT(STDAPICALLTYPE *MFShutdown_)();
static HRESULT(STDAPICALLTYPE *MFCreateMemoryBuffer_)(DWORD cbMaxLength, IMFMediaBuffer **ppBuffer);
static HRESULT(STDAPICALLTYPE *MFCreateSample_)(IMFSample **ppIMFSample);
static HRESULT(STDAPICALLTYPE *MFCreateAttributes_)(IMFAttributes** ppMFAttributes, UINT32 cInitialSize);
static HRESULT(STDAPICALLTYPE *MFCreateMediaType_)(IMFMediaType** ppMFType);
static HRESULT(STDAPICALLTYPE *MFCreateSinkWriterFromURL_)(LPCWSTR pwszOutputURL, IMFByteStream *pByteStream, IMFAttributes *pAttributes, IMFSinkWriter **ppSinkWriter);

static LazyInstance<MFInitializer> g_MFInitializer;


MFInitializer::MFInitializer()
{
    g_MFPlat = ::LoadLibraryA("MFPlat.DLL");
    g_MFReadWrite = ::LoadLibraryA("MFReadWrite.dll");
    if (g_MFPlat && g_MFReadWrite) {
        (void*&)MFStartup_              = ::GetProcAddress(g_MFPlat, "MFStartup");
        (void*&)MFShutdown_             = ::GetProcAddress(g_MFPlat, "MFShutdown");
        (void*&)MFCreateMemoryBuffer_   = ::GetProcAddress(g_MFPlat, "MFCreateMemoryBuffer");
        (void*&)MFCreateSample_         = ::GetProcAddress(g_MFPlat, "MFCreateSample");
        (void*&)MFCreateAttributes_     = ::GetProcAddress(g_MFPlat, "MFCreateAttributes");
        (void*&)MFCreateMediaType_      = ::GetProcAddress(g_MFPlat, "MFCreateMediaType");
        (void*&)MFCreateSinkWriterFromURL_ = ::GetProcAddress(g_MFReadWrite, "MFCreateSinkWriterFromURL");

        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        MFStartup_(MF_VERSION, MFSTARTUP_LITE);
    }
}

MFInitializer::~MFInitializer()
{
    if (g_MFPlat && g_MFReadWrite) {
        MFShutdown_();
        CoUninitialize();

        ::FreeLibrary(g_MFReadWrite);
        ::FreeLibrary(g_MFPlat);
        g_MFReadWrite = nullptr;
        g_MFPlat = nullptr;
    }
}


fcMP4ContextWMF::fcMP4ContextWMF(fcMP4Config &conf, fcIGraphicsDevice *dev, const char *path)
    : m_conf(conf)
    , m_gdev(dev)
{
    g_MFInitializer.get();
    initializeSinkWriter(path);
}

fcMP4ContextWMF::~fcMP4ContextWMF()
{
    if (m_conf.video) { m_video_tasks.stop(); }
    if (m_conf.audio) { m_audio_tasks.stop(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (m_mf_writer) {
        m_mf_writer->Finalize();
        m_mf_writer.Reset();
    }
}

void fcMP4ContextWMF::release()
{
    delete this;
}

const char* fcMP4ContextWMF::getAudioEncoderInfo()
{
    return nullptr;
}

const char* fcMP4ContextWMF::getVideoEncoderInfo()
{
    return nullptr;
}

void fcMP4ContextWMF::addOutputStream(fcStream *s)
{
    // do nothing
}



bool fcMP4ContextWMF::initializeSinkWriter(const char *path)
{
    if (!g_MFPlat || !g_MFReadWrite) { return false; }

    ComPtr<IMFSinkWriter> pSinkWriter;
    HRESULT hr;

    {
        ComPtr<IMFAttributes> pAttributes;
        MFCreateAttributes_(&pAttributes, 1);
        if (pAttributes) {
            hr = pAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);

            std::wstring wpath;
            wpath.resize(strlen(path)+1);
            std::mbstowcs(&wpath[0], path, wpath.size());
            hr = MFCreateSinkWriterFromURL_(wpath.data(), nullptr, pAttributes.Get(), &pSinkWriter);
        }
    }
    if (!pSinkWriter) { return false; }

    // Set the video output media type.
    if(m_conf.video) {
        ComPtr<IMFMediaType> pVideoOutMediaType;
        MFCreateMediaType_(&pVideoOutMediaType);
        if (pVideoOutMediaType) {
            pVideoOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pVideoOutMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
            pVideoOutMediaType->SetUINT32(MF_MT_AVG_BITRATE, m_conf.video_target_bitrate);
            pVideoOutMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            pVideoOutMediaType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);
            MFSetAttributeSize(pVideoOutMediaType.Get(), MF_MT_FRAME_SIZE, m_conf.video_width, m_conf.video_height);
            MFSetAttributeRatio(pVideoOutMediaType.Get(), MF_MT_FRAME_RATE, m_conf.video_target_framerate, 1);
            MFSetAttributeRatio(pVideoOutMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            pSinkWriter->AddStream(pVideoOutMediaType.Get(), &m_mf_video_index);
        }

        ComPtr<IMFMediaType> pVideoInputMediaType;
        MFCreateMediaType_(&pVideoInputMediaType);
        if (pVideoInputMediaType) {
            pVideoInputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pVideoInputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420);
            pVideoInputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            MFSetAttributeSize(pVideoInputMediaType.Get(), MF_MT_FRAME_SIZE, m_conf.video_width, m_conf.video_height);
            MFSetAttributeRatio(pVideoInputMediaType.Get(), MF_MT_FRAME_RATE, m_conf.video_target_framerate, 1);
            MFSetAttributeRatio(pVideoInputMediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            pSinkWriter->SetInputMediaType(m_mf_video_index, pVideoInputMediaType.Get(), nullptr);
        }
    }

    // Set the audio output media type.
    if (m_conf.audio) {
        ComPtr<IMFMediaType> pAudioOutMediaType;
        MFCreateMediaType_(&pAudioOutMediaType);
        if (pAudioOutMediaType) {
            pAudioOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            pAudioOutMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, m_conf.audio_sample_rate);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, m_conf.audio_num_channels);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, m_conf.audio_target_bitrate / 8);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);
            pAudioOutMediaType->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0x29);
            pSinkWriter->AddStream(pAudioOutMediaType.Get(), &m_mf_audio_index);
        }

        ComPtr<IMFMediaType> pAudioInMediaType;
        MFCreateMediaType_(&pAudioInMediaType);
        if (pAudioInMediaType) {
            pAudioInMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            pAudioInMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, m_conf.audio_sample_rate);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, m_conf.audio_num_channels);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, m_conf.audio_sample_rate * 4 * m_conf.audio_num_channels);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4 * m_conf.audio_num_channels);
            pSinkWriter->SetInputMediaType(m_mf_audio_index, pAudioInMediaType.Get(), nullptr);
        }
    }

    // Tell the sink writer to start accepting data.
    if (!SUCCEEDED(pSinkWriter->BeginWriting())) {
        pSinkWriter.Reset();
    }

    // Return the pointer to the caller.
    m_mf_writer = pSinkWriter;
    return m_mf_writer != nullptr;
}


static inline LONGLONG SecondsToTimestamp(fcTime t)
{
    return LONGLONG(t * double(10 * 1000 * 1000));
}

bool fcMP4ContextWMF::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!m_mf_writer || !tex || !m_gdev) { return false; }

    auto buf = m_video_buffers.pop();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    if (m_gdev->readTexture(buf->data(), buf->size(), tex, m_conf.video_width, m_conf.video_height, fmt)) {
        m_video_tasks.run([this, buf, fmt, timestamp]() {
            addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
            m_video_buffers.push(buf);
        });
    }
    else {
        m_video_buffers.push(buf);
        return false;
    }
    return true;
}

bool fcMP4ContextWMF::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!m_mf_writer || !pixels) { return false; }

    auto buf = m_video_buffers.pop();
    size_t psize = fcGetPixelSize(fmt);
    size_t size = m_conf.video_width * m_conf.video_height * psize;
    buf->resize(size);
    memcpy(buf->data(), pixels, size);

    m_video_tasks.run([this, buf, fmt, timestamp]() {
        addVideoFramePixelsImpl(buf->data(), fmt, timestamp);
        m_video_buffers.push(buf);
    });
    return true;
}

bool fcMP4ContextWMF::addVideoFramePixelsImpl(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    const LONGLONG start = SecondsToTimestamp(timestamp);
    const LONGLONG duration = SecondsToTimestamp(1.0 / m_conf.video_target_framerate);
    const DWORD size = roundup<2>(m_conf.video_width) * roundup<2>(m_conf.video_height);
    const DWORD buffer_size = size + (size >> 2) + (size >> 2);

    I420Data i420;

    // convert image to I420
    if (fmt == fcPixelFormat_I420) {
        // use input data directly
        int frame_size = m_conf.video_width * m_conf.video_height;
        i420.y = (void*)pixels;
        i420.u = (char*)i420.y + frame_size;
        i420.v = (char*)i420.u + (frame_size >> 2);
    }
    else if (fmt == fcPixelFormat_RGBAu8) {
        // RGBAu8 -> I420
        m_i420_image.resize(m_conf.video_width, m_conf.video_height);
        RGBA2I420(m_i420_image, pixels, m_conf.video_width, m_conf.video_height);
        i420 = m_i420_image.data();
    }
    else {
        // any format -> RGBAu8 -> I420
        m_rgba_image.resize(m_conf.video_width * m_conf.video_height * 4);
        fcConvertPixelFormat(m_rgba_image.data(), fcPixelFormat_RGBAu8, pixels, fmt, m_conf.video_width * m_conf.video_height);

        m_i420_image.resize(m_conf.video_width, m_conf.video_height);
        RGBA2I420(m_i420_image, m_rgba_image.data(), m_conf.video_width, m_conf.video_height);
        i420 = m_i420_image.data();
    }


    ComPtr<IMFMediaBuffer> pBuffer;
    MFCreateMemoryBuffer_(buffer_size, &pBuffer);
    if (pBuffer)
    {
        BYTE *pData = nullptr;
        pBuffer->Lock(&pData, nullptr, nullptr);
        memcpy(pData, i420.y, buffer_size);
        pBuffer->Unlock();
        pBuffer->SetCurrentLength(buffer_size);

        // Create a media sample and add the buffer to the sample.
        ComPtr<IMFSample> pSample;
        MFCreateSample_(&pSample);
        if (pSample)
        {
            pSample->AddBuffer(pBuffer.Get());
            pSample->SetSampleTime(start);
            pSample->SetSampleDuration(duration);
            m_mf_writer->WriteSample(m_mf_video_index, pSample.Get());
        }
    }
    return true;
}

bool fcMP4ContextWMF::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (!m_mf_writer || !samples) { return false; }

    auto buf = m_audio_buffers.pop();
    buf->assign(samples, num_samples);

    m_audio_tasks.run([this, buf, num_samples, timestamp]() {
        addAudioFrameImpl(buf->data(), num_samples, timestamp);
        m_audio_buffers.push(buf);
    });
    return true;
}

bool fcMP4ContextWMF::addAudioFrameImpl(const float *samples, int num_samples, fcTime timestamp)
{
    const LONGLONG start = SecondsToTimestamp(timestamp);
    const LONGLONG duration = SecondsToTimestamp(1.0 / m_conf.video_target_framerate);
    const DWORD cbBuffer = num_samples * 4;

    ComPtr<IMFMediaBuffer> pBuffer;
    MFCreateMemoryBuffer_(cbBuffer, &pBuffer);
    if (!pBuffer) { return false; }

    {
        BYTE *pData = nullptr;
        pBuffer->Lock(&pData, nullptr, nullptr);
        memcpy(pData, samples, cbBuffer);
        pBuffer->Unlock();
        pBuffer->SetCurrentLength(cbBuffer);

        // Create a media sample and add the buffer to the sample.
        ComPtr<IMFSample> pSample;
        MFCreateSample_(&pSample);
        if (pSample)
        {
            pSample->AddBuffer(pBuffer.Get());
            pSample->SetSampleTime(start);
            pSample->SetSampleDuration(duration);
            m_mf_writer->WriteSample(m_mf_audio_index, pSample.Get());
        }
    }

    return true;
}


fcIMP4Context* fcMP4CreateOSEncoderContext(fcMP4Config &conf, fcIGraphicsDevice *dev, const char *path)
{
    auto ret = new fcMP4ContextWMF(conf, dev, path);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}
