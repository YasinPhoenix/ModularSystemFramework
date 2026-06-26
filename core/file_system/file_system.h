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