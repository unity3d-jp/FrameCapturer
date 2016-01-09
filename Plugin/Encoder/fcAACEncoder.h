#ifndef fcAACEncoder_h
#define fcAACEncoder_h


class fcAACEncoder
{
public:
    static bool loadModule();

    fcAACEncoder(int sampling_rate, int num_channels, int bitrate);
    ~fcAACEncoder();
    operator bool() const;
    Buffer encode(const float *samples, int num_samples);
    const std::string& getHeader();

private:
    void *m_handle;
    unsigned long m_num_read_samples;
    unsigned long m_output_size;
    std::vector<float> m_tmp_buf;
    std::string m_aac_tmp_buf;
    std::string m_aac_buf;
    std::string m_aac_header;
};


#endif // fcAACEncoder_h
