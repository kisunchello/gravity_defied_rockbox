/* Minimal C++ runtime for the plugin (no libstdc++).
 * NOTE: temporary static pool; will be switched to the TLSF heap when the
 * game needs more than this. Includes no Rockbox headers. */
#include <stddef.h>

static char   s_pool[128 * 1024];
static size_t s_off = 0;

void* operator new(size_t n)
{
    s_off = (s_off + 7) & ~((size_t)7);
    if (s_off + n > sizeof(s_pool))
        return 0;
    void* p = &s_pool[s_off];
    s_off += n;
    return p;
}
void* operator new[](size_t n)            { return operator new(n); }
void  operator delete(void* p)            { (void)p; }
void  operator delete[](void* p)          { (void)p; }
void  operator delete(void* p, size_t)    { (void)p; }
void  operator delete[](void* p, size_t)  { (void)p; }

/* bump-pool mark / release: reclaim everything allocated since a mark, as long
 * as nothing allocated after the mark is still referenced. Used to free a mod
 * pack's level loader + its play's GamePhysics after each mod run. */
extern "C" size_t gd_pool_mark(void)        { return s_off; }
extern "C" void   gd_pool_release(size_t m) { if (m <= s_off) s_off = m; }

extern "C" void __cxa_pure_virtual(void)  { }
