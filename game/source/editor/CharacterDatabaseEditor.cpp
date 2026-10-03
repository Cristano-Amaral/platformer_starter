#include "editor/CharacterDatabaseEditor.h"
#include "editor/LevelEditor.h"
#include "render/CharacterPreviewRenderer.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI) && defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
#include "editor/AuthoringPaths.h"
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
#include "imgui.h"
#endif

#include <cstdio>

namespace editor
{
#if defined(PLATFORMER_ENABLE_DEBUG_UI) && defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
void DrawCharacterDatabaseEditor(
    CharacterDatabaseEditorState& state, const LevelEditorViewContext& view,
    const assets::StaticModelCatalog& modelCatalog, bool authoringAvailable, bool* open)
{
    if (open == nullptr || !*open) return;
    ApplyKnownEditorWindowPlacement(kCharacterDatabaseWindowName, view.viewportWidth,
        view.viewportHeight, view.forceDefaultLayout);
    if (!ImGui::Begin(kCharacterDatabaseWindowName, open)) { ImGui::End(); return; }
    const std::filesystem::path path = AuthoringSourcePath(gameplay::kGameplayDefinitionsLogicalPath);
    if (!state.loaded && authoringAvailable && !path.empty())
        ApplyLoadedCharacterDatabase(state, gameplay::LoadGameplayDefinitionsFile(path));
    ImGui::TextUnformatted("Reusable Character definitions and animation bindings.");
    ImGui::TextDisabled("Apply promotes validated edits to runtime; Save persists authored edits.");
    ImGui::Text("Status: %s%s", state.dirty ? "Dirty - " : "", state.statusMessage.c_str());
    ImGui::BeginDisabled(!authoringAvailable || !state.loaded);
    if (ImGui::Button("Apply")) state.applyRequested = true;
    ImGui::EndDisabled(); ImGui::SameLine();
    ImGui::BeginDisabled(!authoringAvailable || !state.loaded || !state.dirty);
    if (ImGui::Button("Save")) TrySaveCharacterDatabase(state, path, authoringAvailable);
    ImGui::EndDisabled(); ImGui::SameLine();
    if (ImGui::Button("Reload")) TryReloadCharacterDatabase(state, path, authoringAvailable, false);
    if (state.reloadConfirmOpen) { ImGui::OpenPopup("Discard Character edits?"); state.reloadConfirmOpen = false; }
    if (ImGui::BeginPopupModal("Discard Character edits?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::Button("Discard and Reload")) { TryReloadCharacterDatabase(state, path, authoringAvailable, true); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine(); if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup(); ImGui::EndPopup();
    }
    char search[128]; std::snprintf(search, sizeof(search), "%s", state.searchFilter.c_str());
    if (ImGui::InputText("Search Characters", search, sizeof(search))) state.searchFilter = search;
    char createName[gameplay::kMaxGameplayDefinitionNameLength + 1];
    std::snprintf(createName, sizeof(createName), "%s", state.createName.c_str());
    if (ImGui::InputText("characters/<name>", createName, sizeof(createName))) state.createName = createName;
    ImGui::SameLine();
    if (ImGui::Button("Create"))
    {
        std::string created;
        const auto status = TryCreateCharacterDefinition(state.working, state.createName, created);
        if (status == CharacterDefinitionEditStatus::Changed)
        {
            TrySelectCharacterDefinition(state, created);
            state.createName.clear();
            RefreshCharacterDatabaseDirty(state);
            state.statusMessage = "Created " + created;
        }
        else state.statusMessage = "Create rejected";
    }
    const auto filtered = FilterCharacterDatabaseIdentities(state.working, state.searchFilter);
    ImGui::BeginChild("##characterList", ImVec2(230, 0), true);
    for (const auto& identity : filtered)
    {
        const auto* definition = state.working.Find(identity);
        if (ImGui::Selectable(identity.c_str(), state.selectedIdentity == identity))
            TrySelectCharacterDefinition(state, identity);
        if (definition != nullptr) ImGui::TextDisabled("%s", definition->character.displayName.c_str());
    }
    ImGui::EndChild(); ImGui::SameLine(); ImGui::BeginChild("##characterInspector", ImVec2(0, 0), true);
    auto* selected = state.working.FindMutable(state.selectedIdentity);
    if (selected == nullptr || selected->category != gameplay::GameplayDefinitionCategory::Character)
    {
        ImGui::TextUnformatted("Select or create a Character.");
        if (view.characterPreview != nullptr)
            view.characterPreview->SyncModel({}, {}, {});
    }
    else
    {
        ImGui::Text("Identity: %s", selected->identity.c_str());
        char rename[gameplay::kMaxGameplayDefinitionNameLength + 1];
        std::snprintf(rename, sizeof(rename), "%s", state.renameName.c_str());
        if (ImGui::InputText("Rename leaf", rename, sizeof(rename))) state.renameName = rename;
        ImGui::SameLine(); if (ImGui::Button("Rename"))
        {
            const auto status = TryRenameSelectedCharacterIdentity(state, state.renameName);
            state.statusMessage = status == CharacterDefinitionEditStatus::Changed ? "Renamed" : "Rename rejected";
            selected = state.working.FindMutable(state.selectedIdentity);
        }
        char displayName[gameplay::kMaxCharacterDisplayNameLength + 1];
        std::snprintf(displayName, sizeof(displayName), "%s", selected->character.displayName.c_str());
        if (ImGui::InputText("Display Name", displayName, sizeof(displayName)))
        { selected->character.displayName = displayName; RefreshCharacterDatabaseDirty(state); }
        char description[gameplay::kMaxCharacterDescriptionLength + 1];
        std::snprintf(description, sizeof(description), "%s", selected->character.description.c_str());
        if (ImGui::InputTextMultiline("Description", description, sizeof(description), ImVec2(-1, 55)))
        { selected->character.description = description; RefreshCharacterDatabaseDirty(state); }
        int type = static_cast<int>(selected->character.type);
        const char* typeNames[] = {"Player", "Enemy", "NPC", "Animal"};
        if (ImGui::Combo("Character Type", &type, typeNames, IM_ARRAYSIZE(typeNames)))
        { selected->character.type = static_cast<gameplay::CharacterType>(type); RefreshCharacterDatabaseDirty(state); }
        ImGui::Separator(); ImGui::TextUnformatted("World Model");
        if (selected->character.worldModelIdentity.empty()) ImGui::TextUnformatted("None");
        else
        {
            ImGui::TextWrapped("%s", selected->character.worldModelIdentity.c_str());
            if (modelCatalog.Find(selected->character.worldModelIdentity) == nullptr)
                ImGui::TextColored(ImVec4(.92f,.72f,.28f,1), "Missing (authored reference preserved)");
        }
        if (ImGui::BeginCombo("Select Model", selected->character.worldModelIdentity.empty() ? "None" : selected->character.worldModelIdentity.c_str()))
        {
            if (ImGui::Selectable("None")) { TryClearCharacterWorldModel(*selected); RefreshCharacterDatabaseDirty(state); }
            for (const auto& model : modelCatalog.Entries()) if (ImGui::Selectable(model.displayName.c_str()))
            { TryAssignCharacterWorldModel(*selected, model.canonicalIdentity); RefreshCharacterDatabaseDirty(state); }
            ImGui::EndCombo();
        }
        ImGui::Separator(); ImGui::TextUnformatted("Character Animations");
        const char* labels[] = {"Idle Clip", "Move Clip", "Jump Clip", "Hit Reaction Clip", "Attack Clip"};
        auto slots = {CharacterAnimationSlot::Idle, CharacterAnimationSlot::Move,
            CharacterAnimationSlot::Jump, CharacterAnimationSlot::HitReaction, CharacterAnimationSlot::Attack};
        int slotIndex = 0;
        for (CharacterAnimationSlot slot : slots)
        {
            std::string* value = slot == CharacterAnimationSlot::Idle
                ? &selected->character.animations.idle : slot == CharacterAnimationSlot::Move
                ? &selected->character.animations.move : slot == CharacterAnimationSlot::Jump ? &selected->character.animations.jump : slot == CharacterAnimationSlot::HitReaction ? &selected->character.animations.hitReaction : &selected->character.animations.attack;
            char clip[gameplay::kMaxCharacterAnimationClipNameLength + 1]{};
            std::snprintf(clip, sizeof(clip), "%s", value->c_str());
            if (ImGui::InputText(labels[slotIndex], clip, sizeof(clip)))
            {
                if (clip[0] == '\0') TryClearCharacterAnimation(*selected, slot);
                else (void)TryAssignCharacterAnimation(*selected, slot, clip);
                RefreshCharacterDatabaseDirty(state);
            }
            ImGui::SameLine();
            ImGui::TextDisabled("%s", value->empty() ? "None" : "Authored (validated at runtime)");
            std::string* assetValue = slot == CharacterAnimationSlot::Idle
                ? &selected->character.animations.idleAsset : slot == CharacterAnimationSlot::Move
                ? &selected->character.animations.moveAsset : slot == CharacterAnimationSlot::Jump ? &selected->character.animations.jumpAsset : slot == CharacterAnimationSlot::HitReaction ? &selected->character.animations.hitReactionAsset : &selected->character.animations.attackAsset;
            ImGui::PushID(slotIndex);
            if (ImGui::BeginCombo("Reusable Asset", assetValue->empty() ? "None (embedded clip)" : assetValue->c_str()))
            {
                if (ImGui::Selectable("None (embedded clip)"))
                { TryClearCharacterAnimationAsset(*selected, slot); RefreshCharacterDatabaseDirty(state); }
                for (const auto& candidate : state.working.Definitions())
                {
                    if (candidate.category != gameplay::GameplayDefinitionCategory::Animation) continue;
                    if (ImGui::Selectable(candidate.identity.c_str(), *assetValue == candidate.identity))
                    { TryAssignCharacterAnimationAsset(*selected, slot, candidate.identity); RefreshCharacterDatabaseDirty(state); }
                }
                ImGui::EndCombo();
            }
            if (!assetValue->empty())
            {
                const auto* asset = state.working.Find(*assetValue);
                if (asset == nullptr || asset->category != gameplay::GameplayDefinitionCategory::Animation)
                    ImGui::TextColored(ImVec4(.92f,.45f,.28f,1), "Missing reusable identity");
                else
                    ImGui::TextDisabled("Source: %s | Clip: %s",
                        asset->animation.sourceAssetIdentity.c_str(), asset->animation.sourceClipName.c_str());
            }
            ImGui::PopID();
            ++slotIndex;
        }
        ImGui::Separator(); ImGui::TextUnformatted("Melee Hit Detection (contact only)");
        auto& hit = selected->character.meleeHit;
        if (ImGui::Checkbox("Enable Melee Hit", &hit.enabled))
        {
            if (!hit.enabled) hit = {};
            RefreshCharacterDatabaseDirty(state);
        }
        if (hit.enabled)
        {
            float window[2] = {hit.windowStart, hit.windowEnd};
            if (ImGui::InputFloat2("Attack window start/end", window))
            { hit.windowStart = window[0]; hit.windowEnd = window[1]; RefreshCharacterDatabaseDirty(state); }
            float center[3] = {hit.center.x, hit.center.y, hit.center.z};
            if (ImGui::InputFloat3("Local hit center", center))
            { hit.center = {center[0], center[1], center[2]}; RefreshCharacterDatabaseDirty(state); }
            float half[3] = {hit.halfExtents.x, hit.halfExtents.y, hit.halfExtents.z};
            if (ImGui::InputFloat3("Hit half extents", half))
            { hit.halfExtents = {half[0], half[1], half[2]}; RefreshCharacterDatabaseDirty(state); }
            if (gameplay::ValidateCharacterDefinition(selected->character)
                == gameplay::ValidateCharacterStatus::InvalidMeleeHit)
                ImGui::TextColored(ImVec4(.92f,.45f,.28f,1), "Invalid melee hit values; Apply will reject");
        }
        RefreshCharacterAssetValidation(state, selected->character, AuthoringSourceRoot());
        const auto& validation = state.assetValidation;
        ImGui::Separator();
        ImGui::TextUnformatted("Humanoid Skeleton Mapping");
        ImGui::TextDisabled("Required roles must be assigned. Optional roles may be None.");
        if (ImGui::Button("Suggest Mapping"))
        {
            animation::SuggestHumanoidMapping(selected->character.humanoidMapping, validation.model);
            RefreshCharacterDatabaseDirty(state);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Mapping"))
        {
            selected->character.humanoidMapping = {};
            RefreshCharacterDatabaseDirty(state);
        }
        for (std::size_t roleIndex = 0; roleIndex < gameplay::kHumanoidJointRoleCount; ++roleIndex)
        {
            const auto role = static_cast<gameplay::HumanoidJointRole>(roleIndex);
            const auto& assigned = selected->character.humanoidMapping.joints[roleIndex];
            ImGui::PushID(static_cast<int>(roleIndex));
            ImGui::Text("%s (%s)", gameplay::kHumanoidJointRoleNames[roleIndex].data(),
                gameplay::HumanoidJointRoleRequired(role) ? "required" : "optional");
            ImGui::SameLine(220.0f);
            if (ImGui::BeginCombo("##joint", assigned.empty() ? "None" : assigned.c_str()))
            {
                if (ImGui::Selectable("None", assigned.empty()))
                    AssignHumanoidJoint(state, role, {});
                for (const auto& joint : validation.model.allJoints)
                    if (ImGui::Selectable(joint.name.c_str(), assigned == joint.name))
                        AssignHumanoidJoint(state, role, joint.name);
                ImGui::EndCombo();
            }
            if (!assigned.empty() && std::none_of(validation.model.allJoints.begin(),
                validation.model.allJoints.end(), [&](const auto& joint) { return joint.name == assigned; }))
                ImGui::TextColored(ImVec4(.92f,.45f,.28f,1), "Stale/missing: %s", assigned.c_str());
            ImGui::PopID();
        }
        const auto mappingValidation = animation::ValidateHumanoidMapping(
            selected->character.humanoidMapping, validation.model);
        ImGui::Text("Mapping: %s", animation::HumanoidMappingStateName(mappingValidation.state));
        if (ImGui::TreeNode("Mapping diagnostics"))
        {
            for (const auto& detail : mappingValidation.details)
                ImGui::TextWrapped("%s: %s (joint %d)", gameplay::HumanoidJointRoleName(detail.role).data(),
                    detail.detail.c_str(), detail.jointIndex);
            ImGui::TreePop();
        }
        const std::string sourceAnimationIdentity = state.previewSlot == CharacterPreviewSlot::Idle
            ? selected->character.animations.idleAsset : state.previewSlot == CharacterPreviewSlot::Move
            ? selected->character.animations.moveAsset : state.previewSlot == CharacterPreviewSlot::Jump ? selected->character.animations.jumpAsset : state.previewSlot == CharacterPreviewSlot::HitReaction ? selected->character.animations.hitReactionAsset : selected->character.animations.attackAsset;
        auto* sourceDefinition = state.working.FindMutable(sourceAnimationIdentity);
        if (sourceDefinition != nullptr
            && sourceDefinition->category == gameplay::GameplayDefinitionCategory::Animation
            && ImGui::TreeNode("Reusable animation source humanoid mapping"))
        {
            if (state.sourceModelKey != sourceDefinition->animation.sourceAssetIdentity)
            {
                gameplay::CharacterDefinition sourceCharacter;
                sourceCharacter.worldModelIdentity = sourceDefinition->animation.sourceAssetIdentity;
                state.sourceModelValidation = animation::ValidateCharacterAssets(
                    state.working, sourceCharacter, AuthoringSourceRoot()).model;
                state.sourceModelKey = sourceCharacter.worldModelIdentity;
            }
            const auto& sourceModel = state.sourceModelValidation;
            auto& sourceMapping = sourceDefinition->animation.sourceHumanoidMapping;
            ImGui::Text("Animation: %s | Clip: %s", sourceAnimationIdentity.c_str(),
                sourceDefinition->animation.sourceClipName.c_str());
            ImGui::Text("Source joints: %d", sourceModel.jointCount);
            if (sourceModel.status != animation::CharacterModelValidationStatus::Resolved)
                ImGui::TextWrapped("Source model: %s", sourceModel.detail.c_str());
            if (ImGui::TreeNode("Source joint hierarchy"))
            {
                for (std::size_t jointIndex = 0; jointIndex < sourceModel.allJoints.size(); ++jointIndex)
                    ImGui::Text("%zu: %s (parent %d)", jointIndex,
                        sourceModel.allJoints[jointIndex].name.c_str(),
                        sourceModel.allJoints[jointIndex].parentIndex);
                ImGui::TreePop();
            }
            if (ImGui::Button("Suggest Source Mapping"))
            { animation::SuggestHumanoidMapping(sourceMapping, sourceModel); RefreshCharacterDatabaseDirty(state); state.validationInitialized = false; }
            ImGui::SameLine();
            if (ImGui::Button("Clear Source Mapping"))
            { sourceMapping = {}; RefreshCharacterDatabaseDirty(state); state.validationInitialized = false; }
            for (std::size_t roleIndex = 0; roleIndex < gameplay::kHumanoidJointRoleCount; ++roleIndex)
            {
                const auto role = static_cast<gameplay::HumanoidJointRole>(roleIndex);
                const auto& assigned = sourceMapping.joints[roleIndex];
                ImGui::PushID(static_cast<int>(roleIndex) + 100);
                ImGui::TextUnformatted(gameplay::kHumanoidJointRoleNames[roleIndex].data());
                ImGui::SameLine(220.0f);
                if (ImGui::BeginCombo("##sourceJoint", assigned.empty() ? "None" : assigned.c_str()))
                {
                    if (ImGui::Selectable("None", assigned.empty()))
                        AssignSourceHumanoidJoint(state, sourceAnimationIdentity, role, {});
                for (const auto& joint : sourceModel.allJoints)
                        if (ImGui::Selectable(joint.name.c_str(), assigned == joint.name))
                            AssignSourceHumanoidJoint(state, sourceAnimationIdentity, role, joint.name);
                    ImGui::EndCombo();
                }
                if (!assigned.empty() && std::none_of(sourceModel.allJoints.begin(),
                    sourceModel.allJoints.end(), [&](const auto& joint) { return joint.name == assigned; }))
                    ImGui::TextColored(ImVec4(.92f,.45f,.28f,1), "Stale/missing: %s", assigned.c_str());
                ImGui::PopID();
            }
            const auto sourceValidation = animation::ValidateHumanoidMapping(sourceMapping, sourceModel);
            ImGui::Text("Source mapping: %s", animation::HumanoidMappingStateName(sourceValidation.state));
            for (const auto& detail : sourceValidation.details)
                if (detail.jointIndex < 0) ImGui::TextWrapped("%s: %s",
                    gameplay::HumanoidJointRoleName(detail.role).data(), detail.detail.c_str());
            ImGui::TreePop();
        }
        if (!state.validationInitialized)
            RefreshCharacterAssetValidation(state, selected->character, AuthoringSourceRoot());
        render::CharacterPreviewRenderer* preview = view.characterPreview;
        const std::filesystem::path sourceRoot = AuthoringSourceRoot();
        if (preview != nullptr)
        {
            preview->SyncModel(selected->character.worldModelIdentity,
                sourceRoot / selected->character.worldModelIdentity, validation.model);
        }
        const auto previewResolution = ResolveCharacterPreviewAnimation(
            state.working, selected->character, validation, state.previewSlot);
        if (preview != nullptr) preview->SyncAnimation(previewResolution, sourceRoot);

        ImGui::Separator();
        ImGui::TextUnformatted("Character / Animation Preview");
        int previewSlot = static_cast<int>(state.previewSlot);
        const char* previewSlotNames[] = {"Idle", "Move", "Jump", "Hit Reaction", "Attack"};
        if (ImGui::Combo("Preview Animation", &previewSlot, previewSlotNames, 5))
        {
            state.previewSlot = static_cast<CharacterPreviewSlot>(previewSlot);
            RestartCharacterPreviewPlayback(state.previewPlayback);
        }
        const bool canAnimate = preview != nullptr && preview->HasAnimation();
        ImGui::BeginDisabled(!canAnimate);
        if (ImGui::Button(state.previewPlayback.playing ? "Pause" : "Play"))
            state.previewPlayback.playing = !state.previewPlayback.playing;
        ImGui::SameLine();
        if (ImGui::Button("Restart")) RestartCharacterPreviewPlayback(state.previewPlayback);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(preview == nullptr || !preview->HasModel());
        if (ImGui::Button("Reset / Reframe") && preview != nullptr)
            ResetStaticModelPreviewOrbit(state.previewOrbit, preview->Bounds());
        constexpr float kOrbitButtonPixels = 75.0f;
        ImGui::SameLine(); ImGui::TextUnformatted("Orbit"); ImGui::SameLine();
        if (ImGui::ArrowButton("##orbitLeft", ImGuiDir_Left))
            ApplyStaticModelPreviewOrbit(state.previewOrbit, -kOrbitButtonPixels, 0.0f);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##orbitUp", ImGuiDir_Up))
            ApplyStaticModelPreviewOrbit(state.previewOrbit, 0.0f, -kOrbitButtonPixels);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##orbitDown", ImGuiDir_Down))
            ApplyStaticModelPreviewOrbit(state.previewOrbit, 0.0f, kOrbitButtonPixels);
        ImGui::SameLine();
        if (ImGui::ArrowButton("##orbitRight", ImGuiDir_Right))
            ApplyStaticModelPreviewOrbit(state.previewOrbit, kOrbitButtonPixels, 0.0f);
        ImGui::SameLine(); ImGui::TextUnformatted("Zoom"); ImGui::SameLine();
        if (ImGui::Button("-##previewZoom"))
            ApplyStaticModelPreviewDolly(state.previewOrbit, -1.0f);
        ImGui::SameLine();
        if (ImGui::Button("+##previewZoom"))
            ApplyStaticModelPreviewDolly(state.previewOrbit, 1.0f);
        ImGui::EndDisabled();

        if (preview != nullptr && preview->HasModel()
            && state.previewFramedIdentity != preview->LoadedIdentity())
        {
            ResetStaticModelPreviewOrbit(state.previewOrbit, preview->Bounds());
            state.previewFramedIdentity = preview->LoadedIdentity();
        }
        else if (preview == nullptr || !preview->HasModel()) state.previewFramedIdentity.clear();

        if (canAnimate)
            AdvanceCharacterPreviewPlayback(state.previewPlayback, view.frameDeltaSeconds,
                preview->AnimationDurationSeconds(), previewResolution.playbackMode);
        ImGui::Text("%s | %s | %s", !canAnimate ? "Unavailable"
                : (state.previewPlayback.playing ? "Playing" : "Paused"),
            previewResolution.playbackMode == animation::PlaybackMode::Loop ? "Loop" : "Clamp",
            CharacterPreviewAnimationStatusName(previewResolution.status));
        ImGui::TextDisabled("%s", previewResolution.detail.c_str());
        if (!sourceAnimationIdentity.empty()
            && previewResolution.retarget.status != animation::RetargetValidationStatus::NotRequested)
        {
            ImGui::Text("Retarget: %s | Source mapping: %s | Target mapping: %s",
                animation::RetargetValidationStatusName(previewResolution.retarget.status),
                previewResolution.retarget.sourceMappingState.c_str(),
                previewResolution.retarget.targetMappingState.c_str());
            ImGui::TextDisabled("Source: %s (%d joints) | Target: %s (%d joints)",
                previewResolution.sourceAssetIdentity.c_str(), previewResolution.retarget.sourceJointCount,
                selected->character.worldModelIdentity.c_str(), previewResolution.retarget.targetJointCount);
        }

        const ImVec2 available = ImGui::GetContentRegionAvail();
        const float previewHeight = std::min(320.0f, std::max(180.0f, available.x * 0.56f));
        const auto renderSize = ResolvePreviewRenderSize(available.x, previewHeight);
        if (renderSize.valid)
        {
            const ImVec2 canvas(static_cast<float>(renderSize.width), static_cast<float>(renderSize.height));
            const ImVec2 screen = ImGui::GetCursorScreenPos();
            unsigned int texture = 0;
            if (preview != nullptr && preview->Render(renderSize.width, renderSize.height,
                state.previewOrbit, state.previewPlayback)) texture = preview->TextureGpuId();
            if (texture != 0)
                ImGui::Image(ImTextureRef(static_cast<ImTextureID>(static_cast<intptr_t>(texture))),
                    canvas, ImVec2(0, 1), ImVec2(1, 0));
            else
            {
                ImGui::Dummy(canvas);
                ImGui::GetWindowDrawList()->AddRectFilled(screen,
                    ImVec2(screen.x + canvas.x, screen.y + canvas.y), IM_COL32(48, 52, 62, 255));
                const char* message = preview != nullptr && preview->IsStaticModel()
                    ? "Static model - skeletal playback unavailable"
                    : (validation.model.detail.empty() ? "Preview unavailable" : validation.model.detail.c_str());
                ImGui::GetWindowDrawList()->AddText(ImVec2(screen.x + 12, screen.y + 12),
                    IM_COL32(220, 220, 224, 255), message);
            }
            ImGui::SetCursorScreenPos(screen);
            ImGui::InvisibleButton("character-preview-orbit", canvas);
            ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
            const ImGuiIO& io = ImGui::GetIO();
            if (ImGui::IsItemActive() && preview != nullptr && preview->HasModel())
                ApplyStaticModelPreviewOrbit(state.previewOrbit, io.MouseDelta.x, io.MouseDelta.y);
            if (ImGui::IsItemHovered() && preview != nullptr && preview->HasModel())
                ApplyStaticModelPreviewDolly(state.previewOrbit, io.MouseWheel);
        }
        ImGui::TextDisabled("LMB orbit | Wheel zoom | Framing derives from model bounds");

        ImGui::Separator();
        ImGui::TextUnformatted("Character Asset / Compatibility");
        ImGui::Text("World Model: %s", animation::CharacterModelValidationStatusName(
            validation.model.status));
        ImGui::Text("Model load: %s | Skin: %s | Skeleton: %s",
            validation.model.modelLoaded ? "Success" : "No",
            validation.model.skinned ? "Skinned" : "Static / non-skinned",
            validation.model.hasSkeleton ? "Available" : "None");
        if (validation.model.hasSkeleton)
        {
            ImGui::Text("Joints: %d", validation.model.jointCount);
            if (ImGui::TreeNode("Recognized joint hierarchy"))
            {
                for (std::size_t joint = 0; joint < validation.model.joints.size(); ++joint)
                {
                    const auto& metadata = validation.model.joints[joint];
                    ImGui::Text("%zu: %s (parent %d)", joint, metadata.name.c_str(),
                        metadata.parentIndex);
                }
                if (validation.model.jointListTruncated)
                    ImGui::TextDisabled("... bounded to first %zu joints",
                        animation::kCharacterAssetDiagnosticJointLimit);
                ImGui::TreePop();
            }
        }
        ImGui::TextDisabled("%s", validation.model.detail.c_str());
        const char* compatibilityLabels[] = {"Idle", "Move", "Jump", "Hit Reaction", "Attack"};
        const animation::CharacterAnimationCompatibilityResult* compatibility[] = {
            &validation.idle, &validation.move, &validation.jump, &validation.hitReaction, &validation.attack};
        for (int index = 0; index < 5; ++index)
        {
            ImGui::Text("%s: %s", compatibilityLabels[index],
                animation::CharacterAnimationCompatibilityStatusName(
                    compatibility[index]->status));
            ImGui::SameLine();
            ImGui::TextDisabled("%s", compatibility[index]->detail.c_str());
        }
        ImGui::Separator(); ImGui::TextUnformatted("Base Stats (authored only)");
        for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
        {
            bool enabled = selected->character.hasBaseStat[index];
            ImGui::PushID(static_cast<int>(index));
            if (ImGui::Checkbox(gameplay::kGameplayStatNames[index].data(), &enabled))
            { selected->character.hasBaseStat[index] = enabled; RefreshCharacterDatabaseDirty(state); }
            if (enabled) { ImGui::SameLine(); float value = selected->character.baseStatValue[index];
                if (ImGui::InputFloat("##value", &value, 0, 0, "%.6g"))
                { selected->character.baseStatValue[index] = value; RefreshCharacterDatabaseDirty(state); } }
            ImGui::PopID();
        }
        if (ImGui::Button("Delete Character")) { TryDeleteSelectedCharacterDefinition(state); selected = nullptr; }
    }
    ImGui::EndChild(); ImGui::End();
}
#else
void DrawCharacterDatabaseEditor(CharacterDatabaseEditorState&, const LevelEditorViewContext&,
    const assets::StaticModelCatalog&, bool, bool*) {}
#endif
}
