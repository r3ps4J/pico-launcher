#include "common.h"
#include <memory>
#include "fat/File.h"
#include "JsonFile.h"

#pragma GCC optimize("Os")

bool JsonFile::WritePretty(const JsonDocument& json, const char* filePath)
{
    if (json.overflowed())
        return false;

    u32 outputSize = measureJsonPretty(json);
    std::unique_ptr<u8[]> fileData(new(cache_align) u8[outputSize]);

    if (serializeJsonPretty(json, fileData.get(), outputSize) != outputSize)
        return false;

    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return false;

    u32 bytesWritten;
    if (file->Write(fileData.get(), outputSize, bytesWritten) != FR_OK || bytesWritten != outputSize)
        return false;

    return file->Sync() == FR_OK;
}

bool JsonFile::Read(DynamicJsonDocument& json, const char* filePath)
{
    const auto file = std::make_unique<File>();
    if (file->Open(filePath, FA_READ | FA_OPEN_EXISTING) != FR_OK)
        return false;

    u32 fileSize = file->GetSize();
    if (fileSize == 0)
        return false;

    std::unique_ptr<u8[]> fileData(new(cache_align) u8[fileSize]);
    u8* fileDataPtr = fileData.get();

    u32 bytesRead = 0;
    if (file->Read(fileDataPtr, fileSize, bytesRead) != FR_OK || bytesRead != fileSize)
        return false;

    return deserializeJson(json, fileDataPtr, fileSize) == DeserializationError::Ok;
}
