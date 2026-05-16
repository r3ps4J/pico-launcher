#pragma once
#include "gui/views/RecyclerAdapter.h"
#include "SaveListItemView.h"

/// @brief Recycler adapter for save files.
class SavesAdapter : public RecyclerAdapter
{
public:
    SavesAdapter(SharedPtr<SaveManagementViewModel> saveManagementViewModel,
        const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
        const SaveListItemView::VramOffsets& vramOffsets)
        : _saveManagementViewModel(std::move(saveManagementViewModel))
        , _materialColorScheme(materialColorScheme), _fontRepository(fontRepository), _vramOffsets(vramOffsets) { }

    u32 GetItemCount() const override
    {
        return _saveManagementViewModel->GetSaveCount();
    }

    void GetViewSize(int& width, int& height) const override
    {
        width = 224;
        height = 24;
    }

    SharedPtr<View> CreateView() const override
    {
        return SaveListItemView::CreateShared(
            _saveManagementViewModel, _vramOffsets, _materialColorScheme, _fontRepository);
    }

    void BindView(SharedPtr<View> view, int index) const override
    {
        auto listItemView = static_cast<SaveListItemView*>(view.GetPointer());
        listItemView->SetEntry(&_saveManagementViewModel->GetSave(index), index);
    }

    void ReleaseView(SharedPtr<View> view, int index) const override
    {
        // Nothing to do
    }

private:
    SharedPtr<SaveManagementViewModel> _saveManagementViewModel;
    const MaterialColorScheme* _materialColorScheme;
    const IFontRepository* _fontRepository;
    SaveListItemView::VramOffsets _vramOffsets;
};
