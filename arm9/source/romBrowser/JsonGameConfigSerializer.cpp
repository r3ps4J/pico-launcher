#include "common.h"
#include <string.h>
#include "core/StringUtil.h"
#include "json/ArduinoJson.h"
#include "json/JsonFile.h"
#include "JsonGameConfigSerializer.h"

#pragma GCC optimize("Os")

#define JSON_RESERVED_SIZE          512

#define CONFIG_PATH_SUFFIX          ".config.json"
#define KEY_SELECTED_SAVE_PATH      "selectedSavePath"

bool JsonGameConfigSerializer::BuildPathForRom(const char* romPath, char* configPath, u32 configPathLength)
{
    if (!romPath || !configPath)
        return false;

    if (strlen(romPath) + strlen(CONFIG_PATH_SUFFIX) >= configPathLength)
        return false;

    StringUtil::Copy(configPath, romPath, configPathLength);
    strlcat(configPath, CONFIG_PATH_SUFFIX, configPathLength);
    return true;
}

static void writeJson(DynamicJsonDocument& json, const GameConfig* gameConfig)
{
    json[KEY_SELECTED_SAVE_PATH] = gameConfig->GetSelectedSavePath();
}

void JsonGameConfigSerializer::Serialize(const GameConfig* gameConfig, const char* filePath) const
{
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    writeJson(json, gameConfig);

    if (!JsonFile::WritePretty(json, filePath))
    {
        LOG_ERROR("Error while writing game config file\n");
        return;
    }

    LOG_DEBUG("Game config file written\n");
}

bool JsonGameConfigSerializer::Deserialize(GameConfig* gameConfig, const char* filePath) const
{
    DynamicJsonDocument json(JSON_RESERVED_SIZE);
    if (!JsonFile::Read(json, filePath))
        return false;

    gameConfig->SetSelectedSavePath(json[KEY_SELECTED_SAVE_PATH] | "");
    return true;
}
