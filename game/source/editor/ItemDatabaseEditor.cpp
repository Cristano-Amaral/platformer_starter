#include "editor/ItemDatabaseEditor.h"

#include "editor/LevelEditor.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)
#include "editor/AuthoringPaths.h"
#include "editor/ContentBrowser.h"
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
#include "imgui.h"
#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/StaticModelThumbnailCache.h"
#include "render/StaticModelThumbnail.h"
#include "render/TextureThumbnail.h"
#endif
#endif

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace editor
{
#if defined(PLATFORMER_ENABLE_DEBUG_UI) && defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
namespace
{
constexpr float kItemPickerThumbSize = 64.0f;

void DrawItemModelThumbnail(
    const LevelEditorViewContext& view,
    std::string_view identity,
    float thumbSize,
    bool missing)
{
    unsigned int gpuId = 0;
    if (!missing && gameplay::IsValidItemWorldModelIdentity(identity) && view.thumbnails != nullptr)
    {
        const std::filesystem::path sourceRoot = AuthoringSourceRoot();
        if (!sourceRoot.empty())
        {
            view.thumbnails->Ensure(identity, sourceRoot / std::string(identity), ThumbnailCacheRoot());
        }
        gpuId = view.thumbnails->TextureGpuId(identity);
        if (view.thumbnails->IsFailed(identity))
        {
            missing = true;
        }
    }
    const ImVec2 dummyMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(thumbSize, thumbSize));
    const ImVec2 dummyMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(
        dummyMin,
        dummyMax,
        missing ? IM_COL32(88, 64, 64, 255) : IM_COL32(48, 52, 62, 255));
    ImGui::GetWindowDrawList()->AddRect(dummyMin, dummyMax, IM_COL32(120, 126, 140, 255));
    if (gpuId != 0)
    {
        const float inset = 2.0f;
        ImGui::SetCursorScreenPos(ImVec2(dummyMin.x + inset, dummyMin.y + inset));
        ImGui::Image(
            ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(gpuId))),
            ImVec2(thumbSize - inset * 2.0f, thumbSize - inset * 2.0f));
        ImGui::SetCursorScreenPos(ImVec2(dummyMin.x, dummyMax.y));
    }
}

void DrawItemIconThumbnail(
    const LevelEditorViewContext& view,
    std::string_view identity,
    float thumbSize,
    bool missing)
{
    unsigned int gpuId = 0;
    if (!missing && gameplay::IsValidItemIconTextureIdentity(identity)
        && view.textureThumbnails != nullptr)
    {
        const std::filesystem::path sourceRoot = AuthoringSourceRoot();
        if (!sourceRoot.empty())
        {
            view.textureThumbnails->Ensure(identity, sourceRoot / std::string(identity));
        }
        gpuId = view.textureThumbnails->TextureGpuId(identity);
        if (view.textureThumbnails->IsFailed(identity))
        {
            missing = true;
        }
    }
    const ImVec2 dummyMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(thumbSize, thumbSize));
    const ImVec2 dummyMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(
        dummyMin,
        dummyMax,
        missing ? IM_COL32(88, 64, 64, 255) : IM_COL32(48, 52, 62, 255));
    ImGui::GetWindowDrawList()->AddRect(dummyMin, dummyMax, IM_COL32(120, 126, 140, 255));
    if (gpuId != 0)
    {
        const float inset = 2.0f;
        ImGui::SetCursorScreenPos(ImVec2(dummyMin.x + inset, dummyMin.y + inset));
        ImGui::Image(
            ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(gpuId))),
            ImVec2(thumbSize - inset * 2.0f, thumbSize - inset * 2.0f));
        ImGui::SetCursorScreenPos(ImVec2(dummyMin.x, dummyMax.y));
    }
}

bool DrawItemAssetPickerPopup(
    ItemAssetPickerState& picker,
    const char* popupId,
    const char* heading,
    const std::vector<ItemAssetPickerItem>& catalogItems,
    const LevelEditorViewContext& view,
    bool isModel,
    std::string& assignedIdentity)
{
    bool visible = false;
    assignedIdentity.clear();
    ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f), ImGuiCond_Appearing);
    if (ImGui::BeginPopup(popupId))
    {
        visible = true;
        ImGui::TextUnformatted(heading);
        char query[128];
        std::snprintf(query, sizeof(query), "%s", picker.filter.c_str());
        if (ImGui::InputText("Search", query, sizeof(query)))
        {
            picker.filter = query;
        }
        const std::vector<ItemAssetPickerItem> filtered =
            FilterItemAssetPickerItems(catalogItems, picker.filter);
        if (filtered.empty())
        {
            ImGui::TextWrapped(
                "%s",
                ItemAssetPickerEmptyStatusText(picker.kind, catalogItems.empty()));
        }
        ImGui::BeginChild("##itemPickerList", ImVec2(0.0f, 0.0f), true);
        const float pickerWidth = ImGui::GetContentRegionAvail().x;
        const int columns =
            ComputeContentBrowserThumbnailColumns(pickerWidth, kItemPickerThumbSize, 8.0f);
        if (ImGui::BeginTable(
                "itemPickerGrid",
                columns,
                ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoPadOuterX))
        {
            int column = 0;
            for (const ItemAssetPickerItem& item : filtered)
            {
                if (column == 0)
                {
                    ImGui::TableNextRow();
                }
                ImGui::TableSetColumnIndex(column);
                ImGui::PushID(item.canonicalIdentity.c_str());
                ImGui::BeginGroup();
                if (ImGui::InvisibleButton(
                        "##pick",
                        ImVec2(
                            kItemPickerThumbSize,
                            TerrainPickerCardHeight(
                                kItemPickerThumbSize, ImGui::GetTextLineHeight()))))
                {
                    assignedIdentity = item.canonicalIdentity;
                    ImGui::CloseCurrentPopup();
                }
                const ImVec2 cellMin = ImGui::GetItemRectMin();
                ImGui::SetCursorScreenPos(cellMin);
                if (isModel)
                {
                    DrawItemModelThumbnail(view, item.canonicalIdentity, kItemPickerThumbSize, false);
                }
                else
                {
                    DrawItemIconThumbnail(view, item.canonicalIdentity, kItemPickerThumbSize, false);
                }
                ImGui::SetCursorScreenPos(
                    ImVec2(cellMin.x, cellMin.y + kItemPickerThumbSize));
                ImGui::PushTextWrapPos(cellMin.x + kItemPickerThumbSize);
                ImGui::TextUnformatted(item.displayName.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndGroup();
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s\n%s", item.displayName.c_str(), item.canonicalIdentity.c_str());
                }
                ImGui::PopID();
                column = (column + 1) % columns;
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    NoteItemAssetPickerOpen(picker, visible);
    return !assignedIdentity.empty();
}
}

void DrawItemDatabaseEditor(
    ItemDatabaseEditorState& state,
    const LevelEditorViewContext& view,
    const assets::StaticModelCatalog& modelCatalog,
    const assets::SourceTextureCatalog& textureCatalog,
    bool authoringAvailable,
    bool* open)
{
    if (open == nullptr || !*open)
    {
        state.modelPicker.open = false;
        state.iconPicker.open = false;
        TickItemDatabasePickerPointerLocks(state, false);
        return;
    }

    ApplyKnownEditorWindowPlacement(
        kItemDatabaseWindowName, view.viewportWidth, view.viewportHeight, view.forceDefaultLayout);
    if (!ImGui::Begin(kItemDatabaseWindowName, open))
    {
        RecoverKnownEditorWindowIfOffscreen(
            kItemDatabaseWindowName,
            view.viewportWidth,
            view.viewportHeight,
            view.recoverOffscreenLayout);
        ImGui::End();
        TickItemDatabasePickerPointerLocks(state, ImGui::IsMouseDown(ImGuiMouseButton_Left));
        return;
    }
    RecoverKnownEditorWindowIfOffscreen(
        kItemDatabaseWindowName,
        view.viewportWidth,
        view.viewportHeight,
        view.recoverOffscreenLayout);

    const std::filesystem::path path =
        AuthoringSourcePath(gameplay::kGameplayDefinitionsLogicalPath);
    if (!state.loaded && authoringAvailable && !path.empty())
    {
        ApplyLoadedItemDatabase(state, gameplay::LoadGameplayDefinitionsFile(path));
    }

    ImGui::TextUnformatted(
        "Reusable Item definitions. Does not drive Inventory, Item Pickup, or Player.");
    ImGui::Text("Source: %s", path.empty() ? "(unavailable)" : path.string().c_str());
    ImGui::Text("Status: %s%s", state.dirty ? "Dirty — " : "", state.statusMessage.c_str());

    ImGui::BeginDisabled(!ItemDatabaseCanSave(state, authoringAvailable));
    if (ImGui::Button("Save"))
    {
        TrySaveItemDatabase(state, path, authoringAvailable);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!authoringAvailable);
    if (ImGui::Button("Reload"))
    {
        TryReloadItemDatabase(state, path, authoringAvailable, false);
    }
    ImGui::EndDisabled();
    if (state.reloadConfirmOpen)
    {
        ImGui::OpenPopup("Discard Item Database edits?");
        state.reloadConfirmOpen = false;
    }
    if (ImGui::BeginPopupModal(
            "Discard Item Database edits?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("Reload discards unsaved Item Database edits. Continue?");
        if (ImGui::Button("Discard and Reload"))
        {
            TryReloadItemDatabase(state, path, authoringAvailable, true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();
    char search[128];
    std::snprintf(search, sizeof(search), "%s", state.searchFilter.c_str());
    if (ImGui::InputText("Search Items", search, sizeof(search)))
    {
        state.searchFilter = search;
    }

    const std::vector<std::string> filtered =
        FilterItemDatabaseIdentities(state.working, state.searchFilter);
    const char* searchStatus = ItemDatabaseSearchStatusText(state.working, filtered);
    if (searchStatus[0] != '\0')
    {
        ImGui::TextWrapped("%s", searchStatus);
    }

    ImGui::BeginChild("##itemDatabaseSplit", ImVec2(0.0f, -110.0f), false);
    ImGui::BeginChild("##itemList", ImVec2(260.0f, 0.0f), true);
    for (const std::string& identity : filtered)
    {
        const gameplay::GameplayDefinition* definition = state.working.Find(identity);
        if (definition == nullptr)
        {
            continue;
        }
        ImGui::PushID(identity.c_str());
        const bool selected = state.selectedIdentity == identity;
        char label[160];
        std::snprintf(
            label,
            sizeof(label),
            "%s\n%s",
            identity.c_str(),
            definition->item.displayName.c_str());
        if (ImGui::Selectable(label, selected, 0, ImVec2(0.0f, ImGui::GetTextLineHeight() * 2.2f)))
        {
            TrySelectItemDefinition(state, identity);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("##itemInspector", ImVec2(0.0f, 0.0f), true);

    gameplay::GameplayDefinition* selected = state.working.FindMutable(state.selectedIdentity);
    if (selected == nullptr || selected->category != gameplay::GameplayDefinitionCategory::Item)
    {
        ImGui::TextUnformatted("Select or create an Item.");
    }
    else
    {
        ImGui::Text("Identity: %s", selected->identity.c_str());
        ImGui::TextUnformatted("Identity stays unique. Rename is explicit and does not migrate references.");
        char rename[gameplay::kMaxGameplayDefinitionNameLength + 1];
        std::snprintf(rename, sizeof(rename), "%s", state.renameName.c_str());
        if (ImGui::InputText("Rename leaf", rename, sizeof(rename)))
        {
            state.renameName = rename;
            state.renameError = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Rename"))
        {
            const RenameItemDefinitionStatus renamed =
                TryRenameSelectedItemIdentity(state, state.renameName);
            state.renameError = renamed != RenameItemDefinitionStatus::Renamed
                && renamed != RenameItemDefinitionStatus::Unchanged;
            if (renamed == RenameItemDefinitionStatus::Renamed)
            {
                state.statusMessage = "Renamed";
            }
            else if (renamed == RenameItemDefinitionStatus::DuplicateIdentity)
            {
                state.statusMessage = "duplicate identity";
            }
            else if (renamed == RenameItemDefinitionStatus::MalformedIdentity)
            {
                state.statusMessage = "malformed identity";
            }
            selected = state.working.FindMutable(state.selectedIdentity);
        }
        if (state.renameError)
        {
            ImGui::TextColored(ImVec4(0.92f, 0.42f, 0.32f, 1.0f), "%s", state.statusMessage.c_str());
        }

        if (selected != nullptr && selected->category == gameplay::GameplayDefinitionCategory::Item)
        {
            char displayName[gameplay::kMaxItemDisplayNameLength + 1];
            std::snprintf(
                displayName, sizeof(displayName), "%s", selected->item.displayName.c_str());
            if (ImGui::InputText("Display Name", displayName, sizeof(displayName)))
            {
                selected->item.displayName = displayName;
                RefreshItemDatabaseDirty(state);
            }
            if (!gameplay::IsValidItemDisplayName(selected->item.displayName))
            {
                ImGui::TextColored(
                    ImVec4(0.92f, 0.42f, 0.32f, 1.0f),
                    "Display Name must be 1-%d printable ASCII characters without quotes.",
                    static_cast<int>(gameplay::kMaxItemDisplayNameLength));
            }

            char description[gameplay::kMaxItemDescriptionLength + 1];
            std::snprintf(
                description, sizeof(description), "%s", selected->item.description.c_str());
            if (ImGui::InputTextMultiline(
                    "Description", description, sizeof(description), ImVec2(-1.0f, 60.0f)))
            {
                selected->item.description = description;
                RefreshItemDatabaseDirty(state);
            }
            if (!gameplay::IsValidItemDescription(selected->item.description))
            {
                ImGui::TextColored(
                    ImVec4(0.92f, 0.42f, 0.32f, 1.0f), "Description is invalid.");
            }

            int typeIndex = static_cast<int>(selected->item.type);
            const char* typeNames[] = {"Generic", "Consumable", "Equipment", "Key", "Quest"};
            if (ImGui::Combo("Item Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames)))
            {
                if (typeIndex >= 0 && typeIndex < static_cast<int>(gameplay::kItemTypeCount))
                {
                    selected->item.type = static_cast<gameplay::ItemType>(typeIndex);
                    if (selected->item.type == gameplay::ItemType::Equipment)
                    {
                        if (!selected->item.equipmentSlot.has_value())
                        {
                            selected->item.equipmentSlot = gameplay::EquipmentSlot::Head;
                        }
                    }
                    else
                    {
                        selected->item.equipmentSlot.reset();
                    }
                    RefreshItemDatabaseDirty(state);
                }
            }
            ImGui::TextDisabled("Item Type is classification. Equipment requires a Slot.");

            if (selected->item.type == gameplay::ItemType::Equipment)
            {
                int slotIndex = selected->item.equipmentSlot.has_value()
                    ? static_cast<int>(*selected->item.equipmentSlot)
                    : 0;
                const char* slotNames[] = {"Head", "Body", "MainHand", "OffHand", "Accessory"};
                if (ImGui::Combo("Equipment Slot", &slotIndex, slotNames, IM_ARRAYSIZE(slotNames)))
                {
                    if (slotIndex >= 0
                        && slotIndex < static_cast<int>(gameplay::kEquipmentSlotCount))
                    {
                        selected->item.equipmentSlot =
                            static_cast<gameplay::EquipmentSlot>(slotIndex);
                        RefreshItemDatabaseDirty(state);
                    }
                }
            }
            else
            {
                ImGui::BeginDisabled(true);
                const char* noneSlot = "None";
                int noneIndex = 0;
                ImGui::Combo("Equipment Slot", &noneIndex, &noneSlot, 1);
                ImGui::EndDisabled();
                ImGui::TextDisabled("Equipment Slot is only authored on Equipment Items.");
            }
            if (selected->item.type == gameplay::ItemType::Equipment
                && (!selected->item.equipmentSlot.has_value()
                    || !gameplay::IsValidEquipmentSlot(*selected->item.equipmentSlot)))
            {
                ImGui::TextColored(
                    ImVec4(0.92f, 0.42f, 0.32f, 1.0f),
                    "Equipment Items require a typed Equipment Slot.");
            }
            if (selected->item.type != gameplay::ItemType::Equipment
                && selected->item.equipmentSlot.has_value())
            {
                ImGui::TextColored(
                    ImVec4(0.92f, 0.42f, 0.32f, 1.0f),
                    "Non-Equipment Items cannot carry Equipment Slot metadata.");
            }

            bool stackable = selected->item.stackable;
            if (ImGui::Checkbox("Stackable", &stackable))
            {
                selected->item.stackable = stackable;
                if (!stackable)
                {
                    selected->item.maxStack = gameplay::kMinItemMaxStack;
                }
                RefreshItemDatabaseDirty(state);
            }
            int maxStack = selected->item.maxStack;
            ImGui::BeginDisabled(!selected->item.stackable);
            if (ImGui::InputInt("Max Stack", &maxStack))
            {
                if (maxStack < gameplay::kMinItemMaxStack)
                {
                    maxStack = gameplay::kMinItemMaxStack;
                }
                if (maxStack > gameplay::kMaxItemMaxStack)
                {
                    maxStack = gameplay::kMaxItemMaxStack;
                }
                selected->item.maxStack = maxStack;
                RefreshItemDatabaseDirty(state);
            }
            ImGui::EndDisabled();
            if (!gameplay::IsValidItemStack(selected->item.stackable, selected->item.maxStack))
            {
                ImGui::TextColored(
                    ImVec4(0.92f, 0.42f, 0.32f, 1.0f),
                    "Non-stackable Items must use Max Stack 1.");
            }

            const std::vector<ItemAssetPickerItem> modelItems =
                CollectItemWorldModelPickerItems(modelCatalog);
            const bool modelMissing = !selected->item.worldModelIdentity.empty()
                && !ItemWorldModelIsCataloged(selected->item.worldModelIdentity, modelItems);
            ImGui::Separator();
            ImGui::TextUnformatted("World Model");
            if (selected->item.worldModelIdentity.empty())
            {
                ImGui::TextUnformatted("None");
                if (ImGui::Button("Assign...##worldModel"))
                {
                    ImGui::OpenPopup("Item World Model Picker");
                }
            }
            else
            {
                DrawItemModelThumbnail(
                    view, selected->item.worldModelIdentity, 48.0f, modelMissing);
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::TextUnformatted(selected->item.worldModelIdentity.c_str());
                if (modelMissing)
                {
                    ImGui::TextColored(
                        ImVec4(0.92f, 0.72f, 0.28f, 1.0f), "%s", ItemMissingWorldModelStatusText());
                }
                if (ImGui::Button("Replace...##worldModel"))
                {
                    ImGui::OpenPopup("Item World Model Picker");
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear##worldModel"))
                {
                    TryClearItemWorldModel(*selected);
                    RefreshItemDatabaseDirty(state);
                }
                ImGui::EndGroup();
            }
            state.modelPicker.kind = ItemAssetPickerKind::WorldModel;
            std::string assignedModel;
            if (DrawItemAssetPickerPopup(
                    state.modelPicker,
                    "Item World Model Picker",
                    "Compatible static .glb models",
                    modelItems,
                    view,
                    true,
                    assignedModel))
            {
                if (TryAssignItemWorldModel(*selected, assignedModel))
                {
                    RefreshItemDatabaseDirty(state);
                }
            }

            const std::vector<ItemAssetPickerItem> iconItems =
                CollectItemIconTexturePickerItems(textureCatalog);
            const bool iconMissing = !selected->item.iconTextureIdentity.empty()
                && !ItemIconTextureIsCataloged(selected->item.iconTextureIdentity, iconItems);
            ImGui::Separator();
            ImGui::TextUnformatted("Icon Texture");
            if (selected->item.iconTextureIdentity.empty())
            {
                ImGui::TextUnformatted("None");
                if (ImGui::Button("Assign...##icon"))
                {
                    ImGui::OpenPopup("Item Icon Texture Picker");
                }
            }
            else
            {
                DrawItemIconThumbnail(
                    view, selected->item.iconTextureIdentity, 48.0f, iconMissing);
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::TextUnformatted(selected->item.iconTextureIdentity.c_str());
                if (iconMissing)
                {
                    ImGui::TextColored(
                        ImVec4(0.92f, 0.72f, 0.28f, 1.0f), "%s", ItemMissingIconTextureStatusText());
                }
                if (ImGui::Button("Replace...##icon"))
                {
                    ImGui::OpenPopup("Item Icon Texture Picker");
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear##icon"))
                {
                    TryClearItemIconTexture(*selected);
                    RefreshItemDatabaseDirty(state);
                }
                ImGui::EndGroup();
            }
            state.iconPicker.kind = ItemAssetPickerKind::IconTexture;
            std::string assignedIcon;
            if (DrawItemAssetPickerPopup(
                    state.iconPicker,
                    "Item Icon Texture Picker",
                    "Compatible PNG textures",
                    iconItems,
                    view,
                    false,
                    assignedIcon))
            {
                if (TryAssignItemIconTexture(*selected, assignedIcon))
                {
                    RefreshItemDatabaseDirty(state);
                }
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Stat Modifiers (authored only, not applied)");
            ImGui::TextDisabled(
                "Duplicate stats are allowed and add left to right.");
            for (std::size_t index = 0; index < selected->item.modifiers.size(); ++index)
            {
                ImGui::PushID(static_cast<int>(index));
                int statIndex = static_cast<int>(selected->item.modifiers[index].stat);
                const char* statNames[] = {
                    "MaxHealth",
                    "MoveSpeed",
                    "JumpStrength",
                    "GravityScale",
                    "AttackPower",
                    "Defense",
                    "InteractionRange"};
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::Combo("##stat", &statIndex, statNames, IM_ARRAYSIZE(statNames)))
                {
                    if (statIndex >= 0 && statIndex < static_cast<int>(gameplay::kGameplayStatCount))
                    {
                        selected->item.modifiers[index].stat =
                            static_cast<gameplay::GameplayStatId>(statIndex);
                        RefreshItemDatabaseDirty(state);
                    }
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100.0f);
                float addend = selected->item.modifiers[index].addend;
                if (ImGui::InputFloat("##addend", &addend, 0.0f, 0.0f, "%.6g"))
                {
                    selected->item.modifiers[index].addend = addend;
                    RefreshItemDatabaseDirty(state);
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    TryRemoveItemStatModifier(*selected, index);
                    RefreshItemDatabaseDirty(state);
                    ImGui::PopID();
                    break;
                }
                if (!gameplay::IsValidGameplayStatAddend(selected->item.modifiers[index].addend))
                {
                    ImGui::TextColored(ImVec4(0.92f, 0.42f, 0.32f, 1.0f), "Addend must be finite.");
                }
                ImGui::PopID();
            }
            ImGui::BeginDisabled(
                selected->item.modifiers.size() >= gameplay::kMaxItemModifiers);
            if (ImGui::Button("Add Modifier"))
            {
                TryAddItemStatModifier(*selected, gameplay::GameplayStatId::MaxHealth, 0.0f);
                RefreshItemDatabaseDirty(state);
            }
            ImGui::EndDisabled();

            ImGui::Separator();
            if (ImGui::Button("Delete Item..."))
            {
                state.deleteConfirmOpen = true;
                ImGui::OpenPopup("Delete Item?");
            }
            if (state.deleteConfirmOpen
                && ImGui::BeginPopupModal("Delete Item?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Delete %s? This does not affect Inventory or Item Pickups.",
                    state.selectedIdentity.c_str());
                if (ImGui::Button("Delete"))
                {
                    TryDeleteSelectedItemDefinition(state);
                    selected = nullptr;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    state.deleteConfirmOpen = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::TextUnformatted("Create Item");
    char createName[gameplay::kMaxGameplayDefinitionNameLength + 1];
    std::snprintf(createName, sizeof(createName), "%s", state.createName.c_str());
    if (ImGui::InputText("items/<name>", createName, sizeof(createName)))
    {
        state.createName = createName;
        state.createError = false;
    }
    ImGui::SameLine();
    ImGui::BeginDisabled(!authoringAvailable || !state.loaded);
    if (ImGui::Button("Create"))
    {
        std::string created;
        const CreateItemDefinitionStatus createdStatus =
            TryCreateItemDefinition(state.working, state.createName, created);
        if (createdStatus == CreateItemDefinitionStatus::Created)
        {
            TrySelectItemDefinition(state, created);
            state.createName.clear();
            state.createError = false;
            RefreshItemDatabaseDirty(state);
            state.statusMessage = "Created " + created;
        }
        else
        {
            state.createError = true;
            if (createdStatus == CreateItemDefinitionStatus::DuplicateIdentity)
            {
                state.statusMessage = "duplicate identity";
            }
            else if (createdStatus == CreateItemDefinitionStatus::CategoryMismatch)
            {
                state.statusMessage = "characters/... is not an Item";
            }
            else
            {
                state.statusMessage = "malformed identity";
            }
        }
    }
    ImGui::EndDisabled();
    if (state.createError)
    {
        ImGui::TextColored(ImVec4(0.92f, 0.42f, 0.32f, 1.0f), "%s", state.statusMessage.c_str());
    }

    TickItemDatabasePickerPointerLocks(state, ImGui::IsMouseDown(ImGuiMouseButton_Left));
    ImGui::End();
}

#else

void DrawItemDatabaseEditor(
    ItemDatabaseEditorState&,
    const LevelEditorViewContext&,
    const assets::StaticModelCatalog&,
    const assets::SourceTextureCatalog&,
    bool,
    bool*)
{
}

#endif
}
