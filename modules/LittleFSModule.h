#pragma once

#include "../core/fileSystem/IFileSystem.h"
#include "common/LittleFSCommon.h"
#include <LittleFS.h>

class LittleFsModule : public IFileSystem
{
public:
    const char *name() override { return "LittleFS"; }

    bool init(System *sys) override
    {
        isMounted = LittleFS.begin(true); // true = format if failed
        this->sys = sys;

        LOG_DEBUG(sys, "LittleFS initialized!", SRC_LITTLEFS);
        return isMounted;
    }

    void update() override {}

    bool mounted() const override { return isMounted; }

    bool format() override
    {
        isMounted = LittleFS.format();

        if (isMounted)
            isMounted = LittleFS.begin();

        return isMounted;
    }

    bool exists(const char *path) override { return LittleFS.exists(path); }
    bool remove(const char *path) override { return LittleFS.remove(path); }
    bool rename(const char *from, const char *to) override { return LittleFS.rename(from, to); }

    bool mkdir(const char *path) override { return LittleFS.mkdir(path); }
    bool rmdir(const char *path) override { return LittleFS.rmdir(path); }

    IFile *open(const char *path, FileMode mode) override
    {
        const char *m = "r";

        switch (mode)
        {
        case FileMode::Read:
            m = "r";
            break;
        case FileMode::Write:
            m = "w";
            break;
        case FileMode::Append:
            m = "a";
            break;
        case FileMode::ReadWrite:
            m = "r+";
            break;
        }

        File f = LittleFS.open(path, m);

        if (!f)
            return nullptr;

        auto *wrapper = new LittleFsFile();
        wrapper->attach(f);

        return wrapper;
    }

    size_t totalBytes() const override { return LittleFS.totalBytes(); }
    size_t usedBytes() const override { return LittleFS.usedBytes(); }

    void list(const char *path, void (*callback)(const FileInfo &, void *), void *ctx) override
    {
        File dir = LittleFS.open(path);

        if (!dir || !dir.isDirectory())
            return;

        File entry = dir.openNextFile();

        while (entry)
        {
            FileInfo info;
            info.name = entry.name();
            info.size = entry.size();
            info.isDirectory = entry.isDirectory();

            callback(info, ctx);

            entry = dir.openNextFile();
        }
    }

private:
    System *sys;
    bool isMounted = false;
};