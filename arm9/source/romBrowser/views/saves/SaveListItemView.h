#pragma once
#include "gui/views/ViewContainer.h"
#include "gui/views/Label2DView.h"
#include "romBrowser/viewModels/SaveManagementViewModel.h"

class MaterialColorScheme;
class IFontRepository;

/// @brief List item view for the save management panel, representing a single save file.
class SaveListItemView : public ViewContainer
{
    SHARED_ONLY(SaveListItemView)

public:
    struct VramOffsets
    {
        u32 checkboxUncheckedIconVramOffset = 0;
        u32 checkboxCheckedIconVramOffset = 0;
        u32 saveSelectorVramOffset = 0;
    };

    void Update() override;
    void Draw(GraphicsContext& graphicsContext) override;
    bool HandleInput(const InputProvider& inputProvider, FocusManager& focusManager) override;
    void HandlePenDown(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenMove(const Point& touchPoint, FocusManager& focusManager) override;
    void HandlePenUp(const Point& lastTouchPoint, FocusManager& focusManager) override;

    Rectangle GetBounds() const override
    {
        return Rectangle(_position.x, _position.y, 224, 24);
    }

    void SetEntry(const SaveFileEntry* saveEntry, int index)
    {
        _saveEntry = saveEntry;
        _index = index;
        _nameLabel->SetText(saveEntry->name);
    }

private:
    SharedPtr<SaveManagementViewModel> _viewModel;
    SharedPtr<Label2DView> _nameLabel;
    VramOffsets _vramOffsets;
    const MaterialColorScheme* _materialColorScheme;
    u32 _iconVramOffset = 0;
    const SaveFileEntry* _saveEntry = nullptr;
    int _index = -1;
    bool _penDown = false;

    SaveListItemView(SharedPtr<SaveManagementViewModel> viewModel, const VramOffsets& vramOffsets,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository);
};
