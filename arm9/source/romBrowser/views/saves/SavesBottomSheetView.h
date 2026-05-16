#pragma once
#include <memory>
#include "core/SharedPtr.h"
#include "romBrowser/views/BottomSheetView.h"
#include "gui/views/Label2DView.h"
#include "gui/views/RecyclerView.h"
#include "romBrowser/viewModels/SaveManagementViewModel.h"
#include "romBrowser/views/IconButton2DView.h"
#include "SavesAdapter.h"
#include "SaveListItemView.h"

class MaterialColorScheme;
class IFontRepository;
class IVramManager;

/// @brief Bottom sheet for browsing and selecting save files.
class SavesBottomSheetView : public BottomSheetView
{
    SHARED_ONLY(SavesBottomSheetView)

public:
    void InitVram(const VramContext& vramContext) override;
    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    SharedPtr<View> MoveFocus(const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;

    void Focus(FocusManager& focusManager) override
    {
        if (_viewModel->GetState() == SaveManagementViewModel::State::DisplaySaves
            && _viewModel->GetSaveCount() != 0)
        {
            _saveListRecycler->Focus(focusManager);
        }
        else
        {
            focusManager.Focus(_newSaveButton);
        }
    }

protected:
    void Close() override;

private:
    SharedPtr<SaveManagementViewModel> _viewModel;
    SharedPtr<Label2DView> _titleLabel;
    SharedPtr<Label2DView> _secondaryLabel;
    SharedPtr<RecyclerView> _saveListRecycler;
    SharedPtr<SavesAdapter> _savesAdapter;
    SharedPtr<IconButton2DView> _newSaveButton;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    IVramManager* _objVramManager = nullptr;
    FocusManager* _focusManager;
    SaveListItemView::VramOffsets _vramOffsets;
    u32 _savedVramState = 0;
    u32 _loadedSavesVersion = 0;

    SavesBottomSheetView(SharedPtr<SaveManagementViewModel> viewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        FocusManager* focusManager);

    void UpdateSaveList();
    u32 LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const;
};
