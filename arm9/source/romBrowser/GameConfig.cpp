#include "common.h"
#include "core/StringUtil.h"
#include "GameConfig.h"

void GameConfig::SetSelectedSavePath(const char* selectedSavePath)
{
    StringUtil::Copy(_selectedSavePath, selectedSavePath, sizeof(_selectedSavePath));
}
