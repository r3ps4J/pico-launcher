#pragma once
#include "common.h"
#include "ArduinoJson.h"

class JsonFile
{
public:
    static bool WritePretty(const JsonDocument& json, const char* filePath);
    static bool Read(DynamicJsonDocument& json, const char* filePath);
};
