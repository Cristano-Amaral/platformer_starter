#include "ui/debug/DebugMetrics.h"

#if defined(PLATFORMER_ENABLE_DEBUG_UI)

#include "core/RunTimeFormat.h"
#include "editor/AuthoringPaths.h"
#include "editor/EditorLayout.h"
#include "editor/EditorLayoutUi.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "gameplay/Equipment.h"
#include "gameplay/Inventory.h"
#include "gameplay/ItemDefinition.h"
#include "gameplay/ItemIdentity.h"
#include "gameplay/PlayerCharacterStats.h"
#include "imgui.h"

#include <cstddef>
#include <cstdio>
#include <filesystem>

namespace ui
{
namespace
{
void ApplyEditorWindowPlacement(
    const char* windowName,
    float viewportWidth,
    float viewportHeight,
    bool forceDefaultLayout)
{
    editor::ApplyKnownEditorWindowPlacement(
        windowName, viewportWidth, viewportHeight, forceDefaultLayout);
}
const char* BoolText(bool value)
{
    return value ? "true" : "false";
}

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
void DrawGameplayDefinitionsInspection()
{
    struct Probe
    {
        const char* label = "";
        gameplay::GameplayDefinitionReference reference;
        std::optional<gameplay::GameplayDefinitionCategory> expected;
        gameplay::GameplayReferenceResolution resolution;
    };

    static bool loaded = false;
    static gameplay::ParseGameplayDefinitionsResult parsed;
    static std::string pathText;
    static Probe probes[3];

    if (!ImGui::CollapsingHeader("Gameplay Definitions (M98-M101)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    ImGui::TextUnformatted(
        "Development inspection of the authored gameplay-definition catalog.");
    ImGui::TextUnformatted(
        "Does not drive Player, Inventory, Item Pickup, or Level objects.");
    if (ImGui::Button("Reload gameplay definitions"))
    {
        loaded = false;
    }
    if (!loaded)
    {
        loaded = true;
        const std::filesystem::path path =
            editor::AuthoringSourcePath(gameplay::kGameplayDefinitionsLogicalPath);
        pathText = path.empty() ? std::string() : path.string();
        if (path.empty())
        {
            parsed = {};
            parsed.status = gameplay::LoadGameplayDefinitionsStatus::Error;
            parsed.error = "authoring unavailable";
        }
        else
        {
            parsed = gameplay::LoadGameplayDefinitionsFile(path);
        }

        probes[0].label = "valid";
        probes[0].reference.identity = "items/master_key";
        probes[0].expected = gameplay::GameplayDefinitionCategory::Item;
        probes[1].label = "missing";
        probes[1].reference.identity = "items/missing_relic";
        probes[1].expected = gameplay::GameplayDefinitionCategory::Item;
        probes[2].label = "category mismatch";
        probes[2].reference.identity = "characters/guard";
        probes[2].expected = gameplay::GameplayDefinitionCategory::Item;
        for (Probe& probe : probes)
        {
            const std::string authored = probe.reference.identity;
            if (parsed.status == gameplay::LoadGameplayDefinitionsStatus::Loaded)
            {
                probe.resolution = parsed.registry.Resolve(probe.reference, probe.expected);
            }
            else
            {
                probe.resolution = {};
            }
            if (probe.reference.identity != authored)
            {
                probe.resolution = {};
                probe.resolution.status = gameplay::GameplayReferenceStatus::Malformed;
            }
        }
    }

    ImGui::Text("Source: %s", pathText.empty() ? "(unavailable)" : pathText.c_str());
    ImGui::Text(
        "Load: %s%s%s",
        gameplay::LoadGameplayDefinitionsStatusName(parsed.status),
        parsed.error.empty() ? "" : " - ",
        parsed.error.c_str());
    ImGui::Text("Definitions: %d", static_cast<int>(parsed.registry.Count()));
    for (const gameplay::GameplayDefinition& definition : parsed.registry.Definitions())
    {
        const std::string_view category = gameplay::GameplayDefinitionCategoryName(definition.category);
        ImGui::BulletText(
            "%s (%.*s)",
            definition.identity.c_str(),
            static_cast<int>(category.size()),
            category.data());
        if (definition.category == gameplay::GameplayDefinitionCategory::Item)
        {
            const std::string_view typeName = gameplay::ItemTypeName(definition.item.type);
            ImGui::Text(
                "    display: %s",
                definition.item.displayName.empty() ? "(none)" : definition.item.displayName.c_str());
            ImGui::Text(
                "    type: %.*s  stackable: %s  maxStack: %d",
                static_cast<int>(typeName.size()),
                typeName.data(),
                definition.item.stackable ? "true" : "false",
                definition.item.maxStack);
            if (!definition.item.worldModelIdentity.empty())
            {
                ImGui::Text("    world model: %s", definition.item.worldModelIdentity.c_str());
            }
            if (!definition.item.iconTextureIdentity.empty())
            {
                ImGui::Text("    icon: %s", definition.item.iconTextureIdentity.c_str());
            }
            for (const gameplay::GameplayStatModifier& modifier : definition.item.modifiers)
            {
                const std::string_view statName = gameplay::GameplayStatName(modifier.stat);
                ImGui::Text(
                    "    modifier %.*s %+g",
                    static_cast<int>(statName.size()),
                    statName.data(),
                    static_cast<double>(modifier.addend));
            }
        }
        else
        {
            const std::string_view typeName = gameplay::CharacterTypeName(definition.character.type);
            ImGui::Text("    display: %s", definition.character.displayName.c_str());
            ImGui::Text("    type: %.*s", static_cast<int>(typeName.size()), typeName.data());
            if (!definition.character.worldModelIdentity.empty())
                ImGui::Text("    world model: %s", definition.character.worldModelIdentity.c_str());
        }
        for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
        {
            const auto value = gameplay::GameplayDefinitionStat(
                definition, static_cast<gameplay::GameplayStatId>(index));
            if (!value.has_value())
            {
                continue;
            }
            const std::string_view statName =
                gameplay::GameplayStatName(static_cast<gameplay::GameplayStatId>(index));
            ImGui::Text(
                "    %.*s = %.6g",
                static_cast<int>(statName.size()),
                statName.data(),
                *value);
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Identity is the authored key. A runtime index is not serialized.");
    if (parsed.status != gameplay::LoadGameplayDefinitionsStatus::Loaded)
    {
        ImGui::TextUnformatted("Resolve probes were not run.");
        return;
    }
    for (const Probe& probe : probes)
    {
        const char* expectedName = probe.expected.has_value()
            ? gameplay::GameplayDefinitionCategoryName(*probe.expected).data()
            : "any";
        ImGui::Text(
            "%s: %s as %s -> %s",
            probe.label,
            probe.reference.identity.c_str(),
            expectedName,
            gameplay::GameplayReferenceStatusName(probe.resolution.status));
        if (probe.resolution.status == gameplay::GameplayReferenceStatus::Resolved
            && probe.resolution.definition != nullptr && probe.resolution.index.has_value())
        {
            ImGui::Text(
                "    bound %s at runtime index %d",
                probe.resolution.definition->identity.c_str(),
                static_cast<int>(*probe.resolution.index));
        }
        else
        {
            ImGui::TextUnformatted("    bound definition: none");
        }
    }
}
#endif

float CoyoteRemaining(const DebugMetricsSnapshot& snapshot)
{
    if (!snapshot.coyoteAvailable)
    {
        return 0.0f;
    }
    if (snapshot.grounded)
    {
        return snapshot.coyoteDuration;
    }

    const float remaining = snapshot.coyoteDuration - snapshot.coyoteElapsed;
    return remaining > 0.0f ? remaining : 0.0f;
}
}

void DrawDebugMetrics(
    const DebugMetricsSnapshot& snapshot,
    float viewportWidth,
    float viewportHeight,
    bool forceDefaultLayout,
    bool recoverOffscreenLayout,
    bool* open,
    gameplay::Inventory* inventory,
    gameplay::Equipment* equipment,
    const gameplay::GameplayDefinitionRegistry* gameplayDefinitions,
    const gameplay::PlayerCharacterStats* playerCharacterStats)
{
    ApplyEditorWindowPlacement(
        editor::kMetricsWindowName,
        viewportWidth,
        viewportHeight,
        forceDefaultLayout);
    ImGui::Begin(editor::kMetricsWindowName, open);
    editor::RecoverKnownEditorWindowIfOffscreen(
        editor::kMetricsWindowName,
        viewportWidth,
        viewportHeight,
        recoverOffscreenLayout);

    if (ImGui::CollapsingHeader("Frame", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("FPS: %.0f", snapshot.fps);
        ImGui::Text("deltaSeconds: %.6f", snapshot.deltaSeconds);
        ImGui::Text("deltaMilliseconds: %.3f", snapshot.deltaSeconds * 1000.0f);
    }

    if (ImGui::CollapsingHeader("Run timer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char formatted[32]{};
        core::FormatRunTime(formatted, sizeof(formatted), snapshot.runTimeSeconds);
        ImGui::Text("Run time seconds: %.6f", snapshot.runTimeSeconds);
        ImGui::Text("Run timer: %s", snapshot.runTimerFrozen ? "Frozen" : "Running");
        ImGui::Text("Formatted time: %s", formatted);
    }

    if (ImGui::CollapsingHeader("Session best", ImGuiTreeNodeFlags_DefaultOpen))
    {
        char formattedBest[32]{};
        core::FormatSessionBestTime(
            formattedBest,
            sizeof(formattedBest),
            snapshot.hasSessionBest,
            snapshot.sessionBestSeconds);
        ImGui::Text("Has session best: %s", BoolText(snapshot.hasSessionBest));
        if (snapshot.hasSessionBest)
        {
            ImGui::Text("Session best seconds: %.6f", snapshot.sessionBestSeconds);
        }
        else
        {
            ImGui::Text("Session best seconds: N/A");
        }
        ImGui::Text("Formatted session best: %s", formattedBest);
    }

    if (ImGui::CollapsingHeader("Persistence", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Save path: %s", snapshot.bestTimeSavePath);
        ImGui::Text("Load status: %s", snapshot.bestTimeLoadStatus);
        ImGui::Text("Save status: %s", snapshot.bestTimeSaveStatus);
        const std::filesystem::path layoutPath = editor::EditorLayoutPath();
        ImGui::TextWrapped(
            "Editor layout: %s",
            layoutPath.empty() ? "(unavailable)" : layoutPath.string().c_str());
    }

    if (ImGui::CollapsingHeader("Level Loading", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Runtime Level Path: %s", snapshot.runtimeLevelPath);
        ImGui::Text("Load Status: %s", snapshot.levelLoadStatus);
        ImGui::Text("Format Version: %d", snapshot.levelFormatVersion);
        ImGui::Text("Loaded Level ID: %s", snapshot.levelId);
    }

    if (ImGui::CollapsingHeader("Level Data", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Level ID: %s", snapshot.levelId);
        ImGui::Text(
            "Initial spawn: %.4f, %.4f, %.4f",
            snapshot.levelInitialSpawn.x,
            snapshot.levelInitialSpawn.y,
            snapshot.levelInitialSpawn.z);
        ImGui::Text("Kill plane Y: %.4f", snapshot.levelKillPlaneY);
        ImGui::Text("Static boxes (ground + elevated): %d", snapshot.levelStaticBoxCount);
        ImGui::Text("Elevated platforms: %d", snapshot.levelElevatedPlatformCount);
        ImGui::Text("Slopes: %d", snapshot.levelSlopeCount);
        ImGui::Text("Checkpoints: %d", snapshot.levelCheckpointCount);
        ImGui::Text("Hazards: %d", snapshot.levelHazardCount);
        ImGui::Text("Collectibles: %d", snapshot.levelCollectibleCount);
        ImGui::Text("Goal present: %s", BoolText(snapshot.levelHasGoal));
        ImGui::Text("Level Goals: %d", snapshot.levelGoalCount);
        ImGui::Text("Moving platform present: %s", BoolText(snapshot.levelHasMovingPlatform));
        ImGui::Text("Dynamic Boxes: %d", snapshot.levelDynamicBoxCount);
        ImGui::Text("Pressure Plates: %d", snapshot.levelPressurePlateCount);
        ImGui::Text("Doors: %d", snapshot.levelDoorCount);
        ImGui::Text(
            "Camera offset: %.4f, %.4f, %.4f",
            snapshot.levelCameraOffset.x,
            snapshot.levelCameraOffset.y,
            snapshot.levelCameraOffset.z);
        ImGui::Text("Camera FOV Y: %.4f", snapshot.levelCameraFieldOfViewY);
    }

    if (ImGui::CollapsingHeader("Player transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("position X: %.4f", snapshot.playerPosition.x);
        ImGui::Text("position Y: %.4f", snapshot.playerPosition.y);
        ImGui::Text("position Z: %.4f", snapshot.playerPosition.z);
    }

    if (ImGui::CollapsingHeader("Player movement", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("horizontal velocity: %.4f", snapshot.horizontalVelocity);
        ImGui::Text("vertical velocity: %.4f", snapshot.verticalVelocity);
        ImGui::Text("grounded: %s", BoolText(snapshot.grounded));
    }

    if (ImGui::CollapsingHeader("Player animation", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Character: %s", snapshot.playerCharacterIdentity);
        ImGui::Text("Model resolved: %s", BoolText(snapshot.playerCharacterModelResolved));
        ImGui::Text("Skeleton joints: %d", snapshot.playerSkeletonJointCount);
        ImGui::Text("Idle / Move / Jump bindings: %s",
            snapshot.playerAnimationBindingsResolved ? "Resolved" : "Missing / invalid");
        ImGui::Text("State: %s", snapshot.playerAnimationState);
        ImGui::Text("Clip: %s", snapshot.playerAnimationClip[0] ? snapshot.playerAnimationClip : "None");
        ImGui::Text("Playback: %.3f s", snapshot.playerAnimationTime);
        ImGui::Text("Cross-fade: %.2f", snapshot.playerAnimationBlend);
    }

    if (ImGui::CollapsingHeader("Input", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("moveX: %.0f", snapshot.moveX);
        ImGui::Text("jumpPressed: %s", BoolText(snapshot.jumpPressed));
        ImGui::Text("respawnPressed: %s", BoolText(snapshot.respawnPressed));
        ImGui::Text("restartPressed: %s", BoolText(snapshot.restartPressed));
        ImGui::Text("grabDropPressed: %s", BoolText(snapshot.grabDropPressed));
        ImGui::Text("Restart available: %s", BoolText(snapshot.restartAvailable));
        ImGui::Text("Restarted this frame: %s", BoolText(snapshot.restartedThisFrame));
    }

    if (ImGui::CollapsingHeader("Milestone 07 state", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("coyote elapsed: %.4f", snapshot.coyoteElapsed);
        ImGui::Text("coyote remaining: %.4f", CoyoteRemaining(snapshot));
        ImGui::Text("coyote available: %s", BoolText(snapshot.coyoteAvailable));
        ImGui::Text("jump buffer remaining: %.4f", snapshot.jumpBufferRemaining);
    }

    if (ImGui::CollapsingHeader("Milestone 07 constants", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("max horizontal speed: %.4f", snapshot.maxMoveSpeed);
        ImGui::Text("acceleration: %.4f", snapshot.acceleration);
        ImGui::Text("deceleration: %.4f", snapshot.deceleration);
        ImGui::Text("jump speed: %.4f", snapshot.jumpSpeed);
        ImGui::Text("gravity: %.4f", snapshot.gravity);
        ImGui::Text("coyote duration: %.4f", snapshot.coyoteDuration);
        ImGui::Text("jump buffer duration: %.4f", snapshot.jumpBufferDuration);
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
            "desired target: %.4f, %.4f, %.4f",
            snapshot.desiredTarget.x,
            snapshot.desiredTarget.y,
            snapshot.desiredTarget.z);
        ImGui::Text(
            "smoothed target: %.4f, %.4f, %.4f",
            snapshot.smoothedTarget.x,
            snapshot.smoothedTarget.y,
            snapshot.smoothedTarget.z);
        ImGui::Text("horizontal dead zone: %.4f", snapshot.horizontalDeadZone);
        ImGui::Text("vertical dead zone: %.4f", snapshot.verticalDeadZone);
        ImGui::Text("follow sharpness: %.4f", snapshot.followSharpness);
    }

    if (ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Jolt initialized: %s", BoolText(snapshot.physicsInitialized));
        ImGui::Text("static greybox bodies: %d", snapshot.staticBodyCount);
        ImGui::Text("authored Dynamic Box bodies: %d", snapshot.physicsDynamicBoxCount);
        ImGui::Text("Pressure Plates: %d", snapshot.physicsPressurePlateCount);
        ImGui::Text("Active Pressure Plates: %d", snapshot.physicsActivePressurePlateCount);
        ImGui::Text("Doors: %d", snapshot.physicsDoorCount);
        ImGui::Text("Door bodies: %d", snapshot.physicsDoorBodyCount);
        ImGui::Text("Desired-open Doors: %d", snapshot.physicsDesiredOpenDoorCount);
        ImGui::Text("first Dynamic Box valid: %s", BoolText(snapshot.dynamicTestBodyValid));
        ImGui::Text("grab target: %s (%d)", BoolText(snapshot.grabHasTarget), snapshot.grabTargetIndex);
        ImGui::Text("grab carrying: %s (%d)", BoolText(snapshot.grabCarrying), snapshot.grabCarriedIndex);
        ImGui::Text(
            "first Dynamic Box position: %.4f, %.4f, %.4f",
            snapshot.physicsTestBoxPosition.x,
            snapshot.physicsTestBoxPosition.y,
            snapshot.physicsTestBoxPosition.z);
    }

    if (ImGui::CollapsingHeader("Player Physics / Character", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
            "CharacterVirtual initialized: %s",
            BoolText(snapshot.characterVirtualInitialized));
        ImGui::Text("Character inner body active: %s", BoolText(snapshot.characterInnerBodyActive));
        ImGui::Text("physical Player X: %.4f", snapshot.playerPosition.x);
        ImGui::Text("physical Player Y: %.4f", snapshot.playerPosition.y);
        ImGui::Text("physical Player Z: %.4f", snapshot.playerPosition.z);
        ImGui::Text("horizontal velocity: %.4f", snapshot.horizontalVelocity);
        ImGui::Text("vertical velocity: %.4f", snapshot.verticalVelocity);
        ImGui::Text("supported/grounded: %s", BoolText(snapshot.grounded));
        ImGui::Text("ground support: %s", snapshot.playerGroundSupport);
        ImGui::Text("active contacts: %d", snapshot.playerContactCount);
        ImGui::Text("dynamic contact this frame: %s", BoolText(snapshot.dynamicContact));
        ImGui::Text("support body kind: %s", snapshot.supportBodyKind);
        ImGui::Text(
            "world velocity: %.4f, %.4f, %.4f",
            snapshot.playerWorldVelocity.x,
            snapshot.playerWorldVelocity.y,
            snapshot.playerWorldVelocity.z);
        ImGui::Text("relative horizontal velocity: %.4f", snapshot.horizontalVelocity);
        ImGui::Text("position finite: %s", BoolText(snapshot.playerPositionFinite));
        ImGui::Text("velocity finite: %s", BoolText(snapshot.playerVelocityFinite));
        ImGui::Text(
            "ground velocity: %.4f, %.4f, %.4f",
            snapshot.groundVelocity.x,
            snapshot.groundVelocity.y,
            snapshot.groundVelocity.z);
        ImGui::Text("supporting ground moving: %s", BoolText(snapshot.supportingGroundMoving));
        ImGui::Text(
            "ground normal: %.4f, %.4f, %.4f",
            snapshot.groundNormal.x,
            snapshot.groundNormal.y,
            snapshot.groundNormal.z);
        ImGui::Text("ground slope angle deg: %.2f", snapshot.groundSlopeAngleDegrees);
        ImGui::Text("current support walkable: %s", BoolText(snapshot.currentSupportWalkable));
        ImGui::Text("support classification: %s", snapshot.supportClassification);
    }

    if (ImGui::CollapsingHeader("Moving Platform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("initialized/valid: %s", BoolText(snapshot.movingPlatformValid));
        ImGui::Text("position X: %.4f", snapshot.movingPlatformPosition.x);
        ImGui::Text("position Y: %.4f", snapshot.movingPlatformPosition.y);
        ImGui::Text("position Z: %.4f", snapshot.movingPlatformPosition.z);
        ImGui::Text("velocity X: %.4f", snapshot.movingPlatformVelocity.x);
        ImGui::Text("velocity Y: %.4f", snapshot.movingPlatformVelocity.y);
        ImGui::Text("velocity Z: %.4f", snapshot.movingPlatformVelocity.z);
        ImGui::Text("direction: %.0f", snapshot.movingPlatformDirection);
        ImGui::Text("path min X: %.4f", snapshot.movingPlatformPathMinX);
        ImGui::Text("path max X: %.4f", snapshot.movingPlatformPathMaxX);
        ImGui::Text("configured speed: %.4f", snapshot.movingPlatformSpeed);
    }

    if (ImGui::CollapsingHeader("Assets", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextUnformatted("Cooker/staging inventory (not drawn in Level 01):");
        ImGui::Text("logical id: %s", snapshot.testTextureLogicalId);
        ImGui::Text("runtime path: %s", snapshot.testTextureRuntimeRelativePath);
        ImGui::Text("canonical scene instance: no");
        ImGui::Separator();
        ImGui::Text("Static Model");
        ImGui::Text("id: %s", snapshot.testModelLogicalId);
        ImGui::Text("canonical scene instance: no");
        ImGui::Separator();
        ImGui::Text("Blender Authored Model");
        ImGui::Text("id: %s", snapshot.authoredModelLogicalId);
        ImGui::Text("canonical scene instance: no");
        ImGui::Separator();
        ImGui::Text("Textured GLB Model");
        ImGui::Text("id: %s", snapshot.texturedModelLogicalId);
        ImGui::Text("canonical scene instance: no");
        ImGui::TextWrapped(
            "loaded/fallback flags stay false because M44 does not load these into the renderer.");
    }

    if (ImGui::CollapsingHeader("Respawn / Checkpoint", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Active checkpoint: %s", snapshot.activeCheckpointLabel);
        ImGui::Text(
            "Respawn position: %.4f, %.4f, %.4f",
            snapshot.respawnPosition.x,
            snapshot.respawnPosition.y,
            snapshot.respawnPosition.z);
        ImGui::Text("Player Y: %.4f", snapshot.playerPosition.y);
        ImGui::Text("Kill plane Y: %.4f", snapshot.killPlaneY);
        ImGui::Text("Death count: %d", snapshot.deathCount);
        ImGui::Text("Last respawn reason: %s", snapshot.lastRespawnReason);
        ImGui::Separator();
        ImGui::Text("Checkpoint 1");
        ImGui::Text("Inside: %s", BoolText(snapshot.checkpoint1Inside));
        ImGui::Text("State: %s", snapshot.checkpoint1VisualState);
        ImGui::Separator();
        ImGui::Text("Checkpoint 2");
        ImGui::Text("Inside: %s", BoolText(snapshot.checkpoint2Inside));
        ImGui::Text("State: %s", snapshot.checkpoint2VisualState);
    }

    if (ImGui::CollapsingHeader("Hazards", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Inside hazard: %s", snapshot.insideHazardLabel);
        ImGui::Text(
            "Hazard contact this frame: %s",
            BoolText(snapshot.hazardContactThisFrame));
        ImGui::Text("Health: %d / %d", snapshot.currentHealth, snapshot.maxHealth);
    }

    if (ImGui::CollapsingHeader("Collectibles", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text(
            "Collectibles: %d / %d",
            snapshot.collectedCount,
            snapshot.levelCollectibleCount);
        for (int index = 0; index < snapshot.levelCollectibleCount; ++index)
        {
            const std::size_t item = static_cast<std::size_t>(index);
            ImGui::Separator();
            ImGui::Text("Collectible %d", index + 1);
            const bool collected =
                item < snapshot.collectibleCollected.size() && snapshot.collectibleCollected[item] != 0;
            ImGui::Text("State: %s", collected ? "Collected" : "Available");
            const bool inside =
                item < snapshot.collectibleInside.size() && snapshot.collectibleInside[item] != 0;
            ImGui::Text("Inside: %s", BoolText(inside));
        }
        ImGui::Separator();
        ImGui::Text("Collected this frame: %s", snapshot.collectedThisFrameLabel);
    }

    if (ImGui::CollapsingHeader("Level Goal", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Level completed: %s", BoolText(snapshot.levelCompleted));
        ImGui::Text("Run complete: %s", BoolText(snapshot.runComplete));
        ImGui::Text("Run complete time: %.3f", snapshot.runCompleteFinalSeconds);
        ImGui::Text("Play Again pending: %s", BoolText(snapshot.playAgainPending));
        ImGui::Text("Play Again failed: %s", BoolText(snapshot.playAgainFailed));
        ImGui::Text("Goal count: %d", snapshot.levelGoalCount);
        ImGui::Text("Goal marker: two-post gate (Gameplay); AABB is Editor-only translucent");
        ImGui::Text(
            "First goal center: %.4f, %.4f, %.4f",
            snapshot.goalCenter.x,
            snapshot.goalCenter.y,
            snapshot.goalCenter.z);
        ImGui::Text(
            "First goal size: %.4f, %.4f, %.4f",
            snapshot.goalSize.x,
            snapshot.goalSize.y,
            snapshot.goalSize.z);
        ImGui::Text("Player inside any goal: %s", BoolText(snapshot.playerInsideGoal));
        ImGui::Text("Transition pending: %s", BoolText(snapshot.levelTransitionPending));
        ImGui::Text("Transition failed: %s", BoolText(snapshot.levelTransitionFailed));
        ImGui::Text("Destination: %s", snapshot.levelTransitionDestination);
        if (snapshot.levelTransitionFailure[0] != '\0')
        {
            ImGui::TextWrapped("Transition error: %s", snapshot.levelTransitionFailure);
        }
    }

#if defined(PLATFORMER_ENABLE_LEVEL_AUTHORING)
    if (ImGui::CollapsingHeader("Inventory (Test)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextUnformatted(
            "Development test harness. Calls production TryAdd / TryRemove / Clear.");
        ImGui::TextUnformatted(
            "Not the player HUD. Tab opens the separate Release-safe Inventory UI.");
        if (inventory == nullptr || inventory->Entries().empty())
        {
            ImGui::TextUnformatted("Inventory: empty");
        }
        else
        {
            for (const gameplay::InventoryEntry& entry : inventory->Entries())
            {
                const char* resolution = "missing";
                if (gameplayDefinitions != nullptr)
                {
                    const gameplay::GameplayReferenceResolution resolved =
                        gameplayDefinitions->Resolve(
                            gameplay::GameplayDefinitionReference{entry.itemId},
                            gameplay::GameplayDefinitionCategory::Item);
                    resolution = gameplay::GameplayReferenceStatusName(resolved.status);
                }
                ImGui::Text(
                    "%s x%d (%s)",
                    entry.itemId.c_str(),
                    entry.quantity,
                    resolution);
            }
        }

        static char itemIdBuffer[64] = "items/master_key";
        static int quantity = 1;
        static char lastResult[96] = "";
        ImGui::InputText("identity", itemIdBuffer, sizeof(itemIdBuffer));
        ImGui::InputInt("quantity", &quantity);
        const bool canMutate = inventory != nullptr && gameplayDefinitions != nullptr;
        ImGui::BeginDisabled(!canMutate);
        if (ImGui::Button("Add") && canMutate)
        {
            const gameplay::InventoryMutationStatus status =
                inventory->TryAdd(itemIdBuffer, quantity, *gameplayDefinitions);
            std::snprintf(
                lastResult,
                sizeof(lastResult),
                status == gameplay::InventoryMutationStatus::Ok
                    ? "Add ok"
                    : "Add failed: %s (unchanged)",
                gameplay::InventoryMutationStatusName(status));
        }
        ImGui::SameLine();
        if (ImGui::Button("Remove") && inventory != nullptr)
        {
            const bool ok = inventory->TryRemove(itemIdBuffer, quantity);
            std::snprintf(
                lastResult,
                sizeof(lastResult),
                ok ? "Remove ok" : "Remove failed (inventory unchanged)");
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear") && inventory != nullptr)
        {
            inventory->Clear();
            std::snprintf(lastResult, sizeof(lastResult), "Cleared");
        }
        ImGui::EndDisabled();
        if (lastResult[0] != '\0')
        {
            ImGui::TextUnformatted(lastResult);
        }
    }

    if (ImGui::CollapsingHeader("Equipment (Test)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextUnformatted("Equipped Item identities contribute additive Player stats.");
        if (equipment == nullptr)
        {
            ImGui::TextUnformatted("Equipment: unavailable");
        }
        else
        {
            for (const gameplay::EquipmentSlot slot : gameplay::kEquipmentSlots)
            {
                const std::string_view identity = equipment->GetEquipped(slot);
                const char* resolution = identity.empty() ? "empty" : "missing";
                if (!identity.empty() && gameplayDefinitions != nullptr)
                {
                    const gameplay::GameplayReferenceResolution resolved =
                        gameplayDefinitions->Resolve(
                            gameplay::GameplayDefinitionReference{std::string(identity)},
                            gameplay::GameplayDefinitionCategory::Item);
                    resolution = gameplay::GameplayReferenceStatusName(resolved.status);
                    if (resolved.definition != nullptr)
                    {
                        const auto& item = resolved.definition->item;
                        ImGui::Text("  model: %s",
                            item.worldModelIdentity.empty() ? "None" : item.worldModelIdentity.c_str());
                        if (item.equipmentAttachment.has_value())
                        {
                            ImGui::Text("  joint: %s (runtime-resolved; rendered when model + joint resolve)",
                                item.equipmentAttachment->jointName.c_str());
                        }
                        else ImGui::TextUnformatted("  joint: None; rendered: no");
                    }
                }
                ImGui::Text(
                    "%s: %s (%s)",
                    gameplay::EquipmentSlotName(slot).data(),
                    identity.empty() ? "(empty)" : identity.data(),
                    resolution);
            }
        }
        ImGui::TextUnformatted(
            "Legacy Item Pickup token key/1 maps to items/master_key. Canonical files stay readable.");
    }

    if (ImGui::CollapsingHeader("Player Character Stats", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (playerCharacterStats == nullptr)
        {
            ImGui::TextUnformatted("Player character stats unavailable.");
        }
        else
        {
            ImGui::Text("Definition: %s", playerCharacterStats->characterIdentity.c_str());
            ImGui::Text(
                "Resolution: %s",
                gameplay::GameplayReferenceStatusName(playerCharacterStats->characterResolution));
            ImGui::TextUnformatted("Stat                 Base       Equipment    Effective");
            for (std::size_t index = 0; index < gameplay::kGameplayStatCount; ++index)
            {
                const auto stat = static_cast<gameplay::GameplayStatId>(index);
                const gameplay::RuntimeCharacterStat& value = playerCharacterStats->values[index];
                ImGui::Text(
                    "%-18s %10.3f %10.3f %10.3f",
                    gameplay::GameplayStatName(stat).data(),
                    value.base,
                    value.equipmentAdditive,
                    value.effective);
            }
        }
    }

    DrawGameplayDefinitionsInspection();
#else
    (void)inventory;
    (void)equipment;
    (void)gameplayDefinitions;
    (void)playerCharacterStats;
#endif

    ImGui::End();
}
}

#else

namespace ui
{
void DrawDebugMetrics(
    const DebugMetricsSnapshot&,
    float,
    float,
    bool,
    bool,
    bool*,
    gameplay::Inventory*,
    gameplay::Equipment*,
    const gameplay::GameplayDefinitionRegistry*,
    const gameplay::PlayerCharacterStats*) {}
}

#endif
