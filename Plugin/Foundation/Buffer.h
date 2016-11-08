#pragma once

#include <vector>
#include <algorithm>

void*       AlignedAlloc(size_t size, size_t align);
void        AlignedFree(void *p);


// low-level vector<>. T must be POD type
template<class T>
class RawVector
{
public:
    typedef T               value_type;
    typedef T&              reference;
    typedef const T&        const_reference;
    typedef T*              pointer;
    typedef const T*        const_pointer;
    typedef pointer         iterator;
    typedef const_pointer   const_iterator;

    RawVector() {}
    explicit RawVector(size_t size) { resize(size); }
    RawVector(const_pointer src, size_t len) { assign(src, len); }
    RawVector(RawVector& v) { assign(v.data(), v.size()); }
    RawVector& operator=(RawVector& v) { assign(v.data(), v.size()); return *this; }
    RawVector(RawVector&& v)
    {
        std::swap(m_data, v.m_data);
        std::swap(m_size, v.m_size);
        std::swap(m_capacity, v.m_capacity);
    }
    RawVector& operator=(RawVector&& v)
    {
        clear();
        std::swap(m_data, v.m_data);
        std::swap(m_size, v.m_size);
        std::swap(m_capacity, v.m_capacity);
        return *this;
    }
    ~RawVector() { clear(); }

    value_type&         operator[](size_t i) { return m_data[i]; }
    const value_type&   operator[](size_t i) const { return m_data[i]; }

    size_t          size() const    { return m_size; }
    bool            empty() const   { return m_size == 0; }
    iterator        begin()         { return m_data; }
    const_iterator  begin() const   { return m_data; }
    iterator        end()           { return m_data + m_size; }
    const_iterator  end() const     { return m_data + m_size; }
    pointer         data()           { return m_data; }
    const_pointer   data() const     { return m_data; }

    T&       front()        { return m_data[0]; }
    const T& front() const  { return m_data[0]; }
    T&       back()         { return m_data[m_size - 1]; }
    const T& back() const   { return m_data[m_size - 1]; }


    static void* allocate(size_t size) { return AlignedAlloc(size, 0x20); }
    static void deallocate(void *addr, size_t size) { AlignedFree(addr); }

    void reserve(size_t s)
    {
        if (s > m_capacity) {
            s = std::max<size_t>(s, m_size * 2);
            size_t newsize = sizeof(T) * s;
            size_t oldsize = sizeof(T) * m_size;

            T *newdata = (T*)allocate(newsize);
            memcpy(newdata, m_data, oldsize);
            deallocate(m_data, oldsize);
            m_data = newdata;
            m_capacity = s;
        }
    }

    void resize(size_t s)
    {
        reserve(s);
        m_size = s;
    }

    void clear()
    {
        size_t oldsize = sizeof(T) * m_size;
        deallocate(m_data, oldsize);
        m_data = nullptr;
        m_size = m_capacity = 0;
    }

    void swap(RawVector &other)
    {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
    }

    void assign(const_pointer data, size_t num)
    {
        resize(num);
        memcpy(m_data, data, sizeof(T)*num);
    }

    template<class FwdIter>
    void assign(FwdIter first, FwdIter last)
    {
        size_t num = std::distance(first, last);
        resize(num);
        memcpy(m_data, first, sizeof(T)*num);
    }

    void append(const_pointer data, size_t num)
    {
        size_t pos = m_size;
        resize(m_size + num);
        memcpy(&m_data[pos], data, sizeof(T)*num);
    }

    void erase(iterator first, iterator last)
    {
        size_t s = std::distance(first, last);
        std::copy(last, end(), first);
        m_size -= s;
    }

    void erase(iterator pos)
    {
        erase(pos, pos + 1);
    }

    void push_back(const T& v)
    {
        resize(m_size + 1);
        back() = v;
    }

    void pop_back()
    {
        --m_size;
    }

    bool operator == (const RawVector& other) const
    {
        return m_size == other.m_size && memcmp(m_data, other.m_data, sizeof(T)*m_size) == 0;
    }

    bool operator != (const RawVector& other) const
    {
        return !(*this == other);
    }

protected:
    T *m_data = nullptr;
    size_t m_size = 0;
    size_t m_capacity = 0;
};
typedef RawVector<char> Buffer;


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
        return (size_t)m_os.tellp();
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
        return (size_t)m_is.tellg();
    }

    void seekg(size_t pos) override
    {
        m_is.seekg(pos);
    }

    size_t read(void *dst, size_t len) override
    {
        m_is.read((char*)dst, len);
        return (size_t)m_is.gcount();
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
        return (size_t)m_ios.tellg();
    }

    void seekg(size_t pos) override
    {
        m_ios.seekg(pos);
    }

    size_t read(void *dst, size_t len) override
    {
        m_ios.read((char*)dst, len);
        return (size_t)m_ios.gcount();
    }


    size_t tellp() override
    {
        return (size_t)m_ios.tellp();
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
