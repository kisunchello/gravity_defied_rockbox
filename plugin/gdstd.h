/* Minimal STL replacements for the plugin environment (no libstdc++).
 * gd::vector<T> and gd::string, no exceptions, element-wise copy so non-POD
 * element types (e.g. gd::vector<gd::vector<int>>) work. */
#ifndef GDSTD_H
#define GDSTD_H

#include <stddef.h>
#include <stdint.h>

void* operator new(size_t);
void* operator new[](size_t);
void  operator delete(void*) noexcept;
void  operator delete[](void*) noexcept;
/* placement new */
inline void* operator new(size_t, void* p) noexcept { return p; }

namespace gd {

template <typename T>
class vector {
    T*     m_data;
    size_t m_size;
    size_t m_cap;

    void grow(size_t need)
    {
        if (need <= m_cap) return;
        size_t nc = m_cap ? m_cap * 2 : 4;
        if (nc < need) nc = need;
        T* nd = (T*)::operator new(nc * sizeof(T));
        for (size_t i = 0; i < m_size; i++) {
            new (&nd[i]) T(m_data[i]);
            m_data[i].~T();
        }
        ::operator delete(m_data);
        m_data = nd;
        m_cap  = nc;
    }

public:
    vector() : m_data(0), m_size(0), m_cap(0) {}
    explicit vector(size_t n) : m_data(0), m_size(0), m_cap(0) { resize(n); }
    vector(const vector& o) : m_data(0), m_size(0), m_cap(0)
    {
        grow(o.m_size);
        for (size_t i = 0; i < o.m_size; i++) new (&m_data[i]) T(o.m_data[i]);
        m_size = o.m_size;
    }
    vector& operator=(const vector& o)
    {
        if (this == &o) return *this;
        clear();
        grow(o.m_size);
        for (size_t i = 0; i < o.m_size; i++) new (&m_data[i]) T(o.m_data[i]);
        m_size = o.m_size;
        return *this;
    }
    ~vector() { clear(); ::operator delete(m_data); }

    void push_back(const T& v) { grow(m_size + 1); new (&m_data[m_size]) T(v); m_size++; }

    void resize(size_t n)
    {
        if (n > m_size) {
            grow(n);
            for (size_t i = m_size; i < n; i++) new (&m_data[i]) T();
        } else {
            for (size_t i = n; i < m_size; i++) m_data[i].~T();
        }
        m_size = n;
    }

    void resize(size_t n, const T& fill)
    {
        if (n > m_size) {
            grow(n);
            for (size_t i = m_size; i < n; i++) new (&m_data[i]) T(fill);
        } else {
            for (size_t i = n; i < m_size; i++) m_data[i].~T();
        }
        m_size = n;
    }

    void clear() { for (size_t i = 0; i < m_size; i++) m_data[i].~T(); m_size = 0; }

    size_t size()  const { return m_size; }
    bool   empty() const { return m_size == 0; }
    T&       operator[](size_t i)       { return m_data[i]; }
    const T& operator[](size_t i) const { return m_data[i]; }
    T&       back()        { return m_data[m_size - 1]; }
    const T& back()  const { return m_data[m_size - 1]; }
    T*       begin()       { return m_data; }
    T*       end()         { return m_data + m_size; }
    const T* begin() const { return m_data; }
    const T* end()   const { return m_data + m_size; }
};

class string {
    char*  m_buf;
    size_t m_len;
    size_t m_cap;

    void ensure(size_t need)
    {
        if (need + 1 <= m_cap) return;
        size_t nc = m_cap ? m_cap * 2 : 16;
        if (nc < need + 1) nc = need + 1;
        char* nb = (char*)::operator new(nc);
        for (size_t i = 0; i < m_len; i++) nb[i] = m_buf[i];
        nb[m_len] = 0;
        ::operator delete(m_buf);
        m_buf = nb;
        m_cap = nc;
    }
    static size_t slen(const char* s) { size_t n = 0; if (s) while (s[n]) n++; return n; }

public:
    string() : m_buf(0), m_len(0), m_cap(0) { ensure(0); }
    string(const char* s) : m_buf(0), m_len(0), m_cap(0) { assign(s, slen(s)); }
    string(const string& o) : m_buf(0), m_len(0), m_cap(0) { assign(o.m_buf, o.m_len); }
    string& operator=(const string& o) { if (this != &o) assign(o.m_buf, o.m_len); return *this; }
    string& operator=(const char* s)   { assign(s, slen(s)); return *this; }
    ~string() { ::operator delete(m_buf); }

    void assign(const char* s, size_t n)
    {
        ensure(n);
        for (size_t i = 0; i < n; i++) m_buf[i] = s[i];
        m_len = n;
        m_buf[m_len] = 0;
    }
    string& operator+=(const char* s)
    {
        size_t n = slen(s);
        ensure(m_len + n);
        for (size_t i = 0; i < n; i++) m_buf[m_len + i] = s[i];
        m_len += n;
        m_buf[m_len] = 0;
        return *this;
    }
    string& operator+=(const string& o) { return (*this += o.c_str()); }
    string& operator+=(char c)
    {
        ensure(m_len + 1);
        m_buf[m_len++] = c;
        m_buf[m_len] = 0;
        return *this;
    }

    const char* c_str() const { return m_buf ? m_buf : ""; }
    size_t size()  const { return m_len; }
    size_t length()const { return m_len; }
    bool   empty() const { return m_len == 0; }
    char&       operator[](size_t i)       { return m_buf[i]; }
    const char& operator[](size_t i) const { return m_buf[i]; }

    bool operator==(const char* s) const
    {
        size_t n = slen(s);
        if (n != m_len) return false;
        for (size_t i = 0; i < n; i++) if (m_buf[i] != s[i]) return false;
        return true;
    }
    bool operator==(const string& o) const { return *this == o.c_str(); }
};

} /* namespace gd */

#endif
