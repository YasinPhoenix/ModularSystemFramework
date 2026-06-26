#pragma once

#include "IFile.h"
#include "../module/IModule.h"

class IFileSystem : public IModule
{
public:
    virtual ~IFileSystem() = default;

    virtual bool mounted() const = 0;

    virtual bool format() = 0;

    virtual bool exists(const char *path) = 0;
    virtual bool remove(const char *path) = 0;
    virtual bool rename(const char *from, const char *to) = 0;

    virtual bool mkdir(const char *path) = 0;
    virtual bool rmdir(const char *path) = 0;

    virtual IFile *open(const char *path, FileMode mode) = 0;

    virtual size_t totalBytes() const = 0;
    virtual size_t usedBytes() const = 0;

    virtual void list(const char *path, void (*callback)(const FileInfo &, void *), void *ctx) = 0;

    virtual IFileSystem *asFileSystem() { return this; }
};