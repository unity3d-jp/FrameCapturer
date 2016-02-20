#ifndef fcBuffer_h
#define fcBuffer_h

#include <vector>
#include <algorithm>


template<class T>
class TDataRef
{
public:
    typedef T value_type;
    typedef T*              pointer;
    typedef const T*        const_pointer;
    typedef pointer         iterator;
    typedef const_pointer   const_iterator;

    TDataRef() : m_data(nullptr), m_size(0) {}
    TDataRef(void *data_, size_t size_) : m_data((pointer)data_), m_size(size_) {}
    TDataRef(std::string &v) : m_data((pointer)&v[0]), m_size(v.size()) {}
    template<class U, class A> TDataRef(std::vector<U, A> &v) : m_data((pointer)&v[0]), m_size(v.size()) {}
    template<class U, size_t L> TDataRef(std::array<U, L> &v) : m_data((pointer)&v[0]), m_size(v.size()) {}
    template<class U, size_t L> TDataRef(U(&v)[L]) : m_data((pointer)v), m_size(L) {}

    char&       operator[](size_t i)        { return m_data[i]; }
    const char& operator[](size_t i) const  { return m_data[i]; }

    size_t          size() const    { return m_size; }
    iterator        begin()         { return m_data; }
    iterator        end()           { return m_data + m_size; }
    const_iterator  begin() const   { return m_data; }
    const_iterator  end() const     { return m_data + m_size; }
    pointer         ptr()           { return m_data; }
    const_pointer   ptr() const     { return m_data; }

private:
    pointer m_data;
    size_t m_size;
};
typedef TDataRef<char> DataRef;


template<class T>
class TBuffer
{
public:
    typedef T value_type;
    typedef std::vector<value_type>     container;
    typedef typename container::iterator         iterator;
    typedef typename container::const_iterator   const_iterator;
    typedef typename container::pointer          pointer;
    typedef typename container::const_pointer    const_pointer;


    TBuffer() {}
    TBuffer(const void *src, size_t len) : m_data((const_pointer)src, (const_pointer)src+len) {}
    TBuffer(const TDataRef<T> &buf) : m_data(buf.begin(), buf.end()) {}
    TBuffer(container&& src) : m_data(src) {}

    value_type&         operator[](size_t i) { return m_data[i]; }
    const value_type&   operator[](size_t i) const { return m_data[i]; }

    container&      get()           { return m_data; }
    container&&     move()          { return std::move(m_data); }
    size_t          size() const    { return m_data.size(); }
    bool            empty() const   { return m_data.empty(); }
    iterator        begin()         { return m_data.begin(); }
    iterator        end()           { return m_data.end(); }
    const_iterator  begin() const   { return m_data.begin(); }
    const_iterator  end() const     { return m_data.end(); }
    pointer         ptr()           { return m_data.empty() ? nullptr : &m_data[0]; }
    const_pointer   ptr() const     { return m_data.empty() ? nullptr : &m_data[0]; }

    void assign(const void *src, size_t len)
    {
        m_data.assign((const_pointer)src, (const_pointer)src + len);
    }

    void append(const void *src, size_t len)
    {
        m_data.insert(m_data.end(), (const_pointer)src, (const_pointer)src + len);
    }

    void resize(size_t newsize)
    {
        m_data.resize(newsize);
    }

    void clear()
    {
        m_data.clear();
    }

    void reserve(size_t newsize)
    {
        m_data.reserve(newsize);
    }

protected:
    container m_data;
};
typedef TBuffer<char> Buffer;


class BinaryStream
{
public:
    virtual ~BinaryStream() {}
    virtual size_t  tellg() = 0;
    virtual void    seekg(size_t pos) = 0;
    virtual size_t  read(void *dst, size_t len) = 0;

    virtual size_t  tellp() = 0;
    virtual void    seekp(size_t pos) = 0;
    virtual size_t  write(const void *data, size_t len) = 0;
};

inline BinaryStream& operator<<(BinaryStream &o, const int8_t&   v) { o.write(&v, 1); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const int16_t&  v) { o.write(&v, 2); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const int32_t&  v) { o.write(&v, 4); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const int64_t&  v) { o.write(&v, 8); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const uint8_t&  v) { o.write(&v, 1); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const uint16_t& v) { o.write(&v, 2); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const uint32_t& v) { o.write(&v, 4); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const uint64_t& v) { o.write(&v, 8); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const float&    v) { o.write(&v, 4); return o; }
inline BinaryStream& operator<<(BinaryStream &o, const double&   v) { o.write(&v, 8); return o; }

inline BinaryStream& operator>>(BinaryStream &o, int8_t&   v) { o.read(&v, 1); return o; }
inline BinaryStream& operator>>(BinaryStream &o, int16_t&  v) { o.read(&v, 2); return o; }
inline BinaryStream& operator>>(BinaryStream &o, int32_t&  v) { o.read(&v, 4); return o; }
inline BinaryStream& operator>>(BinaryStream &o, int64_t&  v) { o.read(&v, 8); return o; }
inline BinaryStream& operator>>(BinaryStream &o, uint8_t&  v) { o.read(&v, 1); return o; }
inline BinaryStream& operator>>(BinaryStream &o, uint16_t& v) { o.read(&v, 2); return o; }
inline BinaryStream& operator>>(BinaryStream &o, uint32_t& v) { o.read(&v, 4); return o; }
inline BinaryStream& operator>>(BinaryStream &o, uint64_t& v) { o.read(&v, 8); return o; }
inline BinaryStream& operator>>(BinaryStream &o, float&    v) { o.read(&v, 4); return o; }
inline BinaryStream& operator>>(BinaryStream &o, double&   v) { o.read(&v, 8); return o; }


class BufferStream : public BinaryStream
{
public:
    BufferStream(Buffer &buf) : m_buf(buf), m_wpos(buf.size()), m_rpos(), m_delete_flag(false) {}
    BufferStream(Buffer *buf, bool del) : m_buf(*buf), m_wpos(buf->size()), m_rpos(), m_delete_flag(del) {}
    ~BufferStream() { if (m_delete_flag) { delete &m_buf; } }

    Buffer& get()               { return m_buf; }
    const Buffer& get() const   { return m_buf; }

    size_t tellg() override
    {
        return m_rpos;
    }

    void seekg(size_t pos) override
    {
        m_rpos = std::min<size_t>(pos, m_buf.size());
    }

    size_t read(void *dst, size_t len) override
    {
        if (m_buf.empty()) { return 0; }
        len = std::min<size_t>(len, m_buf.size() - m_rpos);
        memcpy(dst, &m_buf[m_rpos], len);
        m_rpos += len;
        return len;
    }


    size_t tellp() override
    {
        return m_wpos;
    }

    void seekp(size_t pos) override
    {
        m_wpos = std::min<size_t>(pos, m_buf.size());
    }

    size_t write(const void *data, size_t len) override
    {
        size_t required_size = m_wpos + len;
        if (m_buf.size() < required_size) {
            m_buf.resize(required_size);
        }
        memcpy(&m_buf[m_wpos], data, len);
        m_wpos += len;
        return len;
    }

protected:
    Buffer &m_buf;
    size_t m_wpos;
    size_t m_rpos;
    bool m_delete_flag;
};


class StdOStream : public BinaryStream
{
public:
    StdOStream(std::ostream& os) : m_os(os), m_delete_flag(false) {}
    StdOStream(std::ostream *os, bool del) : m_os(*os), m_delete_flag(del) {}
    ~StdOStream() { if (m_delete_flag) { delete &m_os; } }

    std::ostream& get()             { return m_os; }
    const std::ostream& get() const { return m_os; }

    // dummy
    size_t  tellg() override { return 0; }
    void    seekg(size_t /*pos*/) override {}
    size_t  read(void* /*dst*/, size_t /*len*/) override { return 0; }

    size_t tellp() override
    {
        return m_os.tellp();
    }

    void seekp(size_t pos) override
    {
        m_os.seekp(pos);
    }

    size_t write(const void *data, size_t len) override
    {
        m_os.write((const char*)data, len);
        return len;
    }

protected:
    std::ostream& m_os;
    bool m_delete_flag;
};


class StdIStream : public BinaryStream
{
public:
    StdIStream(std::istream& is) : m_is(is), m_delete_flag(false) {}
    StdIStream(std::istream *is, bool del) : m_is(*is), m_delete_flag(del) {}
    ~StdIStream() { if (m_delete_flag) { delete &m_is; } }

    std::istream& get()             { return m_is; }
    const std::istream& get() const { return m_is; }

    size_t tellg() override
    {
        return m_is.tellg();
    }

    void seekg(size_t pos) override
    {
        m_is.seekg(pos);
    }

    size_t read(void *dst, size_t len) override
    {
        m_is.read((char*)dst, len);
        return m_is.gcount();
    }

    // dummy
    size_t  tellp() override { return 0; }
    void    seekp(size_t /*pos*/) override {}
    size_t  write(const void* /*data*/, size_t /*len*/) override { return 0; }

protected:
    std::istream& m_is;
    bool m_delete_flag;
};


class StdIOStream : public BinaryStream
{
public:
    StdIOStream(std::iostream& ios) : m_ios(ios), m_delete_flag(false) {}
    StdIOStream(std::iostream *ios, bool del) : m_ios(*ios), m_delete_flag(del) {}
    ~StdIOStream() { if (m_delete_flag) { delete &m_ios; } }

    std::iostream& get()            { return m_ios; }
    const std::iostream& get() const{ return m_ios; }

    size_t tellg() override
    {
        return m_ios.tellg();
    }

    void seekg(size_t pos) override
    {
        m_ios.seekg(pos);
    }

    size_t read(void *dst, size_t len) override
    {
        m_ios.read((char*)dst, len);
        return m_ios.gcount();
    }


    size_t tellp() override
    {
        return m_ios.tellp();
    }

    void seekp(size_t pos) override
    {
        m_ios.seekp(pos);
    }

    size_t write(const void *data, size_t len) override
    {
        m_ios.write((const char*)data, len);
        return len;
    }

protected:
    std::iostream& m_ios;
    bool m_delete_flag;
};



typedef size_t (*tellg_t)(void *obj);
typedef void   (*seekg_t)(void *obj, size_t pos);
typedef size_t (*read_t)(void *obj, void *dst, size_t len);

typedef size_t (*tellp_t)(void *obj);
typedef void   (*seekp_t)(void *obj, size_t pos);
typedef size_t (*write_t)(void *obj, const void *data, size_t len);

struct CustomStreamData
{
    void *obj;
    tellg_t tellg;
    seekg_t seekg;
    read_t  read;
    tellp_t tellp;
    seekp_t seekp;
    write_t write;

    CustomStreamData()
        : obj()
        , tellg(), seekg(), read()
        , tellp(), seekp(), write()
    {}
};

class CustomStream : public BinaryStream
{
public:
    CustomStream(const CustomStreamData& csd) : m_csd(csd) {}

    CustomStreamData& get()             { return m_csd; }
    const CustomStreamData& get() const { return m_csd; }

    size_t tellg() override
    {
        return m_csd.tellg(m_csd.obj);
    }

    void seekg(size_t pos) override
    {
        return m_csd.seekg(m_csd.obj, pos);
    }

    size_t read(void *dst, size_t len) override
    {
        return m_csd.read(m_csd.obj, dst, len);
    }


    size_t tellp() override
    {
        return m_csd.tellp(m_csd.obj);
    }

    void seekp(size_t pos) override
    {
        return m_csd.seekp(m_csd.obj, pos);
    }

    size_t write(const void *data, size_t len) override
    {
        return m_csd.write(m_csd.obj, data, len);
    }

private:
    CustomStreamData m_csd;
};


#endif // fcBuffer_h
