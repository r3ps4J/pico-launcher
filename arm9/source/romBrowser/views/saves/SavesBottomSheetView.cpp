#include "common.h"
#include "gui/GraphicsContext.h"
#include "themes/material/MaterialColorScheme.h"
#include "themes/IFontRepository.h"
#include "gui/input/InputProvider.h"
#include "gui/VramContext.h"
#include "gui/palette/GradientPalette.h"
#include "gui/OamBuilder.h"
#include "upIcon.h"
#include "checkboxChecked.h"
#include "checkboxUnchecked.h"
#include "cheatSelector.h"
#include "gui/DescendingStackVramManager.h"
#include "SavesBottomSheetView.h"

#define TITLE_LABEL_X               20
#define TITLE_LABEL_Y               16

#define NO_SAVES_FOUND_LABEL_X      20
#define NO_SAVES_FOUND_LABEL_Y      36

#define NEW_SAVE_BUTTON_X           212
#define NEW_SAVE_BUTTON_Y           (TITLE_LABEL_Y - 7)

#define LIST_X                      16
#define LIST_Y                      36
#define LIST_WIDTH                  224
#define LIST_HEIGHT                 120

SavesBottomSheetView::SavesBottomSheetView(SharedPtr<SaveManagementViewModel> viewModel,
    const MaterialColorScheme* materialColorScheme, const IFontRepository* fontRepository,
    FocusManager* focusManager)
    : _viewModel(std::move(viewModel))
    , _titleLabel(Label2DView::CreateShared(64, 16, 25, fontRepository->GetFont(FontType::Medium11)))
    , _secondaryLabel(Label2DView::CreateShared(153, 16, 64, fontRepository->GetFont(FontType::Regular10)))
    , _saveListRecycler(RecyclerView::CreateShared(
        LIST_X, LIST_Y, LIST_WIDTH, LIST_HEIGHT, RecyclerView::Mode::VerticalList))
    , _newSaveButton(IconButton2DView::CreateShared(
        IconButtonView::Type::Standard,
        IconButtonView::State::NoToggle,
        md::sys::color::inverseOnSurface,
        materialColorScheme))
    , _materialColorScheme(materialColorScheme)
    , _fontRepository(fontRepository)
    , _focusManager(focusManager)
{
    _titleLabel->SetText(u"Saves");
    _secondaryLabel->SetText(u"Loading saves...");
    _secondaryLabel->SetEllipsisStyle(LabelView::EllipsisStyle::Ellipsis);
    AddChildTail(_titleLabel.GetPointer());
    AddChildTail(_secondaryLabel.GetPointer());
    AddChildTail(_saveListRecycler.GetPointer());
    AddChildTail(_newSaveButton.GetPointer());
    _newSaveButton->SetAction([] (IconButtonView*, void* arg)
    {
        ((SavesBottomSheetView*)arg)->_viewModel->CreateNewSave();
    }, this);
}

void SavesBottomSheetView::InitVram(const VramContext& vramContext)
{
    BottomSheetView::InitVram(vramContext);

    const auto objVramManager = vramContext.GetObjVramManager();
    if (objVramManager)
    {
        _vramOffsets.checkboxUncheckedIconVramOffset
            = LoadSprite(*objVramManager, checkboxUncheckedTiles, checkboxUncheckedTilesLen);
        _vramOffsets.checkboxCheckedIconVramOffset
            = LoadSprite(*objVramManager, checkboxCheckedTiles, checkboxCheckedTilesLen);
        _vramOffsets.saveSelectorVramOffset
            = LoadSprite(*objVramManager, cheatSelectorTiles, cheatSelectorTilesLen);
        auto iconButtonVramToken = IconButton2DView::UploadGraphics(*objVramManager);
        _newSaveButton->SetGraphics(iconButtonVramToken);
        _newSaveButton->SetIconVramOffset(LoadSprite(*objVramManager, upIconTiles, upIconTilesLen));
    }

    _objVramManager = vramContext.GetObjVramManager();
}

void SavesBottomSheetView::Update()
{
    _titleLabel->SetPosition(TITLE_LABEL_X, _position.y + TITLE_LABEL_Y);
    _secondaryLabel->SetPosition(NO_SAVES_FOUND_LABEL_X, _position.y + NO_SAVES_FOUND_LABEL_Y);
    _saveListRecycler->SetPosition(LIST_X, _position.y + LIST_Y);
    _newSaveButton->SetPosition(NEW_SAVE_BUTTON_X, _position.y + NEW_SAVE_BUTTON_Y);

    if (_viewModel->GetState() == SaveManagementViewModel::State::Loading)
    {
        _secondaryLabel->SetText(u"Loading saves...");
    }
    else if (_viewModel->GetState() == SaveManagementViewModel::State::NoSaves)
    {
        _secondaryLabel->SetText(u"No saves found.");
    }

    if (_viewModel->GetState() == SaveManagementViewModel::State::DisplaySaves
        && _objVramManager != nullptr
        && (!_savesAdapter || _loadedSavesVersion != _viewModel->GetSavesVersion()))
    {
        UpdateSaveList();
    }

    BottomSheetView::Update();

    int selectedItem = _saveListRecycler->GetSelectedItem();
    if (selectedItem != _viewModel->GetSelectedItem())
    {
        _viewModel->SetSelectedItem(selectedItem);
    }
}

void SavesBottomSheetView::Draw(GraphicsContext& graphicsContext)
{
    graphicsContext.SetClipArea(GetBounds());
    u32 oldPrio = graphicsContext.SetPriority(1);
    {
        auto backColor = _materialColorScheme->GetColor(md::sys::color::surfaceContainerLow);

        if (_viewModel->GetState() == SaveManagementViewModel::State::DisplaySaves)
        {
            graphicsContext.SetClipArea(_saveListRecycler->GetBounds());
            _saveListRecycler->Draw(graphicsContext);

            graphicsContext.SetClipArea(GetBounds());

            auto maskOam = graphicsContext.GetOamManager().AllocOams(8);
            // Top
            u32 maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                GradientPalette(backColor, backColor),
                _position.y + LIST_Y - 24, _position.y + LIST_Y);
            OamBuilder::OamWithSize<64, 32>(LIST_X, _position.y + LIST_Y - 24, _vramOffsets.saveSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[0]);
            OamBuilder::OamWithSize<64, 32>(LIST_X + 64, _position.y + LIST_Y - 24, _vramOffsets.saveSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[1]);
            OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64, _position.y + LIST_Y - 24, _vramOffsets.saveSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[2]);
            OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64 + 32, _position.y + LIST_Y - 24, _vramOffsets.saveSelectorVramOffset >> 7)
                .WithPalette16(maskPaletteRow)
                .WithPriority(graphicsContext.GetPriority())
                .Build(maskOam[3]);

            // Bottom
            if (graphicsContext.IsVisible(Rectangle(LIST_X, _position.y + LIST_Y + LIST_HEIGHT, 224, 24)))
            {
                maskPaletteRow = graphicsContext.GetPaletteManager().AllocRow(
                    GradientPalette(backColor, backColor),
                    _position.y + LIST_Y + LIST_HEIGHT, 192);
                OamBuilder::OamWithSize<64, 32>(LIST_X, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.saveSelectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[4]);
                OamBuilder::OamWithSize<64, 32>(LIST_X + 64, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.saveSelectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[5]);
                OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.saveSelectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[6]);
                OamBuilder::OamWithSize<64, 32>(LIST_X + 2 * 64 + 32, _position.y + LIST_Y + LIST_HEIGHT, _vramOffsets.saveSelectorVramOffset >> 7)
                    .WithPalette16(maskPaletteRow)
                    .WithPriority(graphicsContext.GetPriority())
                    .Build(maskOam[7]);
            }
        }

        _titleLabel->SetBackgroundColor(backColor);
        _titleLabel->SetForegroundColor(_materialColorScheme->onSurface);
        _titleLabel->Draw(graphicsContext);

        if (_viewModel->GetState() != SaveManagementViewModel::State::DisplaySaves)
        {
            _secondaryLabel->SetBackgroundColor(backColor);
            _secondaryLabel->SetForegroundColor(_materialColorScheme->onSurfaceVariant);
            _secondaryLabel->Draw(graphicsContext);
        }

        _newSaveButton->Draw(graphicsContext);
    }
    graphicsContext.SetPriority(oldPrio);
    graphicsContext.ResetClipArea();
}

SharedPtr<View> SavesBottomSheetView::MoveFocus(
    const SharedPtr<View>& currentFocus, FocusMoveDirection direction, View* source)
{
    if (!currentFocus)
    {
        return nullptr;
    }

    if (source == _saveListRecycler.GetPointer() && direction == FocusMoveDirection::Up)
    {
        return _newSaveButton;
    }
    else if (source == _newSaveButton.GetPointer() && direction == FocusMoveDirection::Down
        && _viewModel->GetState() == SaveManagementViewModel::State::DisplaySaves)
    {
        return _saveListRecycler->MoveFocus(currentFocus, direction, this);
    }

    return nullptr;
}

bool SavesBottomSheetView::HandleInput(const InputProvider& inputProvider, FocusManager& focusManager)
{
    if (inputProvider.Triggered(InputKey::B) || inputProvider.Triggered(InputKey::X))
    {
        _viewModel->Close();
        return true;
    }
    return false;
}

void SavesBottomSheetView::Close()
{
    _viewModel->Close();
}

void SavesBottomSheetView::UpdateSaveList()
{
    if (!_savesAdapter)
    {
        _savedVramState = ((DescendingStackVramManager*)_objVramManager)->GetState();
    }
    else
    {
        ((DescendingStackVramManager*)_objVramManager)->SetState(_savedVramState);
    }

    _savesAdapter = SharedPtr<SavesAdapter>::MakeShared(
        _viewModel, _materialColorScheme, _fontRepository, _vramOffsets);
    _saveListRecycler->SetAdapter(_savesAdapter, _viewModel->GetActiveSaveIndex());
    _loadedSavesVersion = _viewModel->GetSavesVersion();

    _saveListRecycler->InitVram(VramContext(nullptr, _objVramManager, nullptr, nullptr));
    _saveListRecycler->Focus(*_focusManager);
}

u32 SavesBottomSheetView::LoadSprite(IVramManager& vramManager, const unsigned int* tiles, u32 tilesLength) const
{
    u32 vramOffset = vramManager.Alloc(tilesLength);
    dma_ntrCopy32(3, tiles, vramManager.GetVramAddress(vramOffset), tilesLength);
    return vramOffset;
}
