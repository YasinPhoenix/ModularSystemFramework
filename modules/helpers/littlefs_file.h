#pragma once

#include "../../core/file_system/ifile.h"
#include <LittleFS.h>

class LittleFsFile : public IFile
{
public:
    LittleFsFile() = default;

    void attach(File f) { file = f; }

    bool isOpen() const override { return file; }

    bool available() override { return file.available(); }

    size_t size() override { return file.size(); }

    size_t position() override { return file.position(); }

    bool seek(size_t pos) override { return file.seek(pos); }

    size_t read(uint8_t *buf, size_t len) override { return file.read(buf, len); }

    size_t write(const uint8_t *buf, size_t len) override { return file.write(buf, len); }

    void flush() override { file.flush(); }

    void close() override { file.close(); }

private:
    File file;
};