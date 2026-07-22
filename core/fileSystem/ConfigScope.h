#pragma once

#include "IFileSystem.h"

class IModule;

typedef void (*ConfigHandler)(void *context, const char *value);

struct ConfigField
{
    const char *key;
    ConfigHandler apply;
};

class ConfigScope
{
private:
    char dir[64];
    char itemsListDir[64 + 11];
    IFileSystem *fs;

    bool initialized = false;

    size_t readFile(const char *path)
    {
        if (!path)
            return false;

        IFile *file = fs->open(path, FileMode::Read);
        if (!file)
            return 0;

        size_t count = 0;

        while (file->available() && count < 20)
        {
            size_t len = file->readBytesUntil('\n', items[count], 32 - 1);

            items[count][len] = '\0';

            // Remove trailing '\r' if the file uses Windows line endings
            if (len > 0 && items[count][len - 1] == '\r')
            {
                items[count][len - 1] = '\0';
            }

            count++;
        }

        file->close();
        delete file;

        return count;
    }

    bool addItem(const char *item, const char *path)
    {
        if (!item || !path)
            return false;

        // Check RAM first
        for (size_t i = 0; i < itemCount; i++)
        {
            if (strcmp(items[i], item) == 0)
                return false; // Already exists
        }

        // Append to file
        IFile *file = fs->open(path, FileMode::Append);
        if (!file)
            return false;

        char itemWithNL[33];
        snprintf(itemWithNL, sizeof(itemWithNL), "%s\n", item);

        file->write((const uint8_t *)itemWithNL, strlen(itemWithNL));
        file->close();
        delete file;

        // Update RAM
        strncpy(items[itemCount], item, 32 - 1);
        items[itemCount][32 - 1] = '\0';
        itemCount++;

        return true;
    }

public:
    char items[20][32];
    size_t itemCount = 0;

    bool init(const char *name, IFileSystem *fs, char *result = nullptr)
    {
        if (!name)
        {
            if (result)
                strncpy(result, "No name was given to the config scope initiation!", 128);
            return false;
        }

        snprintf(this->dir, sizeof(this->dir), "/%s", name);

        snprintf(this->itemsListDir, sizeof(this->itemsListDir), "/%s/configured", name);

        if (!fs)
        {
            if (result)
                strncpy(result, "File system is null", 128);
            return false;
        }

        this->fs = fs;

        if (!fs->mounted())
        {
            if (result)
                strncpy(result, "File system is not mounted", 128);
            return false;
        }

        if (!fs->exists(dir))
        {
            if (!fs->mkdir(dir))
            {
                if (result)
                    strncpy(result, "Failed to create config directory", 128);
                return false;
            }
        }
        else
            itemCount = readFile(itemsListDir);

        if (result)
            strncpy(result, "Successfully initialized config scope!", 128);

        initialized = true;
        return true;
    }

    bool set(const char *key, const char *value, char *result = nullptr)
    {
        if (!initialized)
        {
            if (result)
                strncpy(result, "Config scope not initialized", 128);
            return false;
        }

        if (!fs)
        {
            if (result)
                strncpy(result, "File system is null", 128);
            return false;
        }

        if (!key || !value)
        {
            if (result)
                strncpy(result, "Null pointers were passed to ConfigScope.set()", 128);
            return false;
        }

        char path[128];
        snprintf(path, sizeof(path), "%s/%s", dir, key);

        const char *previousValue = get(key);
        if (previousValue && strcmp(value, previousValue) == 0)
        {
            if (result)
                strncpy(result, "Didn't need to change the value on the file system!", 128);
            return true;
        }

        IFile *file = fs->open(path, FileMode::Write);
        if (!file)
        {
            if (result)
                strncpy(result, "Failed to open config file for writing", 128);
            return false;
        }

        size_t written = file->write((const uint8_t *)value, strlen(value));
        file->close();
        delete file;

        if (written != strlen(value))
        {
            if (result)
                snprintf(result, 128, "Failed to write complete value to config file: %s/%s", path, key);

            return false;
        }

        if (!addItem(key, itemsListDir))
            if (result)
                snprintf(result, 128, "Failed to write key to configured itms: %s", key);

        return true;
    }

    bool get(const char *key, char *value, size_t valueSize, char *result = nullptr)
    {
        if (!initialized)
        {
            if (result)
                strncpy(result, "Config scope not initialized", 128);
            return false;
        }

        if (!fs)
        {
            if (result)
                strncpy(result, "File system is null", 128);
            return false;
        }

        if (!value)
        {
            if (result)
                strncpy(result, "Null pointers were passed to ConfigScope.get()", 128);
            return false;
        }

        char path[128];
        snprintf(path, sizeof(path), "%s/%s", dir, key);

        IFile *file = fs->open(path, FileMode::Read);
        if (!file)
        {
            if (result)
                snprintf(result, 128, "Failed to open config file for reading: %s", path);
            return false;
        }

        size_t bytesRead = file->read((uint8_t *)value, valueSize - 1);
        value[bytesRead] = '\0'; // Null-terminate the string
        file->close();
        delete file;

        if (bytesRead == 0 && !fs->exists(path))
        {
            if (result)
                strncpy(result, "Config file does not exist", 128);
            return false;
        }

        return true;
    }

    const char *get(const char *key, char *result = nullptr)
    {
        static char value[256];
        if (!get(key, value, sizeof(value), result))
            return nullptr;
        return value;
    }

    bool getBatch(const char *keys[], size_t keyCount, char values[][128], char *result = nullptr)
    {
        if (!initialized)
        {
            if (result)
                strncpy(result, "Config scope not initialized", 128);
            return false;
        }

        if (!keys)
        {
            if (result)
                strncpy(result, "Null pointers were passed to ConfigScope.set()", 128);
            return false;
        }

        for (size_t i = 0; i < keyCount; i++)
        {
            if (!get(keys[i], values[i], 128, result))
            {
                values[i][0] = '\0'; // Clear the value on failure
            }
        }

        return true;
    }
};
