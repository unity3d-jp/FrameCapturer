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
#include <mferror.h>

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")


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

    IMFSinkWriter       *m_mf_writer = nullptr;
    DWORD               m_mf_video_index = 0;
    DWORD               m_mf_audio_index = 0;
};



template <class T>
static inline void SafeRelease(T *&ptr)
{
    if (ptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}



static LazyInstance<MFInitializer> g_MFInitializer;

MFInitializer::MFInitializer()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    MFStartup(MF_VERSION, MFSTARTUP_LITE);
}

MFInitializer::~MFInitializer()
{
    MFShutdown();
    CoUninitialize();
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
        SafeRelease(m_mf_writer);
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
    IMFSinkWriter   *pSinkWriter = nullptr;
    HRESULT         hr;

    {
        IMFAttributes *pAttributes = nullptr;
        MFCreateAttributes(&pAttributes, 1);
        if (pAttributes) {
            hr = pAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4);

            std::wstring wpath;
            wpath.resize(strlen(path)+1);
            std::mbstowcs(&wpath[0], path, wpath.size());
            hr = MFCreateSinkWriterFromURL(wpath.data(), nullptr, pAttributes, &pSinkWriter);
            SafeRelease(pAttributes);
        }
    }
    if (!pSinkWriter) { return false; }

    // Set the video output media type.
    if(m_conf.video) {
        IMFMediaType *pVideoOutMediaType = nullptr;
        MFCreateMediaType(&pVideoOutMediaType);
        if (pVideoOutMediaType) {
            pVideoOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pVideoOutMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
            pVideoOutMediaType->SetUINT32(MF_MT_AVG_BITRATE, m_conf.video_target_bitrate);
            pVideoOutMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            pVideoOutMediaType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base);
            MFSetAttributeSize(pVideoOutMediaType, MF_MT_FRAME_SIZE, m_conf.video_width, m_conf.video_height);
            MFSetAttributeRatio(pVideoOutMediaType, MF_MT_FRAME_RATE, m_conf.video_target_framerate, 1);
            MFSetAttributeRatio(pVideoOutMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            pSinkWriter->AddStream(pVideoOutMediaType, &m_mf_video_index);

            SafeRelease(pVideoOutMediaType);
        }

        IMFMediaType *pVideoInputMediaType = nullptr;
        MFCreateMediaType(&pVideoInputMediaType);
        if (pVideoInputMediaType) {
            pVideoInputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pVideoInputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420);
            pVideoInputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            MFSetAttributeSize(pVideoInputMediaType, MF_MT_FRAME_SIZE, m_conf.video_width, m_conf.video_height);
            MFSetAttributeRatio(pVideoInputMediaType, MF_MT_FRAME_RATE, m_conf.video_target_framerate, 1);
            MFSetAttributeRatio(pVideoInputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            pSinkWriter->SetInputMediaType(m_mf_video_index, pVideoInputMediaType, nullptr);

            SafeRelease(pVideoInputMediaType);
        }
    }

    // Set the audio output media type.
    if (m_conf.audio) {
        IMFMediaType *pAudioOutMediaType = nullptr;
        MFCreateMediaType(&pAudioOutMediaType);
        if (pAudioOutMediaType) {
            pAudioOutMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            pAudioOutMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, m_conf.audio_sample_rate);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, m_conf.audio_num_channels);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, m_conf.audio_target_bitrate / 8);
            pAudioOutMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);
            pAudioOutMediaType->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, 0x29);
            pSinkWriter->AddStream(pAudioOutMediaType, &m_mf_audio_index);

            SafeRelease(pAudioOutMediaType);
        }

        IMFMediaType *pAudioInMediaType = nullptr;
        MFCreateMediaType(&pAudioInMediaType);
        if (pAudioInMediaType) {
            pAudioInMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
            pAudioInMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, m_conf.audio_sample_rate);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, m_conf.audio_num_channels);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, m_conf.audio_sample_rate * 4 * m_conf.audio_num_channels);
            pAudioInMediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4 * m_conf.audio_num_channels);
            pSinkWriter->SetInputMediaType(m_mf_audio_index, pAudioInMediaType, nullptr);

            SafeRelease(pAudioInMediaType);
        }
    }

    // Tell the sink writer to start accepting data.
    if (!SUCCEEDED(pSinkWriter->BeginWriting())) {
        SafeRelease(pSinkWriter);
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


    IMFMediaBuffer *pBuffer = nullptr;
    MFCreateMemoryBuffer(buffer_size, &pBuffer);
    if (pBuffer)
    {
        BYTE *pData = nullptr;
        pBuffer->Lock(&pData, nullptr, nullptr);
        memcpy(pData, i420.y, buffer_size);
        pBuffer->Unlock();
        pBuffer->SetCurrentLength(buffer_size);

        // Create a media sample and add the buffer to the sample.
        IMFSample *pSample = nullptr;
        MFCreateSample(&pSample);
        if (pSample)
        {
            pSample->AddBuffer(pBuffer);
            pSample->SetSampleTime(start);
            pSample->SetSampleDuration(duration);
            m_mf_writer->WriteSample(m_mf_video_index, pSample);
        }
        SafeRelease(pSample);
    }

    SafeRelease(pBuffer);
    return true;
}


bool fcMP4ContextWMF::addAudioFrame(const float *samples, int num_samples, fcTime timestamp)
{
    if (!!m_mf_writer || !samples) { return false; }

    const LONGLONG start = SecondsToTimestamp(timestamp);
    const LONGLONG duration = SecondsToTimestamp(1.0 / m_conf.video_target_framerate);
    const DWORD cbBuffer = num_samples * 4;

    // Create a new memory buffer.
    IMFMediaBuffer *pBuffer = nullptr;
    HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

    // Lock the buffer and copy the video frame to the buffer.
    if (SUCCEEDED(hr))
    {
        BYTE *pData = nullptr;
        pBuffer->Lock(&pData, nullptr, nullptr);
        memcpy(pData, samples, cbBuffer);
        pBuffer->Unlock();
        pBuffer->SetCurrentLength(cbBuffer);

        // Create a media sample and add the buffer to the sample.
        IMFSample *pSample = nullptr;
        hr = MFCreateSample(&pSample);
        if (SUCCEEDED(hr))
        {
            pSample->AddBuffer(pBuffer);
            pSample->SetSampleTime(start);
            pSample->SetSampleDuration(duration);
            m_mf_writer->WriteSample(m_mf_audio_index, pSample);
        }
        SafeRelease(pSample);
    }
    SafeRelease(pBuffer);

    return SUCCEEDED(hr);
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
