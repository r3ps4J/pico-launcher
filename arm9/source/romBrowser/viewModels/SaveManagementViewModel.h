#pragma once
#include <memory>
#include "core/task/TaskQueue.h"
#include "romBrowser/FileInfo.h"
#include "romBrowser/IRomBrowserController.h"

struct SaveFileEntry
{
    char name[256];
    char path[256];
    bool isActive;
};

/// @brief View model for the save management screen.
class SaveManagementViewModel
{
public:
    /// @brief Enum representing the state of the save management panel.
    enum class State
    {
        /// @brief Saves are being loaded.
        Loading,

        /// @brief No saves were found.
        NoSaves,

        /// @brief Saves are being displayed.
        DisplaySaves
    };

    SaveManagementViewModel(const FileInfo& romFileInfo, IRomBrowserController* romBrowserController);

    /// @brief Selects the save at the specified \p index.
    void ActivateItem(int index);

    /// @brief Creates a new numbered save file.
    void CreateNewSave();

    /// @brief Closes the save management panel.
    void Close();

    /// @brief Gets the current state of the save management panel.
    /// @return The current state of the save management panel.
    State GetState() const { return _state; }

    /// @brief Gets the number of discovered save files.
    /// @return The number of discovered save files.
    u32 GetSaveCount() const { return _saveCount; }

    /// @brief Gets the save at the specified \p index.
    /// @param index The index of the save to get.
    /// @return The save at the specified \p index.
    const SaveFileEntry& GetSave(int index) const { return _saves[index]; }

    /// @brief Gets the active save index.
    /// @return The active save index, or -1 when there is no active save.
    constexpr int GetActiveSaveIndex() const { return _activeSaveIndex; }

    /// @brief Gets the index of the selected item.
    /// @return The index of the selected item.
    constexpr int GetSelectedItem() const { return _selectedItem; }

    /// @brief Sets the index of the selected item.
    /// @param selectedItem The index of the selected item to set.
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    /// @brief Gets a version that changes when the save list is reloaded.
    /// @return The save list version.
    constexpr u32 GetSavesVersion() const { return _savesVersion; }

private:
    FileInfo _romFileInfo;
    IRomBrowserController* _romBrowserController;
    QueueTask<void> _loadSavesTask;
    std::unique_ptr<SaveFileEntry[]> _saves;
    u32 _saveCount = 0;
    u32 _saveCapacity = 0;
    State _state = State::Loading;
    int _selectedItem = -1;
    int _activeSaveIndex = -1;
    u32 _savesVersion = 0;
    char _romPath[256] = { 0 };
    char _romDirectory[256] = { 0 };
    char _savePathPrefix[256] = { 0 };
    char _saveFileNamePrefix[256] = { 0 };
    char _configPath[256] = { 0 };
    char _configSelectedSavePath[256] = { 0 };

    void BuildRomPath();
    void LoadGameConfig();
    void LoadSaves(const char* preferredSavePath = nullptr);
    void AddSave(const char* name, const char* path);
    void ClearSaves();
    void SortSaves();
    void SelectInitialSave(const char* preferredSavePath);
    void SetActiveSaveIndex(int index, bool saveGameConfig);
    void SaveGameConfig();
    bool TryGetNextNewSavePath(char* savePath, u32 savePathLength) const;
    bool IsExtraSaveFileName(const char* fileName) const;
    void MakePathForFileName(char* path, u32 pathLength, const char* fileName) const;
};
