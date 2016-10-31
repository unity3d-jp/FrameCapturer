#include "pch.h"
#include "fcFoundation.h"
#include "../fcMP4File.h"
#include "fcMP4_WMF.h"

#include "../fcI420.h"
#include "GraphicsDevice/fcGraphicsDevice.h"
#include "Foundation/TaskQueue.h"

#include <Windows.h>
#include <codecapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

class fcMP4ContextWMF : public fcIMP4Context
{
public:
    using VideoBuffer = Buffer;
    using VideoBufferPtr = std::shared_ptr<VideoBuffer>;
    using VideoBufferQueue = ResourceQueue<VideoBufferPtr>;

    using AudioBuffer = RawVector<float>;
    using AudioBufferPtr = std::shared_ptr<AudioBuffer>;
    using AudioBufferQueue = ResourceQueue<AudioBufferPtr>;


    fcMP4ContextWMF(fcMP4Config &conf, fcIGraphicsDevice *dev);
    ~fcMP4ContextWMF();

    void release() override;

    const char* getAudioEncoderInfo() override;
    const char* getVideoEncoderInfo() override;

    void addOutputStream(fcStream *s) override;

    bool addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp) override;
    bool addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp) override;
    bool addAudioFrame(const float *samples, int num_samples, fcTime timestamp) override;

    bool isValid() const { return m_mf_writer != nullptr; }

private:
    HRESULT InitializeSinkWriter();

    fcMP4Config         m_conf;
    fcIGraphicsDevice   *m_gdev = nullptr;

    TaskQueue           m_video_tasks;
    VideoBufferQueue    m_video_buffers;
    Buffer              m_rgba_image;
    fcI420Image         m_i420_image;

    TaskQueue           m_audio_tasks;
    AudioBufferQueue    m_audio_buffers;

    IMFSinkWriter       *m_mf_writer = nullptr;
    DWORD               m_mf_video_index = 0;
    DWORD               m_mf_audio_index = 0;
};



template <class T>
static inline void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = nullptr;
    }
}

fcMP4ContextWMF::fcMP4ContextWMF(fcMP4Config &conf, fcIGraphicsDevice *dev)
    : m_conf(conf)
    , m_gdev(dev)
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr))
        {
            hr = InitializeSinkWriter();
        }
    }
}

fcMP4ContextWMF::~fcMP4ContextWMF()
{
    if (m_conf.video) { m_video_tasks.stop(); }
    if (m_conf.audio) { m_audio_tasks.stop(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    HRESULT hr = m_mf_writer->Finalize();
    SafeRelease(&m_mf_writer);
    MFShutdown();
    CoUninitialize();
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
}

bool fcMP4ContextWMF::addVideoFrameTexture(void *tex, fcPixelFormat fmt, fcTime timestamp)
{
    if (!m_mf_writer) { return false; }
    return false;
}

bool fcMP4ContextWMF::addVideoFramePixels(const void *pixels, fcPixelFormat fmt, fcTime timestamp)
{
    if (!m_mf_writer) { return false; }
    return false;
}

bool fcMP4ContextWMF::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (!m_mf_writer) { return false; }
    return false;
}


// Format constants
const UINT32 VIDEO_WIDTH = 320;
const UINT32 VIDEO_HEIGHT = 240;
const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 384000;
const UINT32 VIDEO_PELS = VIDEO_WIDTH * VIDEO_HEIGHT;
const UINT32 VIDEO_FRAME_COUNT = 20 * VIDEO_FPS;

// Buffer to hold the video frame data.
DWORD videoFrameBuffer[VIDEO_PELS];

HRESULT fcMP4ContextWMF::InitializeSinkWriter()
{
    IMFSinkWriter   *pSinkWriter = nullptr;
    HRESULT         hr;

    {
        IMFAttributes *pAttributes = nullptr;
        hr = MFCreateAttributes(&pAttributes, 1);
        hr = pAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);
        if (SUCCEEDED(hr))
        {
            hr = MFCreateSinkWriterFromURL(L"output.mp4", nullptr, pAttributes, &pSinkWriter);
            SafeRelease(&pAttributes);
        }
    }

    // Set the video output media type.
    {
        IMFMediaType *pVideoOutMediaType = nullptr;
        hr = MFCreateMediaType(&pVideoOutMediaType);
        if (SUCCEEDED(hr))
        {
            hr = pVideoOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            hr = pVideoOutMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
            hr = pVideoOutMediaType->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
            hr = pVideoOutMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            hr = pVideoOutMediaType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);
            hr = MFSetAttributeSize(pVideoOutMediaType, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
            hr = MFSetAttributeRatio(pVideoOutMediaType, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
            hr = MFSetAttributeRatio(pVideoOutMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            hr = pSinkWriter->AddStream(pVideoOutMediaType, &m_mf_video_index);
        }
        SafeRelease(&pVideoOutMediaType);
    }

    // Set the audio output media type.
    {
        IMFMediaType *pAudioOutMediaType = nullptr;
        hr = MFCreateMediaType(&pAudioOutMediaType);
        if (SUCCEEDED(hr))
        {
            hr = pAudioOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            hr = pAudioOutMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
            hr = pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
            hr = pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
            hr = pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
            hr = pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000);
            hr = pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);
            hr = pAudioOutMediaType->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0x29);
            hr = pSinkWriter->AddStream(pAudioOutMediaType, &m_mf_audio_index);
            SafeRelease(&pAudioOutMediaType);
        }
    }

    // Set the video input media type.
    {
        IMFMediaType *pVideoInputMediaType = nullptr;
        hr = MFCreateMediaType(&pVideoInputMediaType);
        if (SUCCEEDED(hr))
        {
            hr = pVideoInputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            hr = pVideoInputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420);
            hr = pVideoInputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            hr = MFSetAttributeSize(pVideoInputMediaType, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
            hr = MFSetAttributeRatio(pVideoInputMediaType, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
            hr = MFSetAttributeRatio(pVideoInputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            hr = pSinkWriter->SetInputMediaType(m_mf_video_index, pVideoInputMediaType, nullptr);
        }
        SafeRelease(&pVideoInputMediaType);
    }

    // Set the audio input media type.
    {
        IMFMediaType *pAudioInMediaType = nullptr;
        hr = MFCreateMediaType(&pAudioInMediaType);
        if (SUCCEEDED(hr))
        {
            hr = pAudioInMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            hr = pAudioInMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
            hr = pAudioInMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
            hr = pAudioInMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
            hr = pAudioInMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
            hr = pAudioInMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 48000 * 4 * 2);
            hr = pAudioInMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4 * 2);
            hr = pSinkWriter->SetInputMediaType(m_mf_audio_index, pAudioInMediaType, nullptr);
        }
        SafeRelease(&pAudioInMediaType);
    }

    // Tell the sink writer to start accepting data.
    hr = pSinkWriter->BeginWriting();

    // Return the pointer to the caller.
    m_mf_writer = pSinkWriter;
    return hr;
}

HRESULT WriteVideoFrame(
    IMFSinkWriter *m_mf_writer,
    DWORD m_mf_video_index,
    const LONGLONG& rtStart        // Time stamp.
)
{
    const LONG cbWidth = 4 * VIDEO_WIDTH;
    const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;

    // Create a new memory buffer.
    IMFMediaBuffer *pBuffer = nullptr;
    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

    // Lock the buffer and copy the video frame to the buffer.
    if (SUCCEEDED(hr))
    {
        BYTE *pData = nullptr;
        hr = pBuffer->Lock(&pData, nullptr, nullptr);
        hr = MFCopyImage(
            pData,                      // Destination buffer.
            cbWidth,                    // Destination stride.
            (BYTE*)videoFrameBuffer,    // First row in source image.
            cbWidth,                    // Source stride.
            cbWidth,                    // Image width in bytes.
            VIDEO_HEIGHT                // Image height in pixels.
        );
        pBuffer->Unlock();
        hr = pBuffer->SetCurrentLength(cbBuffer);

        // Create a media sample and add the buffer to the sample.
        {
            IMFSample *pSample = nullptr;
            hr = MFCreateSample(&pSample);
            if (SUCCEEDED(hr))
            {
                hr = pSample->AddBuffer(pBuffer);
                hr = pSample->SetSampleTime(rtStart);
                hr = pSample->SetSampleDuration(VIDEO_FRAME_DURATION);
                hr = m_mf_writer->WriteSample(m_mf_video_index, pSample);
            }
            SafeRelease(&pSample);
        }
    }

    SafeRelease(&pBuffer);
    return hr;
}

static HRESULT WriteAudioFrame(
    IMFSinkWriter *m_mf_writer,
    DWORD m_mf_audio_index,
    const LONGLONG& rtStart        // Time stamp.
)
{
    const DWORD cbBuffer = 48000 * 4 * 2 / VIDEO_FPS;

    // Create a new memory buffer.
    IMFMediaBuffer *pBuffer = nullptr;
    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

    // Lock the buffer and copy the video frame to the buffer.
    if (SUCCEEDED(hr))
    {
        BYTE *pData = nullptr;
        hr = pBuffer->Lock(&pData, nullptr, nullptr);
        float *pfBuf = (float *)pData;
        for (int i = 0; i < 48000 * 2 / VIDEO_FPS; i += 2) {
            pfBuf[i] = pfBuf[i + 1] = std::sin(((float)rtStart + (float)i / 48000 * VIDEO_FPS * VIDEO_FRAME_DURATION) / 10000000.0f * 3.14 * 440);
        }
        pBuffer->Unlock();
        hr = pBuffer->SetCurrentLength(cbBuffer);

        // Create a media sample and add the buffer to the sample.
        IMFSample *pSample = nullptr;
        hr = MFCreateSample(&pSample);
        if (SUCCEEDED(hr))
        {
            hr = pSample->AddBuffer(pBuffer);
            hr = pSample->SetSampleTime(rtStart);
            hr = pSample->SetSampleDuration(VIDEO_FRAME_DURATION);
            hr = m_mf_writer->WriteSample(m_mf_audio_index, pSample);
        }
        SafeRelease(&pSample);
    }
    SafeRelease(&pBuffer);
    return hr;
}


fcIMP4Context* fcMP4CreateOSEncoderContext(fcMP4Config &conf, fcIGraphicsDevice *dev)
{
    auto ret = new fcMP4ContextWMF(conf, dev);
    if (!ret->isValid()) {
        delete ret;
        ret = nullptr;
    }
    return ret;
}
