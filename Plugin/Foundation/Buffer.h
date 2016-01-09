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
    iterator        begin()         { return m_data.begin(); }
    iterator        end()           { return m_data.end(); }
    const_iterator  begin() const   { return m_data.begin(); }
    const_iterator  end() const     { return m_data.end(); }
    pointer         ptr()           { return m_data.empty() ? nullptr : &m_data[0]; }
    const_pointer   ptr() const     { return m_data.empty() ? nullptr : &m_data[0]; }

    void assign(const_pointer src, size_t len)
    {
        m_data.assign(src, src + len);
    }

    void resize(size_t size)
    {
        m_data.resize(size);
    }

    void clear()
    {
        m_data.clear();
    }

protected:
    container m_data;
};
typedef TBuffer<char> Buffer;


template<class T>
class TStreamBuffer : public TBuffer<T>
{
typedef TBuffer<T> super;
public:
    TStreamBuffer() {}
    TStreamBuffer(const void *src, size_t len) : super(src, len), m_wpos(0), m_rpos(0) {}
    TStreamBuffer(const TDataRef<T> &buf) : super(buf), m_wpos(0), m_rpos(0) {}
    TStreamBuffer(container&& src) : super(src), m_wpos(0), m_rpos(0) {}

    void assign(const_pointer src, size_t len)
    {
        m_data.assign(src, src + len);
        m_wpos = std::min<size_t>(m_wpos, size);
        m_rpos = std::min<size_t>(m_rpos, size);
    }

    void resize(size_t size)
    {
        m_data.resize(size);
        m_wpos = std::min<size_t>(m_wpos, size);
        m_rpos = std::min<size_t>(m_rpos, size);
    }

    void clear()
    {
        m_data.clear();
        m_wpos = m_rpos = 0;
    }


    size_t read(void *dst, size_t len)
    {
        if (m_data.empty()) { return 0; }
        len = std::min<size_t>(len, m_data.size() - m_rpos);
        memcpy(dst, &m_data[m_rpos], sizeof(value_type) * len);
        m_rpos += len;
        return len;
    }

    size_t write(const void *data, size_t len)
    {
        size_t required_size = m_wpos + len;
        if (m_data.size() < required_size) {
            m_data.resize(required_size);
        }
        memcpy(&m_data[m_wpos], data, sizeof(value_type) * len);
        m_wpos += len;
        return len;
    }

    size_t  getRPosition() const { return m_rpos; }
    void    setRPosition(size_t pos) { m_rpos = std::min<size_t>(pos, m_data.size()); }
    size_t  getWPosition() const { return m_wpos; }
    void    setWPosition(size_t pos) { m_wpos = std::min<size_t>(pos, m_data.size()); }

protected:
    size_t m_wpos;
    size_t m_rpos;
};
typedef TStreamBuffer<char> StreamBuffer;
inline StreamBuffer& operator<<(StreamBuffer &o, uint8_t  v) { o.write(&v, sizeof(v)); return o; }
inline StreamBuffer& operator<<(StreamBuffer &o, uint16_t v) { o.write(&v, sizeof(v)); return o; }
inline StreamBuffer& operator<<(StreamBuffer &o, uint32_t v) { o.write(&v, sizeof(v)); return o; }
inline StreamBuffer& operator<<(StreamBuffer &o, uint64_t v) { o.write(&v, sizeof(v)); return o; }

#endif // fcBuffer_h
