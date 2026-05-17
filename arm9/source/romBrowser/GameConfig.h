#pragma once

/// @brief Per-game configuration stored next to a ROM.
class GameConfig
{
public:
    const char* GetSelectedSavePath() const { return _selectedSavePath; }
    void SetSelectedSavePath(const char* selectedSavePath);

private:
    char _selectedSavePath[256] = { 0 };
};
