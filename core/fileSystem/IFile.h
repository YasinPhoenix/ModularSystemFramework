#pragma once

enum class FileMode
{
    Read,
    Write,
    Append,
    ReadWrite
};

struct FileInfo
{
    const char *name;
    size_t size;
    bool isDirectory;
};

class IFile
{
public:
    virtual ~IFile() = default;

    virtual bool available() = 0;
    virtual size_t size() = 0;
    virtual size_t position() = 0;
    virtual bool seek(size_t pos) = 0;

    virtual size_t read(uint8_t *buf, size_t len) = 0;
    virtual size_t readBytesUntil(char terminator, char *buffer, size_t length) = 0;
    virtual size_t write(const uint8_t *buf, size_t len) = 0;

    virtual void flush() = 0;
    virtual void close() = 0;

    virtual bool isOpen() const = 0;
};