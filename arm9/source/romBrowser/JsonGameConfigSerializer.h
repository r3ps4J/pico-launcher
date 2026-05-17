#pragma once
#include "common.h"
#include "GameConfig.h"

/// @brief JSON serializer for per-game configuration files.
class JsonGameConfigSerializer
{
public:
    static bool BuildPathForRom(const char* romPath, char* configPath, u32 configPathLength);

    void Serialize(const GameConfig* gameConfig, const char* filePath) const;
    bool Deserialize(GameConfig* gameConfig, const char* filePath) const;
};
