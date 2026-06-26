#pragma once 

#include "file_system.h"

class IFile
{
public:
    virtual ~IFile() = default;

    virtual bool available() = 0;
    virtual size_t size() = 0;
    virtual size_t position() = 0;
    virtual bool seek(size_t pos) = 0;

    virtual size_t read(uint8_t *buf, size_t len) = 0;
    virtual size_t write(const uint8_t *buf, size_t len) = 0;

    virtual void flush() = 0;
    virtual void close() = 0;

    virtual bool isOpen() const = 0;
};