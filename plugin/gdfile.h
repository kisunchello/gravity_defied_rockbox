/* FileStream replacement: same API the game expects (readVariable / setPos /
 * isOpen) but backed by the Rockbox file API through the shim. Read-only. */
#ifndef GDFILE_H
#define GDFILE_H

#include <stddef.h>
#include "rbshim.h"

class FileStream {
    int fd;
public:
    FileStream() : fd(-1) {}
    explicit FileStream(const char* path) : fd(shim_open(path)) {}
    ~FileStream() { if (fd >= 0) shim_close(fd); }

    bool open(const char* path)
    {
        if (fd >= 0) shim_close(fd);
        fd = shim_open(path);
        return fd >= 0;
    }
    bool isOpen() const { return fd >= 0; }
    void setPos(int pos) { shim_seek(fd, pos); }

    template <class T>
    void readVariable(T* p, bool swapEndianness = false, size_t size = 0)
    {
        unsigned char* b = (unsigned char*)p;
        if (!size) size = sizeof(T);
        shim_read(fd, b, (int)size);
        if (swapEndianness) {
            for (size_t i = 0; i < size / 2; i++) {
                unsigned char t = b[i];
                b[i] = b[size - 1 - i];
                b[size - 1 - i] = t;
            }
        }
    }
};

#endif
