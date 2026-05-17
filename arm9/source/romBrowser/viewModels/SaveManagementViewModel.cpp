#include "common.h"
#include <algorithm>
#include <memory>
#include <string.h>
#include "core/StringUtil.h"
#include "core/mini-printf.h"
#include "fat/File.h"
#include "romBrowser/GameConfig.h"
#include "romBrowser/JsonGameConfigSerializer.h"
#include "romBrowser/FileType/NullFileTypeProvider.h"
#include "romBrowser/SdFolderFactory.h"
#include "SaveManagementViewModel.h"

static char toLowerAscii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');

    return c;
}

static bool startsWithIgnoreCase(const char* str, const char* prefix)
{
    while (*prefix)
    {
        if (toLowerAscii(*str++) != toLowerAscii(*prefix++))
            return false;
    }

    return true;
}

static bool endsWithIgnoreCase(const char* str, const char* suffix)
{
    const u32 strLength = strlen(str);
    const u32 suffixLength = strlen(suffix);
    if (strLength < suffixLength)
        return false;

    return startsWithIgnoreCase(str + strLength - suffixLength, suffix);
}

SaveManagementViewModel::SaveManagementViewModel(
    const FileInfo& romFileInfo, IRomBrowserController* romBrowserController)
    : _romFileInfo(romFileInfo), _romBrowserController(romBrowserController)
{
    BuildRomPath();
    LoadGameConfig();
    _loadSavesTask = _romBrowserController->GetIoTaskQueue()->Enqueue([this] (const vu8& cancelRequested)
    {
        LoadSaves();
        return TaskResult<void>::Completed();
    });
}

void SaveManagementViewModel::ActivateItem(int index)
{
    if (_state != State::DisplaySaves)
        return;

    SetActiveSaveIndex(index, true);
}

void SaveManagementViewModel::CreateNewSave()
{
    if (_romPath[0] == 0 || _state == State::Loading)
        return;

    char newSavePath[256];
    newSavePath[0] = 0;
    if (TryGetNextNewSavePath(newSavePath, sizeof(newSavePath)))
    {
        const auto file = std::make_unique<File>();
        if (file->Open(newSavePath, FA_CREATE_NEW | FA_WRITE) == FR_OK)
        {
            file->Close();
            LoadSaves(newSavePath);
        }
    }
}

void SaveManagementViewModel::Close()
{
    _romBrowserController->HideSaveManagement();
}

void SaveManagementViewModel::BuildRomPath()
{
    f_getcwd(_romPath, sizeof(_romPath));
    strlcat(_romPath, "/", sizeof(_romPath));
    u32 pathLength = strlen(_romPath);
    if (pathLength >= 2 && _romPath[pathLength - 2] == '/')
    {
        _romPath[pathLength - 1] = 0;
    }
    strlcat(_romPath, _romFileInfo.GetFileName(), sizeof(_romPath));
    JsonGameConfigSerializer::BuildPathForRom(_romPath, _configPath, sizeof(_configPath));

    StringUtil::Copy(_savePathPrefix, _romPath, sizeof(_savePathPrefix));
    char* extension = strrchr(_savePathPrefix, '.');
    char* lastSlash = strrchr(_savePathPrefix, '/');
    if (extension != nullptr && (lastSlash == nullptr || extension > lastSlash))
    {
        *extension = 0;
    }

    StringUtil::Copy(_saveFileNamePrefix, _romFileInfo.GetFileName(), sizeof(_saveFileNamePrefix));
    extension = strrchr(_saveFileNamePrefix, '.');
    if (extension != nullptr)
    {
        *extension = 0;
    }

    StringUtil::Copy(_romDirectory, _romPath, sizeof(_romDirectory));
    lastSlash = strrchr(_romDirectory, '/');
    if (lastSlash == nullptr)
    {
        StringUtil::Copy(_romDirectory, ".", sizeof(_romDirectory));
    }
    else if (lastSlash == _romDirectory)
    {
        _romDirectory[1] = 0;
    }
    else
    {
        *lastSlash = 0;
    }
}

void SaveManagementViewModel::LoadGameConfig()
{
    _configSelectedSavePath[0] = 0;
    if (_configPath[0] == 0)
        return;

    GameConfig gameConfig;
    if (JsonGameConfigSerializer().Deserialize(&gameConfig, _configPath))
    {
        StringUtil::Copy(_configSelectedSavePath, gameConfig.GetSelectedSavePath(), sizeof(_configSelectedSavePath));
    }
}

void SaveManagementViewModel::LoadSaves(const char* preferredSavePath)
{
    ClearSaves();

    NullFileTypeProvider fileTypeProvider;
    auto saveFolder = SdFolderFactory(&fileTypeProvider).CreateFromPath(_romDirectory);
    if (saveFolder)
    {
        saveFolder->SortByNameInPlace();

        char defaultSaveFileName[256];
        int defaultSaveFileNameLength = mini_snprintf(
            defaultSaveFileName, sizeof(defaultSaveFileName), "%s.sav", _saveFileNamePrefix);
        if (defaultSaveFileNameLength > 0 && defaultSaveFileNameLength < (int)sizeof(defaultSaveFileName))
        {
            const auto defaultSave = saveFolder->BinarySearch(defaultSaveFileName);
            if (defaultSave != nullptr)
            {
                char savePath[256];
                MakePathForFileName(savePath, sizeof(savePath), defaultSave->GetFileName());
                AddSave(defaultSave->GetFileName(), savePath);
            }
        }

        const FileInfo* const* files = saveFolder->GetFiles();
        for (int i = 0; i < saveFolder->GetFileCount(); i++)
        {
            if (files[i]->GetFileType()->GetClassification() == FileTypeClassification::Folder
                || !IsExtraSaveFileName(files[i]->GetFileName()))
            {
                continue;
            }

            char savePath[256];
            MakePathForFileName(savePath, sizeof(savePath), files[i]->GetFileName());
            AddSave(files[i]->GetFileName(), savePath);
        }
    }

    SortSaves();
    SelectInitialSave(preferredSavePath);
    _state = _saveCount != 0 ? State::DisplaySaves : State::NoSaves;
    _savesVersion++;
}

void SaveManagementViewModel::AddSave(const char* name, const char* path)
{
    if (_saveCount == _saveCapacity)
    {
        u32 newCapacity = _saveCapacity == 0 ? 4 : _saveCapacity * 2;
        auto newSaves = std::make_unique_for_overwrite<SaveFileEntry[]>(newCapacity);
        if (_saves)
        {
            memcpy(newSaves.get(), _saves.get(), _saveCount * sizeof(SaveFileEntry));
        }
        _saves = std::move(newSaves);
        _saveCapacity = newCapacity;
    }

    auto& save = _saves[_saveCount++];
    StringUtil::Copy(save.name, name, sizeof(save.name));
    StringUtil::Copy(save.path, path, sizeof(save.path));
    save.isActive = false;
}

void SaveManagementViewModel::ClearSaves()
{
    _saves.reset();
    _saveCount = 0;
    _saveCapacity = 0;
    _selectedItem = -1;
    _activeSaveIndex = -1;
}

void SaveManagementViewModel::SortSaves()
{
    if (_saveCount <= 2)
        return;

    std::sort(_saves.get() + 1, _saves.get() + _saveCount,
        [] (const SaveFileEntry& a, const SaveFileEntry& b)
        {
            return strcasecmp(a.name, b.name) < 0;
        });
}

void SaveManagementViewModel::SelectInitialSave(const char* preferredSavePath)
{
    int selectedIndex = -1;
    const char* controllerSelectedPath = _romBrowserController->GetSelectedSavePathForGame(_romPath);
    const char* selectedPath = preferredSavePath != nullptr && preferredSavePath[0] != 0
        ? preferredSavePath
        : controllerSelectedPath != nullptr && controllerSelectedPath[0] != 0
            ? controllerSelectedPath
            : _configSelectedSavePath[0] != 0
                ? _configSelectedSavePath
                : nullptr;

    if (selectedPath != nullptr && selectedPath[0] != 0)
    {
        for (u32 i = 0; i < _saveCount; i++)
        {
            if (!strcasecmp(_saves[i].path, selectedPath))
            {
                selectedIndex = i;
                break;
            }
        }
    }

    if (selectedIndex < 0 && _saveCount != 0)
    {
        selectedIndex = 0;
    }

    if (selectedIndex >= 0)
    {
        SetActiveSaveIndex(selectedIndex, preferredSavePath != nullptr && preferredSavePath[0] != 0);
    }
    else
    {
        _romBrowserController->SetSelectedSavePathForGame(_romPath, "");
    }
}

void SaveManagementViewModel::SetActiveSaveIndex(int index, bool saveGameConfig)
{
    if (index < 0 || index >= (int)_saveCount)
        return;

    for (u32 i = 0; i < _saveCount; i++)
    {
        _saves[i].isActive = i == (u32)index;
    }
    _activeSaveIndex = index;
    _selectedItem = index;
    _romBrowserController->SetSelectedSavePathForGame(_romPath, _saves[index].path);
    if (saveGameConfig)
    {
        SaveGameConfig();
    }
}

void SaveManagementViewModel::SaveGameConfig()
{
    if (_configPath[0] == 0 || _activeSaveIndex < 0)
        return;

    GameConfig gameConfig;
    gameConfig.SetSelectedSavePath(_saves[_activeSaveIndex].path);
    JsonGameConfigSerializer().Serialize(&gameConfig, _configPath);
    StringUtil::Copy(_configSelectedSavePath, gameConfig.GetSelectedSavePath(), sizeof(_configSelectedSavePath));
}

bool SaveManagementViewModel::TryGetNextNewSavePath(char* savePath, u32 savePathLength) const
{
    for (u32 i = 1; i != 0; i++)
    {
        int length = mini_snprintf(savePath, savePathLength, "%s.%u.sav", _savePathPrefix, i);
        if (length <= 0 || length >= (int)savePathLength)
            return false;

        FILINFO fileInfo;
        if (f_stat(savePath, &fileInfo) != FR_OK)
            return true;
    }

    return false;
}

bool SaveManagementViewModel::IsExtraSaveFileName(const char* fileName) const
{
    const u32 saveFileNamePrefixLength = strlen(_saveFileNamePrefix);
    const u32 fileNameLength = strlen(fileName);
    if (fileNameLength <= saveFileNamePrefixLength + 5)
        return false;

    return startsWithIgnoreCase(fileName, _saveFileNamePrefix)
        && fileName[saveFileNamePrefixLength] == '.'
        && endsWithIgnoreCase(fileName, ".sav");
}

void SaveManagementViewModel::MakePathForFileName(char* path, u32 pathLength, const char* fileName) const
{
    StringUtil::Copy(path, _romDirectory, pathLength);
    strlcat(path, "/", pathLength);
    u32 length = strlen(path);
    if (length >= 2 && path[length - 2] == '/')
    {
        path[length - 1] = 0;
    }
    strlcat(path, fileName, pathLength);
}
