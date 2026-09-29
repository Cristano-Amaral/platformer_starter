#include "render/LevelCharacters.h"
#include "core/Application.h"
#include "render/Renderer.h"
#include "platform/RuntimePaths.h"
#include "gameplay/GameplayDefinitionFile.h"
#include "gameplay/Player.h"
#include "editor/AuthoredObjectLifecycle.h"
#include "editor/EditorGizmo.h"
#include "editor/EditorHierarchy.h"
#include "editor/EditorPicking.h"
#include "world/LevelFile.h"
#include "world/LevelWriter.h"

#include <raylib.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <vector>

namespace core
{
// Arranges only fixture state and calls actual private production commands.
// No reset or frame-update logic is reimplemented in this accessor.
struct ApplicationLifecycleTestAccess
{
    static bool Configure(Application& app, const world::LevelDefinition& level,
        const gameplay::GameplayDefinitionRegistry& registry)
    {
        app.levelDefinition = level;
        app.gameplayDefinitions = registry;
        if (!app.physicsWorld.Initialize(level)
            || !app.physicsWorld.InitializePlayer(level.initialSpawnVisualCenter, app.player.Size())) return false;
        app.respawnState.respawnPosition = {4, 2, 0};
        app.respawnState.activeCheckpointIndex = 0;
        app.respawnState.deathCount = 3;
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        app.renderer.SetPlayerRuntimeHealth(&app.playerHealth);
        app.RefreshPlayerCharacterStats();
        app.playerHealth.ApplyDamage({app.playerHealth.Current() - (41)});
        app.runTimerState.elapsedSeconds = 12;
        app.inventory.TryAdd("items/master_key", 1, app.gameplayDefinitions);
        app.physicsWorld.UpdateMovingPlatform(0.5f);
        app.levelCharacters.Rebuild(app.levelDefinition.characters, app.gameplayDefinitions, platform::RuntimeAssetRoot());
        return true;
    }
    static bool AttackRegression(Application& app)
    {
        auto& bindings = app.gameplayDefinitions.FindMutable("characters/player")->character.animations;
        input::InputState request;
        request.attackPressed = true;
        for (const char* assignment : {"", "animations/missing", "animations/humanoid_jump"})
        {
            bindings.attack.clear(); bindings.attackAsset = assignment;
            app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
            app.playerHealth.Reset();
            const float before = app.playerHealth.Current();
            const bool valid = std::string_view(assignment) == "animations/humanoid_jump";
            std::vector<float> health;
            for (const auto& actor : app.levelCharacters.Npcs()) health.push_back(actor.health.Current());
            for (const auto& actor : app.levelCharacters.Enemies()) health.push_back(actor.health.Current());
            if (app.RequestPlayerAttack(request) != valid || app.playerHealth.Current() != before) return false;
            std::size_t index = 0;
            for (const auto& actor : app.levelCharacters.Npcs()) if (actor.health.Current() != health[index++]) return false;
            for (const auto& actor : app.levelCharacters.Enemies()) if (actor.health.Current() != health[index++]) return false;
            if (valid && app.RequestPlayerAttack(request)) return false;
            app.AdvanceLevelCharacters(10, false);
            if (app.renderer.PlayerAttackActive()) return false;
        }
        auto* playerDefinition = app.gameplayDefinitions.FindMutable("characters/player");
        const auto originalCharacter = playerDefinition->character;
        playerDefinition->character = app.gameplayDefinitions.Find("characters/retarget_target")->character;
        playerDefinition->character.type = gameplay::CharacterType::Player;
        playerDefinition->character.animations.attackAsset = "animations/humanoid_jump";
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        if (app.RequestPlayerAttack(request) || app.playerHealth.AttackActive()) return false;
        playerDefinition->character.animations.attackAsset = "animations/retarget_move";
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        if (!app.RequestPlayerAttack(request)) return false;
        app.playerHealth.Reset();
        playerDefinition->character = originalCharacter;
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        if (!app.RequestPlayerAttack(request)) return false;
        app.ApplyPlayerRuntimeDamage({1});
        if (app.playerHealth.AttackActive() || !app.playerHealth.HitReactionActive() || !app.playerHealth.DamageFeedbackActive()) return false;
        app.playerHealth.Reset();
        if (!app.RequestPlayerAttack(request)) return false;
        app.PerformRespawn(gameplay::RespawnReason::Fall);
        if (app.playerHealth.AttackActive()) return false;
        if (!app.RequestPlayerAttack(request)) return false;
        app.ApplyPlayerRuntimeDamage({10000});
        if (app.playerHealth.AttackActive() || !app.playerHealth.Defeated()) return false;
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0, false);
        auto exercise = [&](auto actors) {
            for (auto& actor : actors) {
                // Resolve a fixture assignment through the actual presentation rebuild.
                if (!actor.instance->RequestAttack()) return false;
                const auto position = actor.position;
                const double phase = actor.phase;
                while (actor.health.AttackActive()) {
                    app.AdvanceLevelCharacters(1.0f / 60.0f, false);
                    if (actor.phase != phase || actor.position.x != position.x || actor.position.y != position.y || actor.position.z != position.z
                        || actor.instance->WorldTransform().position.x != position.x
                        || actor.instance->WorldTransform().position.y != position.y
                        || actor.instance->WorldTransform().position.z != position.z) return false;
                }
                app.AdvanceLevelCharacters(0.1f, false);
                if (actor.origin.patrolEnabled && actor.phase == phase) return false;
                if (!actor.instance->RequestAttack()) return false;
                actor.health.ApplyDamage({1});
                if (actor.health.AttackActive() || !actor.health.HitReactionActive() || !actor.health.DamageFeedbackActive()) return false;
                actor.health.Reset(); actor.instance->Advance(0);
                if (!actor.instance->RequestAttack()) return false;
                actor.health.ApplyDamage({10000});
                if (actor.health.AttackActive() || actor.instance->RequestAttack()) return false;
            }
            return true;
        };
        for (const auto& placement : app.levelDefinition.characters) {
            auto& animations = app.gameplayDefinitions.FindMutable(placement.definitionIdentity)->character.animations;
            animations.attackAsset = "animations/humanoid_jump";
            animations.hitReactionAsset = "animations/humanoid_jump";
        }
        app.levelCharacters.Rebuild(app.levelDefinition.characters, app.gameplayDefinitions, platform::RuntimeAssetRoot());
        if (!exercise(app.levelCharacters.Npcs()) || !exercise(app.levelCharacters.Enemies())) return false;
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        const auto count = app.levelCharacters.Instances().size();
        for (auto& actor : app.levelCharacters.Npcs()) if (!actor.instance->RequestAttack()) return false;
        for (auto& actor : app.levelCharacters.Enemies()) if (!actor.instance->RequestAttack()) return false;
        if (!app.RequestPlayerAttack(request)) return false;
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0, false);
        for (const auto& actor : app.levelCharacters.Npcs()) if (actor.health.AttackActive() || actor.instance->AttackActive()) return false;
        for (const auto& actor : app.levelCharacters.Enemies()) if (actor.health.AttackActive() || actor.instance->AttackActive()) return false;
        return !app.playerHealth.AttackActive() && count == app.levelCharacters.Instances().size();
    }
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    static void ClearCharacterApplyAuthority(Application& app) { app.levelEditorState.characterDatabase.preserveRuntimeDefinitions = false; }
    static bool ApplyAttackAuthoring(Application& app)
    {
        auto& state = app.levelEditorState.characterDatabase;
        gameplay::ParseGameplayDefinitionsResult fixture;
        fixture.status = gameplay::LoadGameplayDefinitionsStatus::Loaded;
        fixture.registry = app.gameplayDefinitions;
        if (!editor::ApplyLoadedCharacterDatabase(state, fixture)) return false;
        state.working.FindMutable("characters/player")->character.animations.attackAsset = "animations/humanoid_jump";
        editor::RefreshCharacterDatabaseDirty(state);
        if (!app.ApplyCharacterDatabasePreview() || !state.dirty) { std::fprintf(stderr, "ApplyAttack: apply failed/dirty %s\n", state.statusMessage.c_str()); return false; }
        input::InputState request; request.attackPressed = true;
        if (!app.RequestPlayerAttack(request)) { std::fprintf(stderr, "ApplyAttack: request failed %s model=%s reaction=%d duration=%f\n", app.renderer.PlayerAttackDiagnostic().c_str(), app.gameplayDefinitions.Find("characters/player")->character.worldModelIdentity.c_str(), app.playerHealth.HitReactionActive(), app.renderer.PlayerAttackDuration()); return false; }
        if (!app.ApplyCharacterDatabasePreview() || app.playerHealth.AttackActive()) return false;
        state.working.FindMutable("characters/player")->character.animations.attackAsset = "animations/missing";
        editor::RefreshCharacterDatabaseDirty(state);
        const auto savedFixture = std::filesystem::temp_directory_path() / "platformer_m118_character_apply.gameplay";
        const bool saved = editor::TrySaveCharacterDatabase(state, savedFixture, true) == editor::ItemDatabaseSaveStatus::Saved;
        std::error_code ignored; std::filesystem::remove(savedFixture, ignored);
        if (!saved || app.gameplayDefinitions.Find("characters/player")->character.animations.attackAsset != "animations/humanoid_jump") return false;
        app.levelEditorState.active = true;
        app.SetLevelEditorActive(false);
        const bool finalRequest = app.RequestPlayerAttack(request);
        if (!finalRequest) std::fprintf(stderr, "ApplyAttack: exit request failed %s model=%s reaction=%d duration=%f\n", app.renderer.PlayerAttackDiagnostic().c_str(), app.gameplayDefinitions.Find("characters/player")->character.worldModelIdentity.c_str(), app.playerHealth.HitReactionActive(), app.renderer.PlayerAttackDuration());
        return app.gameplayDefinitions.Find("characters/player")->character.animations.attackAsset == "animations/humanoid_jump" && finalRequest;
    }
#endif
    static bool StartPlayerAttack(Application& app)
    {
        app.gameplayDefinitions.FindMutable("characters/player")->character.animations.attackAsset = "animations/humanoid_jump";
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        app.playerHealth.Reset();
        input::InputState request; request.attackPressed = true;
        return app.RequestPlayerAttack(request);
    }
    static bool ExercisePlayerMaximum(Application& app)
    {
        auto item = gameplay::GameplayDefinition{};
        item.identity = "items/m114_health_fixture";
        item.category = gameplay::GameplayDefinitionCategory::Item;
        item.item = gameplay::MakeDefaultItemDefinition(item.identity);
        item.item.type = gameplay::ItemType::Equipment;
        item.item.equipmentSlot = gameplay::EquipmentSlot::Head;
        item.item.modifiers.push_back({gameplay::GameplayStatId::MaxHealth, 40.5f});
        if (app.gameplayDefinitions.Register(item).status != gameplay::RegisterGameplayDefinitionStatus::Registered) return false;
        app.RefreshPlayerCharacterStats();
        app.playerHealth.Reset();
        const float base = app.playerHealth.Maximum();
        app.playerHealth.ApplyDamage({10});
        const float before = app.playerHealth.Current();
        app.inventory.TryAdd(item.identity, 1, app.gameplayDefinitions);
        if (app.equipment.Equip(app.inventory, item.identity, app.gameplayDefinitions) != gameplay::EquipmentTransactionStatus::Ok) return false;
        app.RefreshPlayerCharacterStats(); // Same production synchronization as Inventory UI commands.
        const bool increased = app.playerHealth.Maximum() == base + 40.5f && app.playerHealth.Current() == before;
        for (const auto& actor : app.levelCharacters.Npcs())
            if (actor.health.Maximum() != gameplay::ResolveCharacterMaxHealth(
                    app.gameplayDefinitions.Find(actor.origin.definitionIdentity)->character)) return false;
        for (const auto& actor : app.levelCharacters.Enemies())
            if (actor.health.Maximum() != gameplay::ResolveCharacterMaxHealth(
                    app.gameplayDefinitions.Find(actor.origin.definitionIdentity)->character)) return false;
        app.playerHealth.ApplyHealing({1000});
        app.equipment.Unequip(app.inventory, gameplay::EquipmentSlot::Head, app.gameplayDefinitions);
        app.RefreshPlayerCharacterStats();
        const bool clamped = app.playerHealth.Maximum() == base && app.playerHealth.Current() == base;
        app.playerHealth.ApplyDamage({10});
        app.ResetGameplayAfterPlayAgain(); // Real New Run reset boundary, after world construction.
        return increased && clamped && !app.playerHealth.HitReactionActive() && !app.playerHealth.DamageFeedbackActive()
            && app.playerHealth.Current() == app.playerHealth.Maximum();
    }
    static bool FreshRunReactionReset(Application& app)
    {
        // Exercise real staged-level replacement; restore only this generated copy.
        // Canonical source and cooked assets are never written by this fixture.
        const auto path = platform::RuntimeAssetPath("levels/level_01.level");
        struct RestoreStagedFile
        {
            std::filesystem::path path;
            std::string original;
            bool existed;
            ~RestoreStagedFile()
            {
                if (existed) { std::ofstream output(path, std::ios::binary); output << original; }
                else { std::error_code error; std::filesystem::remove(path, error); }
            }
        } restore{path, {}, std::filesystem::exists(path)};
        if (restore.existed)
        {
            std::ifstream input(path, std::ios::binary);
            restore.original.assign(std::istreambuf_iterator<char>(input), {});
        }
        const auto authored = app.levelDefinition;
        std::filesystem::create_directories(path.parent_path());
        { std::ofstream output(path, std::ios::binary); output << world::SerializeLevelText(authored); }
        NonLethalDamageAll(app);
        if (!app.renderer.PlayerHitReactionActive()) return false;
        for (auto* instance : app.levelCharacters.Instances())
            if (instance && !instance->HitReactionActive()) return false;
        app.topLevelFlow = gameplay::TopLevelFlow::MainMenu;
        app.mainMenuState = {};
        gameplay::RequestPlayFromMainMenu(app.mainMenuState, app.topLevelFlow);
        app.TryFinishPendingFreshRun();
        return app.topLevelFlow == gameplay::TopLevelFlow::Gameplay && !app.mainMenuState.playFailed
            && app.levelCharacters.Instances().size() == authored.characters.size()
            && AllHealthRestored(app) && world::AuthoredLevelDataEqual(app.levelDefinition, authored);
    }
    static void NonLethalDamageAll(Application& app)
    {
        app.ApplyPlayerRuntimeDamage({1});
        for (auto& actor : app.levelCharacters.Npcs()) actor.health.ApplyDamage({1});
        for (auto& actor : app.levelCharacters.Enemies()) actor.health.ApplyDamage({1});
    }
    static bool InvalidPlayerReaction(Application& app)
    {
        auto& bindings = app.gameplayDefinitions.FindMutable("characters/player")->character.animations;
        const auto original = bindings;
        for (const char* identity : {"", "animations/missing"})
        {
            bindings.hitReactionAsset = identity;
            bindings.hitReaction = "MissingClip";
            app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
            app.playerHealth.Reset(); app.ApplyPlayerRuntimeDamage({1});
            if (app.renderer.PlayerHitReactionActive() || !app.playerHealth.DamageFeedbackActive()
                || app.playerHealth.Current() != app.playerHealth.Maximum() - 1
                || !app.renderer.PlayerAnimationBindingsResolved()) return false;
        }
        bindings = original;
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        app.playerHealth.Reset();
        return true;
    }
    static bool ActiveReactionManualReset(Application& app)
    {
        app.playerHealth.Reset();
        app.ApplyPlayerRuntimeDamage({1});
        for (auto& actor : app.levelCharacters.Npcs()) actor.health.ApplyDamage({1});
        for (auto& actor : app.levelCharacters.Enemies()) actor.health.ApplyDamage({1});
        const auto count = app.levelCharacters.Instances().size();
        if (!app.renderer.PlayerHitReactionActive()) return false;
        for (auto* instance : app.levelCharacters.Instances())
            if (instance && !instance->HitReactionActive()) return false;
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0.25f, false);
        if (app.levelCharacters.Instances().size() != count || !AllHealthRestored(app)) return false;
        for (const auto& actor : app.levelCharacters.Npcs())
            if (actor.position.x != actor.origin.position.x || actor.position.z != actor.origin.position.z) return false;
        for (const auto& actor : app.levelCharacters.Enemies())
            if (actor.position.x != actor.origin.position.x || actor.position.z != actor.origin.position.z) return false;
        return true;
    }
    static bool PatrolReactionRegression(Application& app)
    {
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0.0f, false); // Consume the reset presentation frame.
        const auto exercise = [&](auto actors, auto others) {
            if (actors.empty() || others.empty()) return false;
            auto& actor = actors[0];
            app.AdvanceLevelCharacters(0.1f, false);
            const auto position = actor.position;
            const double phase = actor.phase;
            const auto otherPosition = others[0].position;
            if (!actor.origin.patrolEnabled || actor.health.Defeated()
                || !actor.health.HitReactionAvailable()) return false;
            const auto damage = actor.health.ApplyDamage({1});
            if (damage.applied != 1 || !actor.instance->HitReactionActive()
                || !actor.instance->DamageFeedbackActive()) return false;
            // Match normal frames after the Runtime Health control accepts damage.
            for (int frame = 0; frame < 40; ++frame)
            {
                const bool reacting = actor.health.HitReactionActive();
                app.AdvanceLevelCharacters(1.0f / 60.0f, false);
                const auto rendered = actor.instance->WorldTransform().position;
                if (reacting && (actor.position.x != position.x || actor.position.y != position.y
                    || actor.position.z != position.z || actor.phase != phase
                    || rendered.x != position.x || rendered.y != position.y || rendered.z != position.z)) return false;
                if (actor.instance->HitReactionActive()
                    && (actor.instance->Mode() == render::CharacterInstanceMode::Unavailable
                        || actor.instance->PlaybackTime() != actor.health.HitReactionTime())) return false;
                if (!reacting)
                    return !actor.health.Defeated() && !actor.instance->HitReactionActive()
                        && actor.phase > phase && (actor.position.x != position.x || actor.position.y != position.y || actor.position.z != position.z)
                        && (others[0].position.x != otherPosition.x || others[0].position.z != otherPosition.z)
                        && actor.instance->Locomotion() == render::CharacterLocomotionState::Move;
            }
            return false;
        };
        const bool npc = exercise(app.levelCharacters.Npcs(), app.levelCharacters.Enemies());
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0.0f, false);
        const bool enemy = exercise(app.levelCharacters.Enemies(), app.levelCharacters.Npcs());
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0.0f, false);
        return npc && enemy;
    }
    static bool RetargetPatrolReactionRegression(Application& app)
    {
        const auto fixtureRegistry = app.gameplayDefinitions;
        for (const auto& placement : app.levelDefinition.characters)
        {
            auto& character = app.gameplayDefinitions.FindMutable(placement.definitionIdentity)->character;
            const auto type = character.type;
            character = app.gameplayDefinitions.Find("characters/retarget_target")->character;
            character.type = type;
            character.animations.hitReactionAsset = "animations/humanoid_jump";
        }
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0.0f, false);
        const auto invalid = [&](auto actors) {
            auto& actor = actors[0];
            const auto position = actor.position;
            actor.health.ApplyDamage({1});
            app.AdvanceLevelCharacters(1.0f / 60.0f, false);
            return !actor.health.HitReactionAvailable() && !actor.instance->HitReactionActive()
                && actor.instance->DamageFeedbackActive() && !actor.health.Defeated()
                && actor.instance->Mode() == render::CharacterInstanceMode::Retargeted
                && (actor.position.x != position.x || actor.position.y != position.y || actor.position.z != position.z);
        };
        const bool invalidNpc = invalid(app.levelCharacters.Npcs());
        const bool invalidEnemy = invalid(app.levelCharacters.Enemies());
        for (const auto& placement : app.levelDefinition.characters)
            app.gameplayDefinitions.FindMutable(placement.definitionIdentity)->character.animations.hitReactionAsset = "animations/retarget_clamp";
        const bool valid = PatrolReactionRegression(app);
        app.gameplayDefinitions = fixtureRegistry;
        app.PerformRespawn(gameplay::RespawnReason::Manual);
        app.AdvanceLevelCharacters(0.0f, false);
        return invalidNpc && invalidEnemy && valid;
    }
    static bool HitReactionRegression(Application& app)
    {
        app.playerHealth.Reset();
        app.ApplyPlayerRuntimeDamage({1});
        if (!app.renderer.PlayerHitReactionActive() || !app.playerHealth.DamageFeedbackActive()) return false;
        app.AdvanceLevelCharacters(0.1f, false);
        if (!app.renderer.PlayerHitReactionActive() || app.playerHealth.HitReactionTime() <= 0) return false;
        app.ApplyPlayerRuntimeDamage({1});
        if (app.playerHealth.HitReactionTime() != 0) return false;
        app.AdvanceLevelCharacters(0.5f, false);
        if (app.renderer.PlayerHitReactionActive()) return false;
        app.playerHealth.ApplyHealing({1});
        app.ApplyPlayerRuntimeDamage({0});
        app.ApplyPlayerRuntimeDamage({-1});
        if (app.renderer.PlayerHitReactionActive()) return false;
        app.ApplyPlayerHazardDamage(true, 0, true);
        if (!app.renderer.PlayerHitReactionActive()) return false;
        app.playerHealth.Reset();
        gameplay::ResetHazardContactState(app.hazardContact);
        return true;
    }
    static bool FeedbackRegression(Application& app)
    {
        app.playerHealth.Reset();
        app.ApplyPlayerRuntimeDamage({1});
        app.AdvanceLevelCharacters(0, false);
        if (!app.renderer.PlayerDamageFeedbackActive()) return false;
        auto npcs = app.levelCharacters.Npcs();
        auto enemies = app.levelCharacters.Enemies();
        if (npcs.empty() || enemies.empty()) return false;
        npcs[0].health.ApplyDamage({1});
        if (!npcs[0].instance->DamageFeedbackActive() || enemies[0].instance->DamageFeedbackActive()) return false;
        for (std::size_t i = 1; i < npcs.size(); ++i)
            if (npcs[i].instance->DamageFeedbackActive()) return false;
        app.AdvanceLevelCharacters(gameplay::kDamageFeedbackSeconds, false);
        if (app.renderer.PlayerDamageFeedbackActive() || npcs[0].instance->DamageFeedbackActive()) return false;
        enemies[0].health.ApplyDamage({1});
        if (!enemies[0].instance->DamageFeedbackActive() || npcs[0].instance->DamageFeedbackActive()
            || app.renderer.PlayerDamageFeedbackActive()) return false;
        for (std::size_t i = 1; i < enemies.size(); ++i)
            if (enemies[i].instance->DamageFeedbackActive()) return false;
        return true;
    }
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
    static bool ApplyFeedbackRegression(Application& app)
    {
        app.playerHealth.Reset();
        app.ApplyPlayerRuntimeDamage({1});
        for (auto& actor : app.levelCharacters.Npcs()) actor.health.ApplyDamage({1});
        for (auto& actor : app.levelCharacters.Enemies()) actor.health.ApplyDamage({1});
        const auto count = app.levelCharacters.Instances().size();
        const auto fixtureRegistry = app.gameplayDefinitions;
        app.SetLevelEditorActive(true);
        app.ApplyLevelEditorPreview();
        const bool appliedReset = AllHealthRestored(app) && app.levelCharacters.Instances().size() == count;
        app.SetLevelEditorActive(false);
        // Exit reloads the canonical catalog and clears instances. Restore only the
        // in-memory fixture definitions, then exercise the real next-frame Sync.
        app.gameplayDefinitions = fixtureRegistry;
        app.renderer.ReloadPlayerPresentationAssets(&app.gameplayDefinitions);
        app.AdvanceLevelCharacters(0.0f, false);
        if (!appliedReset) return false;
        if (app.playerHealth.HitReactionActive() || app.playerHealth.DamageFeedbackActive() || app.playerHealth.Current() != app.playerHealth.Maximum()
            || app.levelCharacters.Instances().size() != count) return false;
        for (auto* instance : app.levelCharacters.Instances())
            if (instance && (instance->HitReactionActive() || instance->DamageFeedbackActive())) return false;
        return true;
    }
#endif
    static bool DeathRegression(Application& app)
    {
        app.playerHealth.Reset();
        app.ApplyPlayerRuntimeDamage({app.playerHealth.Maximum()});
        if (!gameplay::PlayerDeathIsActive(app.playerDeath) || !app.playerHealth.Defeated() || app.playerHealth.HitReactionActive()) return false;
        app.AdvancePlayerDeath(0.5f);
        if (app.playerHealth.DamageFeedbackActive()) return false;
        const float remaining = app.playerDeath.remainingSeconds;
        app.ApplyPlayerRuntimeDamage({1});
        if (app.playerDeath.remainingSeconds != remaining || app.playerHealth.ApplyHealing({1}).accepted) return false;
        const int deaths = app.respawnState.deathCount;
        app.AdvancePlayerDeath(remaining);
        if (gameplay::PlayerDeathIsActive(app.playerDeath) || app.playerHealth.Defeated()
            || app.playerHealth.Current() != app.playerHealth.Maximum() || app.renderer.PlayerDamageFeedbackActive() || app.respawnState.deathCount != deaths + 1) return false;
        gameplay::ResetHazardContactState(app.hazardContact);
        for (int i = 0; i < 8; ++i)
        {
            app.ApplyPlayerHazardDamage(true, 1.0f, true);
        }
        if (!gameplay::PlayerDeathIsActive(app.playerDeath) || !app.playerHealth.Defeated() || app.playerHealth.HitReactionActive()) return false;
        app.AdvancePlayerDeath(gameplay::kPlayerDeathDelaySeconds);
        return !app.playerHealth.Defeated() && app.playerHealth.Current() == app.playerHealth.Maximum();
    }
    static bool IndependentDefeat(Application& app)
    {
        auto npcs = app.levelCharacters.Npcs();
        auto enemies = app.levelCharacters.Enemies();
        if (npcs.empty() || enemies.empty()) return false;
        const auto npcPosition = npcs[0].position;
        const auto enemyPosition = enemies[0].position;
        const auto secondNpc = npcs.size() > 1 ? npcs[1].position : npcPosition;
        const auto secondEnemy = enemies.size() > 1 ? enemies[1].position : enemyPosition;
        npcs[0].health.ApplyDamage({npcs[0].health.Maximum()});
        app.AdvanceLevelCharacters(0.25f, false);
        if (npcs[0].position.x != npcPosition.x || npcs[0].position.z != npcPosition.z
            || npcs[0].instance->Locomotion() != render::CharacterLocomotionState::Idle
            || (enemies[0].position.x == enemyPosition.x && enemies[0].position.z == enemyPosition.z)) return false;
        if (npcs.size() > 1 && (npcs[1].health.Defeated()
            || (npcs[1].position.x == secondNpc.x && npcs[1].position.z == secondNpc.z))) return false;
        enemies[0].health.ApplyDamage({enemies[0].health.Maximum()});
        const auto stoppedEnemy = enemies[0].position;
        app.AdvanceLevelCharacters(0.25f, false);
        if (enemies.size() > 1 && (enemies[1].health.Defeated()
            || (enemies[1].position.x == secondEnemy.x && enemies[1].position.z == secondEnemy.z))) return false;
        return enemies[0].position.x == stoppedEnemy.x && enemies[0].position.z == stoppedEnemy.z
            && !npcs[0].health.ApplyHealing({1}).accepted && !enemies[0].health.ApplyHealing({1}).accepted;
    }
    static void DamageAll(Application& app)
    {
        app.ApplyPlayerRuntimeDamage({app.playerHealth.Maximum()});
        for (auto& actor : app.levelCharacters.Npcs()) actor.health.ApplyDamage({actor.health.Maximum()});
        for (auto& actor : app.levelCharacters.Enemies()) actor.health.ApplyDamage({actor.health.Maximum()});
    }
    static bool AllDefeatedWithDeath(const Application& app)
    {
        if (!app.playerHealth.Defeated() || !gameplay::PlayerDeathIsActive(app.playerDeath)) return false;
        for (const auto& actor : app.levelCharacters.Npcs()) if (!actor.health.Defeated()) return false;
        for (const auto& actor : app.levelCharacters.Enemies()) if (!actor.health.Defeated()) return false;
        return true;
    }
    static bool AllHealthRestored(const Application& app)
    {
        if (app.playerHealth.Defeated() || app.playerHealth.HitReactionActive() || app.playerHealth.DamageFeedbackActive() || app.playerHealth.Current() != app.playerHealth.Maximum()) return false;
        for (const auto& actor : app.levelCharacters.Npcs())
            if (actor.health.Defeated() || actor.health.HitReactionActive() || actor.health.DamageFeedbackActive() || actor.instance->DamageFeedbackActive() || actor.health.Current() != actor.health.Maximum()) return false;
        for (const auto& actor : app.levelCharacters.Enemies())
            if (actor.health.Defeated() || actor.health.HitReactionActive() || actor.health.DamageFeedbackActive() || actor.instance->DamageFeedbackActive() || actor.health.Current() != actor.health.Maximum()) return false;
        return true;
    }
    static void ManualRespawn(Application& app) { app.PerformRespawn(gameplay::RespawnReason::Manual); }
    static void FallRespawn(Application& app) { app.PerformRespawn(gameplay::RespawnReason::Fall); }
    static void RestartRun(Application& app) { app.RestartRun(); }
    static void Advance(Application& app, float deltaSeconds) { app.AdvanceLevelCharacters(deltaSeconds, false); }
    static render::Renderer& Presentation(Application& app) { return app.renderer; }
    static void DamagePlayer(Application& app) { app.playerHealth.Reset(); app.ApplyPlayerRuntimeDamage({1}); }
    static const render::LevelCharacters& Characters(const Application& app) { return app.levelCharacters; }
    static const world::LevelDefinition& Authored(const Application& app) { return app.levelDefinition; }
    static physics::MovingPlatformState MovingPlatform(const Application& app) { return app.physicsWorld.GetMovingPlatform(); }
    static bool ManualPlayerRulesPreserved(const Application& app)
    {
        const auto position = app.player.Position();
        return position.x == 4 && position.y == 2 && position.z == 0
            && app.respawnState.activeCheckpointIndex == 0 && app.respawnState.deathCount == 3
            && app.playerHealth.Current() == app.playerHealth.Maximum() && app.runTimerState.elapsedSeconds == 12
            && app.inventory.GetQuantity("items/master_key") == 1;
    }
};
}

namespace
{
int failures = 0;
void Expect(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
bool Equal(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
struct Pixels { std::vector<Color> colors; int width = 0; };
Pixels Render(render::Renderer& renderer, RenderTexture2D target,
    const world::LevelDefinition& active, std::span<render::CharacterInstance* const> instances, bool screen = false, bool placeholders = false, bool showPlayer = false)
{
    renderer.SetCharacterInstances(instances);
    renderer.SetCharacterPlacements(placeholders ? std::span<const world::CharacterPlacementSpec>(active.characters)
        : std::span<const world::CharacterPlacementSpec>{});
    if (screen) BeginDrawing(); else BeginTextureMode(target);
    ClearBackground({17, 23, 31, 255});
    const gameplay::Player player{showPlayer ? core::Vec3{0,21,0} : core::Vec3{100,100,0}, world::kPlayerVisualSize};
    std::vector<std::uint8_t> collectibles(active.collectibles.size(), 1);
    std::vector<std::uint8_t> pickups(active.itemPickups.size(), 1);
    const render::CameraView camera{{0.0f, 21.0f, 10.0f}, {0.0f, 21.0f, 0.0f}, {0, 1, 0}, 40.0f};
    renderer.DrawWorld(player, {}, {}, camera, active, {}, {}, {}, {100,100,0}, {1,1,1}, {},
        false, false, collectibles, 0, pickups, -1, {}, {}, -1, "", 0, false, 0,
        false, 0, {}, {}, {}, {}, {}, false, true);
    if (!screen) EndTextureMode();
    Image image = screen ? LoadImageFromScreen() : LoadImageFromTexture(target.texture);
    if (screen) EndDrawing();
    Color* colors = LoadImageColors(image);
    Pixels result;
    result.width = image.width;
    if (colors) result.colors.assign(colors, colors + image.width * image.height);
    if (colors) UnloadImageColors(colors);
    UnloadImage(image);
    return result;
}
int ChangedHalf(const Pixels& a, const Pixels& b, bool left)
{
    int changed = 0;
    if (a.width != b.width || a.colors.size() != b.colors.size()) return 0;
    for (std::size_t i = 0; i < a.colors.size(); ++i)
    {
        if ((static_cast<int>(i % a.width) < a.width / 2) != left) continue;
        const auto x = a.colors[i], y = b.colors[i];
        if (x.r != y.r || x.g != y.g || x.b != y.b) ++changed;
    }
    return changed;
}
}

int main()
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "CharacterPlacementTest");
    if (!IsWindowReady()) return 1;
    {
        auto catalog = gameplay::LoadGameplayDefinitionsFile(PLATFORMER_GAMEPLAY_DEFINITIONS_SOURCE_PATH);
        auto& registry = catalog.registry;
        // Fixtures explicitly opt into reactions; manual canonical assignments must not
        // change the older locomotion/red-feedback-only rendering expectations.
        registry.FindMutable("characters/player")->character.animations.hitReaction.clear();
        registry.FindMutable("characters/player")->character.animations.hitReactionAsset.clear();
        auto loaded = world::LoadLevelFile(std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT) / "levels/level_01.level");
        Expect(loaded.status == world::LoadLevelFileStatus::Loaded, "canonical level load");
        const auto canonicalSource = loaded.level;
        auto canonical = canonicalSource;
        canonical.characters.clear(); // Fixtures must not depend on manual-test placements.
        const auto text = world::SerializeLevelText(canonical);
        const std::string records = "character -2 20 0 0 0 0 1 1 1 characters/player\n"
            "character 2 20 0 0 0 0 1 1 1 characters/player\n";
        auto parsed = world::ParseLevelText(text + records);
        Expect(parsed.status == world::LoadLevelFileStatus::Loaded && parsed.level.characters.size() == 2,
            "multiple Character records parse through production v1");
        auto active = parsed.level;
        const auto written = world::SerializeLevelText(active);
        const auto roundTrip = world::ParseLevelText(written);
        Expect(world::AuthoredLevelDataEqual(active, roundTrip.level), "authored deterministic round trip");
        Expect(written == world::SerializeLevelText(roundTrip.level), "canonical writer deterministic");
        Expect(written.find("handle") == std::string::npos && written.find("playback") == std::string::npos,
            "runtime state never serialized");
        Expect(world::CountLevelV1RecordLines(active) == world::CountLevelV1RecordLines(canonical) + 2,
            "Character consumes existing line budget");
        auto tooMany = active;
        tooMany.characters.resize(world::kMaxLevelLines, active.characters[0]);
        Expect(!world::IsWritableLevelDefinition(tooMany), "existing line guard also bounds Characters");
        for (const char* identity : {"characters/", "characters/Bad", "items/key", "characters/a/b", "characters/a b"})
            Expect(world::ParseLevelText(text + "character 0 0 0 0 0 0 1 1 1 " + identity + "\n").status
                == world::LoadLevelFileStatus::Invalid, "malformed Character reference rejected");
        Expect(world::ParseLevelText(text + "character 0 0 0 0 0 0 0 1 1 characters/player\n").status
            == world::LoadLevelFileStatus::Invalid, "nonpositive scale rejected");
        auto missing = world::ParseLevelText(text + "character 0 0 0 0 0 0 1 1 1 characters/absent\n");
        Expect(missing.status == world::LoadLevelFileStatus::Loaded
            && missing.level.characters[0].definitionIdentity == "characters/absent", "Missing identity preserved exactly");
        Expect(registry.Resolve({"characters/player"}, gameplay::GameplayDefinitionCategory::Character).status
            == gameplay::GameplayReferenceStatus::Resolved, "same authoritative registry resolves Character");

        render::LevelCharacters runtime;
        runtime.Rebuild(active.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances().size() == 2 && runtime.Instances()[0] && runtime.Instances()[1], "one instance per valid active placement");
        if (!runtime.Instances()[0] || !runtime.Instances()[1]) return 1;
        auto* a = runtime.Instances()[0]; auto* b = runtime.Instances()[1];
        const auto oldHandle = a->Handle();
        Expect(a->Handle() != b->Handle() && a->PoseAddress() != b->PoseAddress()
            && a->BoneMatricesAddress() != b->BoneMatricesAddress(), "same-definition occurrences have independent handles poses and matrices");
        Expect(Equal(a->WorldTransform().position, active.characters[0].position)
            && Equal(b->WorldTransform().position, active.characters[1].position), "authored model origins initialize runtime");
        Expect(a->Locomotion() == render::CharacterLocomotionState::Idle && a->PlaybackTime() == 0,
            "initial generic state deterministic Idle");
        Expect(a->Mode() == render::CharacterInstanceMode::Exact && b->Mode() == render::CharacterInstanceMode::Exact,
            "canonical Player identity is visual-only Exact; gameplay Player stays separate");

        render::Renderer renderer;
        RenderTexture2D target = LoadRenderTexture(512, 256);
        gameplay::RuntimeHealth playerRenderHealth;
        renderer.SetPlayerRuntimeHealth(&playerRenderHealth);
        const auto playerNormal = Render(renderer, target, active, {}, false, false, true);
        playerRenderHealth.ApplyDamage({1});
        const auto playerFlash = Render(renderer, target, active, {}, false, false, true);
        Expect(ChangedHalf(playerNormal, playerFlash, true) + ChangedHalf(playerNormal, playerFlash, false) > 20,
            "production Player draw consumes damage tint");
        playerRenderHealth.Reset();
        const auto playerRestored = Render(renderer, target, active, {}, false, false, true);
        Expect(ChangedHalf(playerNormal, playerRestored, true) + ChangedHalf(playerNormal, playerRestored, false) == 0,
            "Player reset restores exact original rendering");
        const auto empty = Render(renderer, target, active, {});
        const auto both = Render(renderer, target, active, runtime.Instances());
        Expect(ChangedHalf(empty, both, true) > 20 && ChangedHalf(empty, both, false) > 20,
            "two authored instances reach actual production Renderer simultaneously at distinct transforms");
        gameplay::RuntimeHealth renderHealth;
        a->SetRuntimeHealth(&renderHealth);
        renderHealth.ApplyDamage({1});
        const auto flashed = Render(renderer, target, active, runtime.Instances());
        Expect(ChangedHalf(both, flashed, true) > 20 && ChangedHalf(both, flashed, false) == 0,
            "production material tint changes only damaged instance pixels");
        renderHealth.AdvanceDamageFeedback(gameplay::kDamageFeedbackSeconds);
        const auto restored = Render(renderer, target, active, runtime.Instances());
        Expect(ChangedHalf(both, restored, true) == 0 && ChangedHalf(both, restored, false) == 0,
            "expired flash restores original model materials and pixels");
        const auto bTime = b->PlaybackTime();
        a->Advance(0.3f);
        const auto animated = Render(renderer, target, active, runtime.Instances());
        Expect(ChangedHalf(both, animated, true) > 0 && ChangedHalf(both, animated, false) == 0
            && b->PlaybackTime() == bTime, "production animated pixels and playback are independent");
        runtime.Rebuild(active.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances().size() == 2 && runtime.Instances()[0]->Handle() != oldHandle,
            "rebuild replaces transient handles without duplicate accumulation");

        editor::StructuralIndexMap map;
        editor::ResetStructuralIndexMap(map, active);
        auto working = active;
        *editor::GetEditablePosition(working, {editor::EditorObjectKind::Character, 0}) = {-3,20,0};
        *editor::GetEditableRotation(working, {editor::EditorObjectKind::Character, 0}) = {0,35,0};
        *editor::GetEditableScale(working, {editor::EditorObjectKind::Character, 0}) = {1.1f,1.2f,1.3f};
        Expect(Equal(runtime.Instances()[0]->WorldTransform().position, active.characters[0].position),
            "workingCopy transform edits do not affect active runtime");
        world::LevelDefinition candidate;
        std::size_t discarded = 0;
        Expect(editor::PrepareLevelEditorApplyCandidate(working, map, candidate, discarded)
            && world::IsWritableLevelDefinition(candidate), "Apply candidate promotes valid Character edits");
        active = candidate;
        runtime.Rebuild(active.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        const auto& transform = runtime.Instances()[0]->WorldTransform();
        Expect(Equal(transform.position, active.characters[0].position)
            && Equal(transform.rotationDegrees, active.characters[0].rotationDegrees)
            && Equal(transform.scale, active.characters[0].scale), "full authored TRS initializes instance");
        const auto preview = world::CharacterPlacementVisualTransform(working.characters[0]);
        Expect(Equal(preview.position, transform.position) && Equal(preview.rotationDegrees, transform.rotationDegrees)
            && Equal(preview.scale, transform.scale), "editor ghost uses identical TRS with no Y offset");
        const auto savedPath = std::filesystem::temp_directory_path() / "platformer_m111_roundtrip.level";
        Expect(world::SaveLevelFile(savedPath, active).status == world::WriteLevelFileStatus::Saved, "real authored file Save");
        const auto reloaded = world::LoadLevelFile(savedPath);
        Expect(reloaded.status == world::LoadLevelFileStatus::Loaded && world::AuthoredLevelDataEqual(active, reloaded.level),
            "Save Reload preserves exact identities and transforms");
        const auto beforeReloadHandle = runtime.Instances()[0]->Handle();
        runtime.Rebuild(reloaded.level.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances()[0]->Handle() != beforeReloadHandle
            && Equal(runtime.Instances()[0]->WorldTransform().position, active.characters[0].position),
            "Reload creates new transient identity without changing placement semantics");
        std::filesystem::remove(savedPath);

        editor::ResetStructuralIndexMap(map, active);
        working = active;
        Expect(editor::DuplicateSelected(working, {editor::EditorObjectKind::Character, 0}).succeeded,
            "existing Duplicate semantics support Character");
        Expect(working.characters.size() == 3 && working.characters[2].position.x == working.characters[0].position.x + editor::kLifecycleDuplicateOffsetX,
            "Duplicate copies authored fields and offsets X");
        runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances().size() == 3 && runtime.Instances()[0]->Handle() != runtime.Instances()[2]->Handle(),
            "duplicate Apply creates independent instance");
        working = parsed.level;
        Expect(editor::IsValidSelection(working, {editor::EditorObjectKind::Character, 0})
            && editor::IsEditableSelection({editor::EditorObjectKind::Character, 0}), "Character uses existing selection routing");
        const auto hierarchy = editor::BuildHierarchyEntries(working);
        bool foundCharacter = false;
        for (const auto& entry : hierarchy) foundCharacter |= entry.selection == editor::EditorSelection{editor::EditorObjectKind::Character, 0};
        Expect(foundCharacter, "normal Hierarchy includes Character");
        const auto group = editor::CreateAuthoringGroupFromSelection(working,
            {editor::EditorObjectKind::Character, 0}, {{editor::EditorObjectKind::Character, 1}});
        Expect(group.succeeded && world::AuthoringGroupsAreValid(working), "Character preserves generic authoring group behavior");
        Expect(editor::DeleteSelected(working, {editor::EditorObjectKind::Character, 0}).succeeded, "existing Delete supports Character");
        Expect(working.authoringGroups.empty(), "delete remaps/dissolves undersized authoring group");
        runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        const auto survivor = Render(renderer, target, working, runtime.Instances());
        Expect(runtime.Instances().size() == 1 && Equal(runtime.Instances()[0]->WorldTransform().position, parsed.level.characters[1].position),
            "delete rebuild removes only deleted occurrence");
        Expect(ChangedHalf(empty, survivor, true) == 0 && ChangedHalf(both, survivor, false) == 0,
            "production pixels prove deleted occurrence gone and survivor unchanged");
        for (int i = 0; i < 3; ++i) runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances().size() == 1, "repeated rebuild never accumulates instances");

        working = active;
        editor::ResetStructuralIndexMap(map, active);
        editor::AddCharacter(working, {0,20,0});
        Expect(working.characters.back().definitionIdentity.empty(), "Add starts None without silent Player default");
        Expect(editor::PrepareLevelEditorApplyCandidate(working, map, candidate, discarded)
            && discarded == 1 && world::AuthoredLevelDataEqual(active, candidate), "provably new None discarded by established Apply rule");
        working.characters[0].definitionIdentity.clear();
        Expect(editor::PrepareLevelEditorApplyCandidate(working, map, candidate, discarded)
            && !world::IsWritableLevelDefinition(candidate), "existing cleared placement blocks Apply and protects active");
        working = active;
        working.characters[0].scale.x = std::numeric_limits<float>::quiet_NaN();
        Expect(!world::IsWritableLevelDefinition(working), "invalid working transform blocks promotion");

        working = parsed.level;
        working.characters[1].definitionIdentity = "characters/retarget_target";
        runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances()[0]->Mode() == render::CharacterInstanceMode::Exact
            && runtime.Instances()[1]->Mode() == render::CharacterInstanceMode::Retargeted, "Exact preferred and M109 fallback reused");
        const auto mixedBefore = Render(renderer, target, working, runtime.Instances());
        runtime.Advance(0.3f);
        const auto mixedAfter = Render(renderer, target, working, runtime.Instances());
        Expect(ChangedHalf(empty, mixedBefore, true) > 20 && ChangedHalf(empty, mixedBefore, false) > 20
            && ChangedHalf(mixedBefore, mixedAfter, true) > 0 && ChangedHalf(mixedBefore, mixedAfter, false) > 0,
            "production Renderer pixels prove simultaneous Exact and Retargeted animation");
        working.characters.push_back(missing.level.characters[0]);
        runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances().size() == 3 && runtime.Instances()[0] && runtime.Instances()[1]
            && !runtime.Instances()[2], "Missing reference never spawns as resolved and isolates valid placements");

        gameplay::GameplayDefinition staticDefinition;
        staticDefinition.identity = "characters/static_fixture";
        staticDefinition.category = gameplay::GameplayDefinitionCategory::Character;
        staticDefinition.character = gameplay::MakeDefaultCharacterDefinition(staticDefinition.identity);
        staticDefinition.character.worldModelIdentity = "models/test_static.glb";
        runtime.Clear();
        Expect(registry.Register(staticDefinition).status == gameplay::RegisterGameplayDefinitionStatus::Registered, "static fixture in existing registry");
        working.characters[0].definitionIdentity = staticDefinition.identity;
        runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
        Expect(runtime.Instances()[0]->HasModel() && runtime.Instances()[0]->IsStatic(), "static placement safe");
        runtime.Advance(1.0f);
        Expect(!Render(renderer, target, working, runtime.Instances()).colors.empty(), "static reaches production renderer safely");
        working.characters.resize(1);
        for (auto type : {gameplay::CharacterType::Player, gameplay::CharacterType::Enemy,
                gameplay::CharacterType::NPC, gameplay::CharacterType::Animal})
        {
            runtime.Clear();
            registry.FindMutable(staticDefinition.identity)->character.type = type;
            working.characters[0].patrolEnabled = true;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(runtime.Enemies().size() == (type == gameplay::CharacterType::Enemy ? 1 : 0)
                && runtime.Npcs().size() == (type == gameplay::CharacterType::NPC ? 1 : 0),
                "only resolved matching types receive independent NPC/Enemy actors");
            working.characters[0].patrolEnabled = false;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(runtime.Instances()[0]->IsStatic()
                && runtime.Instances()[0]->Locomotion() == render::CharacterLocomotionState::Idle,
                "disabled patrol keeps every Character Type Idle");
        }
        runtime.Clear();
        // M112 uses the production registry, authored lifecycle, instance owner,
        // and Renderer boundary above. Fixtures never alter the source catalog.
        auto exercisePatrolRuntime = [&](auto actors, gameplay::CharacterType runtimeType, const std::string& prefix)
        {
            render::Renderer patrolRenderer;
            SetWindowSize(64, 64);
            auto actorDefinition = *registry.Find("characters/player");
            actorDefinition.identity = (prefix + "_fixture");
            actorDefinition.character.type = runtimeType;
            actorDefinition.character.animations.hitReactionAsset = "animations/humanoid_jump";
            actorDefinition.character.animations.attackAsset = "animations/humanoid_jump";
            actorDefinition.character.hasBaseStat[0] = true;
            actorDefinition.character.baseStatValue[0] = 73.5f;
            Expect(registry.Register(actorDefinition).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
                "NPC/Enemy fixture registers through production registry");
            auto retargetActor = *registry.Find("characters/retarget_target");
            retargetActor.identity = (prefix + "_retarget");
            retargetActor.character.type = runtimeType;
            retargetActor.character.hasBaseStat[0] = true;
            retargetActor.character.baseStatValue[0] = 0;
            Expect(registry.Register(retargetActor).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
                "retarget NPC/Enemy fixture registers");
            Expect(!parsed.level.characters[0].patrolEnabled
                && parsed.level.characters[0].patrolDistance == 2 && parsed.level.characters[0].patrolSpeed == 1,
                "legacy M111 grammar defaults to disabled patrol");
            const std::string patrolRecord = "character -2 20 0 0 0 0 1 1 1 " + actorDefinition.identity;
            for (const char* suffix : {" npc_patrol 2 2 1", " npc_patrol true 2 1", " patrol 1 2 1",
                    " npc_patrol 1 nan 1", " npc_patrol 1 2 inf", " npc_patrol 1 0 1",
                    " npc_patrol 1 -1 1", " npc_patrol 1 101 1", " npc_patrol 1 2 0",
                    " npc_patrol 1 2 -1", " npc_patrol 1 2 21", " npc_patrol 0 0 1",
                    " npc_patrol 1 2", " npc_patrol 1 2 1 extra"})
                Expect(world::ParseLevelText(text + patrolRecord + suffix + "\n").status == world::LoadLevelFileStatus::Invalid,
                    "invalid patrol suffix rejects safely even when disabled");
            working = world::ParseLevelText(text + patrolRecord + " npc_patrol 1 2 1\n"
                "character 2 20 0 0 0 0 1 1 1 " + actorDefinition.identity + " npc_patrol 0 3 2\n").level;
            const auto patrolAuthored = working;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            const auto checkHealth = [&](auto mutableActors) {
                auto& a = mutableActors[0]; auto& b = mutableActors[1];
                const auto origin = a.position;
                Expect(a.instance->RequestAttack(), "NPC/Enemy attack resolves");
                const double attackPhase = a.phase;
                runtime.Advance(0.1f);
                Expect(Equal(origin, a.position) && Equal(origin, a.instance->WorldTransform().position)
                    && a.phase == attackPhase, "NPC/Enemy attack freezes actual patrol phase and render transform");
                runtime.Advance(10);
                Expect(Equal(origin, a.position) && a.phase == attackPhase, "expiry has no patrol catch-up");
                const float otherTime = b.instance->PlaybackTime();
                a.health.ApplyDamage({1});
                Expect(a.instance->HitReactionActive() && !b.instance->HitReactionActive()
                    && b.instance->PlaybackTime() == otherTime && a.health.DamageFeedbackActive(),
                    "accepted NPC/Enemy hit starts only exact instance and independent red flash");
                runtime.Advance(0.1f);
                Expect(Equal(origin, a.position) && a.instance->PlaybackTime() > 0,
                    "NPC/Enemy reaction samples clip and suspends patrol");
                a.health.ApplyDamage({1});
                runtime.Advance(0);
                Expect(a.instance->PlaybackTime() == 0, "repeated accepted hit restarts animation at frame zero");
                runtime.Advance(0.5f);
                Expect(!a.instance->HitReactionActive() && Equal(origin, a.position), "reaction finishes without patrol catch-up");
                runtime.Advance(0.1f);
                Expect(!Equal(origin, a.position) && a.instance->Locomotion() == render::CharacterLocomotionState::Move,
                    "NPC/Enemy patrol and normal animation resume");
                a.health.ApplyHealing({10}); a.health.ApplyDamage({0}); a.health.ApplyDamage({-1});
                Expect(!a.instance->HitReactionActive(), "Heal and rejected damage do not start actor reaction");
                Expect(mutableActors.size() == 2 && mutableActors[0].health.Maximum() == 73.5f
                    && mutableActors[1].health.Current() == 73.5f, "NPC/Enemy health resolves authored base maximum");
                mutableActors[0].health.ApplyDamage({80});
                Expect(mutableActors[0].health.Depleted() && mutableActors[1].health.Current() == 73.5f,
                    "same-definition actors own independent health");
                const auto stopped = mutableActors[0].position;
                runtime.Advance(0.25f);
                const auto withoutCharacters = Render(renderer, target, working, {});
                const auto defeatedCharacters = Render(renderer, target, working, runtime.Instances());
                Expect(Equal(mutableActors[0].position, stopped)
                    && ChangedHalf(withoutCharacters, defeatedCharacters, true) > 20
                    && ChangedHalf(withoutCharacters, defeatedCharacters, false) > 20,
                    "defeated NPC/Enemy remains rendered through production Renderer at stopped transform");
            };
            if (runtimeType == gameplay::CharacterType::NPC) checkHealth(runtime.Npcs());
            else checkHealth(runtime.Enemies());
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(actors(runtime)[0].health.Current() == 73.5f
                && gameplay::CharacterDefinitionsEqual(registry.Find(actorDefinition.identity)->character, actorDefinition.character),
                "Apply rebuild restores independent health without mutating CharacterDefinition");
            const auto patrolText = world::SerializeLevelText(working);
            auto patrolReload = world::ParseLevelText(patrolText);
            Expect(world::AuthoredLevelDataEqual(working, patrolReload.level)
                && patrolText == world::SerializeLevelText(patrolReload.level), "patrol round trip deterministic including disabled values");
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(actors(runtime).size() == 2 && runtime.Instances().size() == 2,
                "one NPC/Enemy actor and associated instance per eligible placement");
            if (actors(runtime).size() != 2) return 1;
            Expect(actors(runtime)[0].instance == runtime.Instances()[0]
                && actors(runtime)[1].instance == runtime.Instances()[1]
                && actors(runtime)[0].handle != actors(runtime)[1].handle, "distinct session actors borrow exactly their M111 instance");
            Expect(runtime.Instances()[0]->Mode() == render::CharacterInstanceMode::Exact
                && runtime.Instances()[0]->Locomotion() == render::CharacterLocomotionState::Move,
                "NPC/Enemy Move reuses Exact presentation");
            runtime.Advance(1);
            Expect(actors(runtime)[0].position.x == -1 && actors(runtime)[0].direction == 1,
                "first deterministic outbound step");
            Expect(Equal(actors(runtime)[1].position, working.characters[1].position)
                && runtime.Instances()[1]->Locomotion() == render::CharacterLocomotionState::Idle,
                "disabled same-definition NPC/Enemy independent and Idle at origin");
            runtime.Advance(1);
            Expect(Equal(actors(runtime)[0].position, actors(runtime)[0].endpointMax)
                && actors(runtime)[0].direction == -1 && actors(runtime)[0].rotationDegrees.y == -90,
                "exact positive endpoint reverses and faces negative travel direction");
            runtime.Advance(4);
            Expect(Equal(actors(runtime)[0].position, actors(runtime)[0].endpointMin)
                && actors(runtime)[0].direction == 1 && actors(runtime)[0].rotationDegrees.y == 90,
                "exact negative endpoint reverses deterministically");
            runtime.Advance(16);
            Expect(Equal(actors(runtime)[0].position, actors(runtime)[0].endpointMin)
                && world::AuthoredLevelDataEqual(working, patrolAuthored), "large steps wrap multiple periods without authored mutation");
            runtime.Advance(std::numeric_limits<float>::infinity());
            runtime.Advance(-1);
            Expect(Equal(actors(runtime)[0].position, actors(runtime)[0].endpointMin), "invalid simulation delta ignored");
            for (int rebuild = 0; rebuild < 3; ++rebuild)
            {
                const auto actorHandle = actors(runtime)[0].handle;
                const auto instanceHandle = runtime.Instances()[0]->Handle();
                runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
                Expect(actors(runtime).size() == 2 && runtime.Instances().size() == 2
                    && actors(runtime)[0].handle != actorHandle && runtime.Instances()[0]->Handle() != instanceHandle
                    && Equal(actors(runtime)[0].position, working.characters[0].position)
                    && runtime.Instances()[0]->PlaybackTime() == 0, "owner rebuild resets state without accumulation (not Application command coverage)");
                runtime.Advance(0.5f);
            }
            // M112 Correction 1: ordinary Gameplay R calls PerformRespawn(Manual),
            // not RestartRun. Exercise both actual Application commands plus the
            // exact frame-update boundary used before production rendering.
            for (const char* asset : {"models/player.glb", "models/humanoid_animations.glb", "models/retarget_source.glb", "models/retarget_target.glb", "shaders/world_lit.vs", "shaders/world_lit.fs"})
            {
                const auto destination = platform::RuntimeAssetPath(asset);
                std::filesystem::create_directories(destination.parent_path());
                std::filesystem::copy_file(std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT) / asset,
                    destination, std::filesystem::copy_options::overwrite_existing);
            }
            {
                auto restartAuthored = patrolAuthored;
                restartAuthored.characters[1].patrolEnabled = true;
                restartAuthored.characters[1].rotationDegrees = {15, 90, 0};
                restartAuthored.characters[1].scale = {1.2f, 1.3f, 1.4f};
                auto companionDefinition = actorDefinition;
                companionDefinition.identity = prefix + "_companion";
                companionDefinition.character.type = runtimeType == gameplay::CharacterType::NPC
                    ? gameplay::CharacterType::Enemy : gameplay::CharacterType::NPC;
                Expect(registry.Register(companionDefinition).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
                    "other runtime type fixture registers for production coexistence");
                auto companionPlacement = restartAuthored.characters[0];
                companionPlacement.definitionIdentity = companionDefinition.identity;
                companionPlacement.position = {8, 20, 0};
                companionPlacement.patrolSpeed = 0.5f;
                restartAuthored.characters.push_back(companionPlacement);
                auto companions = [&](const render::LevelCharacters& owner) {
                    return runtimeType == gameplay::CharacterType::NPC
                        ? owner.Enemies().size() : owner.Npcs().size();
                };
                auto companionAtOrigin = [&](const render::LevelCharacters& owner) {
                    if (runtimeType == gameplay::CharacterType::NPC) {
                        const auto& actor = owner.Enemies()[0];
                        return Equal(actor.position, companionPlacement.position) && actor.direction == 1
                            && actor.phase == companionPlacement.patrolDistance && actor.rotationDegrees.y == 90
                            && actor.instance->PlaybackTime() == 0;
                    }
                    const auto& actor = owner.Npcs()[0];
                    return Equal(actor.position, companionPlacement.position) && actor.direction == 1
                        && actor.phase == companionPlacement.patrolDistance && actor.rotationDegrees.y == 90
                        && actor.instance->PlaybackTime() == 0;
                };
                registry.FindMutable("characters/player")->character.animations.hitReactionAsset = "animations/humanoid_jump";
                core::Application app;
                using Access = core::ApplicationLifecycleTestAccess;
                Expect(Access::Configure(app, restartAuthored, registry), "production Application lifecycle fixture initializes");
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
                Expect(Access::ApplyFeedbackRegression(app), "real Editor Apply clears Player and instance feedback and restores existing full health with stable counts");
#endif
#if defined(PLATFORMER_ENABLE_DEBUG_UI)
                Expect(Access::ApplyAttackAuthoring(app), "real Character Apply promotes Attack without Save, survives editor exit and clears active Attack on reapply");
                Expect(Access::Configure(app, restartAuthored, registry), "restore after transient Character Apply");
                Access::ClearCharacterApplyAuthority(app);
#endif
                Expect(Access::AttackRegression(app), "production Attack input, expiry, patrol hold/resume, interruption, defeat and respawn");
                Expect(Access::Configure(app, restartAuthored, registry), "restore fixture after Attack lifecycle regression");
                Expect(Access::InvalidPlayerReaction(app), "invalid Player optional slot preserves Health flash and locomotion");
                Expect(Access::StartPlayerAttack(app), "Player input requests compatible Attack");
                auto& playerRenderer = Access::Presentation(app);
                Render(playerRenderer, target, restartAuthored, {}, false, false, true);
                Expect(std::string(playerRenderer.PlayerCurrentClipName()) == "Jump", "Player Attack selects the rendered action pose");
                Access::Advance(app, 10);
                Render(playerRenderer, target, restartAuthored, {}, false, false, true);
                Expect(std::string(playerRenderer.PlayerCurrentClipName()) == "Idle", "Player Attack returns rendered pose to locomotion");
                Access::DamagePlayer(app);
                Render(playerRenderer, target, restartAuthored, {}, false, false, true);
                Expect(std::string(playerRenderer.PlayerCurrentClipName()) == "Jump", "real Player damage overrides rendered animation slot");
                Access::Advance(app, 0.5f);
                Render(playerRenderer, target, restartAuthored, {}, false, false, true);
                Expect(std::string(playerRenderer.PlayerCurrentClipName()) == "Idle", "Player reaction expiry restores rendered locomotion selection");
                Expect(Access::RetargetPatrolReactionRegression(app), "Application retargeted NPC/Enemy reject incompatible Jump safely and hold/resume patrol with a compatible reaction");
                Expect(Access::PatrolReactionRegression(app), "Application frames hold NPC and Enemy patrol position and rendered transform throughout reaction, then resume");
                Expect(Access::HitReactionRegression(app), "production Player direct and Hazard reaction restart/finish/non-damage");
                Expect(Access::FeedbackRegression(app), "production Player NPC Enemy presentation independently reads accepted damage and expires");
                Expect(Access::ActiveReactionManualReset(app), "real Manual R clears active Player/NPC/Enemy reactions without duplicate instances");
                Expect(Access::ExercisePlayerMaximum(app), "Application equipment sync preserves/clamps Player health and New Run fills effective maximum");
                Expect(Access::DeathRegression(app), "production direct damage and legacy Hazard death converge once and real delay completion restores Alive/full");
                // Re-arrange the checkpoint/timer/inventory fixture after New Run's intentional resets.
                Expect(Access::Configure(app, restartAuthored, registry), "production fixture reconfigured after New Run regression");
                Expect(Access::IndependentDefeat(app), "production NPC and Enemy defeat independently stops patrol without revive");
                Access::ManualRespawn(app);
                Access::Advance(app, 0);
                const auto& appCharacters = Access::Characters(app);
                if (actors(appCharacters).size() != 2) return 1;
                const auto initialFacing0 = actors(appCharacters)[0].rotationDegrees;
                const auto initialFacing1 = actors(appCharacters)[1].rotationDegrees;
                for (int restart = 0; restart < 3; ++restart)
                {
                    Access::Advance(app, restart == 0 ? 2.5f : 2.0f);
                    Expect(!Equal(actors(appCharacters)[0].position, restartAuthored.characters[0].position)
                        && !Equal(actors(appCharacters)[1].position, restartAuthored.characters[1].position)
                        && actors(appCharacters)[0].direction == -1 && actors(appCharacters)[1].direction == -1,
                        "both actors observably away from origin and returning before real R command");
                    Expect(companions(appCharacters) == 1
                        && !Equal(appCharacters.Instances()[2]->WorldTransform().position, companionPlacement.position),
                        "NPC and Enemy coexist and both advance independently before real Manual Respawn");
                    const auto stopped = actors(appCharacters)[0].position;
                    auto* victim = appCharacters.Instances()[0];
                    Access::DamageAll(app);
                    Expect(Access::AllDefeatedWithDeath(app), "all defeated and production Player death active");
                    Access::Advance(app, 0.125f);
                    Expect(victim->DamageFeedbackActive(), "feedback remains active before real Manual respawn");
                    Expect(Equal(actors(appCharacters)[0].position, stopped) && victim != nullptr
                        && victim->Locomotion() == render::CharacterLocomotionState::Idle,
                        "defeated production actor stops and remains present");
                    const auto oldActorHandle = actors(appCharacters)[0].handle;
                    const auto oldInstanceHandle = appCharacters.Instances()[0]->Handle();
                    const auto platformBefore = Access::MovingPlatform(app);
                    Access::ManualRespawn(app);
                    Expect(Access::AllHealthRestored(app), "real PerformRespawn Manual restores Player NPC and Enemy health");
                    Expect(Access::ManualPlayerRulesPreserved(app), "ordinary R preserves Player checkpoint inventory timer and death-count authority");
                    const auto platformAfter = Access::MovingPlatform(app);
                    Expect(Equal(platformBefore.position, platformAfter.position) && platformBefore.direction == platformAfter.direction,
                        "ordinary R does not change pre-existing moving-platform behavior");
                    Access::Advance(app, 0.25f); // Same reset-frame update used by Application::Run.
                    Expect(actors(appCharacters).size() == 2 && appCharacters.Instances().size() == 3
                        && actors(appCharacters)[0].handle != oldActorHandle
                        && appCharacters.Instances()[0]->Handle() != oldInstanceHandle,
                        "real R replaces session actors/instances without accumulation");
                    Expect(companions(appCharacters) == 1 && companionAtOrigin(appCharacters),
                        "real Manual Respawn resets companion type position phase direction facing playback without accumulation");
                    for (std::size_t index = 0; index < 2; ++index)
                    {
                        const auto& actor = actors(appCharacters)[index];
                        Expect(Equal(actor.position, restartAuthored.characters[index].position)
                            && Equal(actor.instance->WorldTransform().position, restartAuthored.characters[index].position)
                            && actor.phase == restartAuthored.characters[index].patrolDistance && actor.direction == 1
                            && actor.locomotion == gameplay::CharacterPatrolLocomotionState::Move && actor.instance->PlaybackTime() == 0
                            && actor.instance->IsPlaying() && actor.instance->Mode() == render::CharacterInstanceMode::Exact,
                            "real R reset frame presents authored origin initial progress direction Move and playback");
                    }
                    Expect(Equal(actors(appCharacters)[0].rotationDegrees, initialFacing0)
                        && Equal(actors(appCharacters)[1].rotationDegrees, initialFacing1)
                        && world::AuthoredLevelDataEqual(Access::Authored(app), restartAuthored),
                        "runtime facing reset leaves authored TRS and each patrol configuration unchanged");
                    Access::Advance(app, 0.5f);
                    Expect(appCharacters.Instances()[2]->WorldTransform().position.x == 8.25f,
                        "companion resumes deterministically at independent speed");
                    Expect(std::abs(actors(appCharacters)[0].position.x + 1.5f) < 0.0001f
                        && std::abs(actors(appCharacters)[1].position.z + 1.0f) < 0.0001f,
                        "patrol resumes from deterministic initial state at independent authored speeds");
                }
                Access::NonLethalDamageAll(app);
                Access::RestartRun(app);
                Expect(Access::AllHealthRestored(app), "full RestartRun restores all runtime health");
                Access::Advance(app, 0.25f);
                Expect(Equal(actors(appCharacters)[0].position, restartAuthored.characters[0].position)
                    && Equal(actors(appCharacters)[1].position, restartAuthored.characters[1].position)
                    && actors(appCharacters)[0].direction == 1 && actors(appCharacters)[1].direction == 1
                    && appCharacters.Instances()[0]->PlaybackTime() == 0,
                    "full RestartRun also renders the reset origin without consuming that frame delta");
                Access::Advance(app, 0.5f);
                const auto retainedPosition = actors(appCharacters)[0].position;
                const auto retainedHandle = actors(appCharacters)[0].handle;
                Access::FallRespawn(app);
                Expect(Equal(actors(appCharacters)[0].position, retainedPosition)
                    && actors(appCharacters)[0].handle == retainedHandle,
                    "fall respawn retains existing narrower NPC/Enemy authority");
                Expect(world::AuthoredLevelDataEqual(Access::Authored(app), restartAuthored), "all real lifecycle commands leave authored data unchanged");
                Expect(Access::FreshRunReactionReset(app), "real Main Menu Play clears active reactions through staged replacement without duplicates");
            }
            const auto savedPatrolPath = std::filesystem::temp_directory_path() / "platformer_m113_roundtrip.level";
            Expect(world::SaveLevelFile(savedPatrolPath, working).status == world::WriteLevelFileStatus::Saved,
                "Save while NPC/Enemy moved stores only authored configuration");
            patrolReload = world::LoadLevelFile(savedPatrolPath);
            runtime.Rebuild(patrolReload.level.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(world::AuthoredLevelDataEqual(patrolReload.level, patrolAuthored)
                && Equal(actors(runtime)[0].position, working.characters[0].position), "Reload retains patrol and resets transient state");
            std::filesystem::remove(savedPatrolPath);
            auto pendingPatrol = working;
            editor::ResetStructuralIndexMap(map, working);
            pendingPatrol.characters[0].patrolEnabled = false;
            pendingPatrol.characters[0].patrolDistance = 5;
            pendingPatrol.characters[0].rotationDegrees = {12, 35, 18};
            Expect(actors(runtime)[0].origin.patrolEnabled && actors(runtime)[0].origin.patrolDistance == 2,
                "working patrol edits cannot affect active runtime before Apply");
            Expect(editor::PrepareLevelEditorApplyCandidate(pendingPatrol, map, candidate, discarded), "real Apply candidate accepts NPC/Enemy configuration");
            runtime.Rebuild(candidate.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            runtime.Advance(1);
            Expect(Equal(actors(runtime)[0].position, candidate.characters[0].position)
                && Equal(actors(runtime)[0].rotationDegrees, candidate.characters[0].rotationDegrees)
                && runtime.Instances()[0]->Locomotion() == render::CharacterLocomotionState::Idle, "disable Apply returns to authored origin rotation and Idle");
            pendingPatrol.characters[0].patrolSpeed = 0;
            Expect(!world::IsWritableLevelDefinition(pendingPatrol), "invalid working patrol blocks promotion");
            working.characters[0].rotationDegrees = {25, 90, 15};
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            runtime.Advance(1);
            Expect(std::abs(actors(runtime)[0].position.z + 1) < 0.001f
                && actors(runtime)[0].position.y == 20
                && Equal(working.characters[0].rotationDegrees, core::Vec3{25,90,15}), "projected authored local X drives horizontal patrol and never rewrites rotation");
            working.characters[0].rotationDegrees = {0,0,90};
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(actors(runtime)[0].axis.x == 1 && actors(runtime)[0].axis.z == 0, "vertical local X uses deterministic authored-yaw fallback");
            working = patrolAuthored;
            working.characters[1].patrolEnabled = true;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            runtime.Advance(0.5f);
            Expect(actors(runtime)[0].position.x == -1.5f && actors(runtime)[1].position.x == 3.0f
                && actors(runtime)[0].phase != actors(runtime)[1].phase,
                "two moving placements sharing one NPC/Enemy definition have independent phase and speed");
            working = patrolAuthored;
            render::LevelCharacters previewOwner{false};
            previewOwner.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            previewOwner.Advance(1);
            Expect(actors(previewOwner).empty() && Equal(previewOwner.Instances()[0]->WorldTransform().position, working.characters[0].position),
                "working preview remains authored-only despite enabled patrol");
            previewOwner.Clear();
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            const auto actorsBefore = Render(patrolRenderer, target, working, runtime.Instances());
            runtime.Advance(0.3f);
            const auto actorsMoved = Render(patrolRenderer, target, working, runtime.Instances());
            Expect(ChangedHalf(actorsBefore, actorsMoved, true) > 0, "moving NPC/Enemy reaches established production CharacterInstance renderer");
            Expect(editor::DeleteSelected(working, {editor::EditorObjectKind::Character, 0}).succeeded, "NPC/Enemy placement Delete uses existing authored lifecycle");
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            const auto actorSurvivor = Render(patrolRenderer, target, working, runtime.Instances());
            Expect(actors(runtime).size() == 1 && runtime.Instances().size() == 1
                && Equal(actors(runtime)[0].position, working.characters[0].position)
                && ChangedHalf(actorsBefore, actorSurvivor, false) == 0, "delete removes stale actor with unrelated survivor pixels preserved");
            working = patrolAuthored;
            working.characters[1].definitionIdentity = retargetActor.identity;
            working.characters[1].patrolEnabled = true;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(actors(runtime).size() == 2 && runtime.Instances()[1]->Mode() == render::CharacterInstanceMode::Retargeted,
                "NPC/Enemy Move supports M109 Retargeted");
            Expect(actors(runtime)[1].health.Maximum() == gameplay::kFallbackMaxHealth,
                "NPC and Enemy authored zero maximum uses identical legacy fallback");
            const auto retargetBefore = Render(patrolRenderer, target, working, runtime.Instances());
            runtime.Advance(0.3f);
            const auto retargetAfter = Render(patrolRenderer, target, working, runtime.Instances());
            Expect(ChangedHalf(retargetBefore, retargetAfter, true) > 0 && ChangedHalf(retargetBefore, retargetAfter, false) > 0,
                "simultaneous NPC/Enemy Exact Retargeted presentation animates through production draw");
            for (auto type : {gameplay::CharacterType::Player, gameplay::CharacterType::Enemy, gameplay::CharacterType::Animal, gameplay::CharacterType::NPC})
            {
                runtime.Clear();
                registry.FindMutable(staticDefinition.identity)->character.type = type;
                working = patrolAuthored;
                working.characters.resize(1);
                working.characters[0].definitionIdentity = staticDefinition.identity;
                runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
                Expect(actors(runtime).size() == (type == runtimeType ? 1 : 0), "only resolved NPC/Enemy type receives behavior");
                if (type == gameplay::CharacterType::Player || type == gameplay::CharacterType::Animal)
                    Expect(runtime.Npcs().empty() && runtime.Enemies().empty(), "generic Player/Animal placements remain presentation-only without health actors");
                Expect(runtime.Instances()[0]->IsStatic(), "static presentation safe for all types");
                runtime.Advance(1);
                Expect(runtime.Instances()[0]->Locomotion() == ((type == gameplay::CharacterType::NPC || type == gameplay::CharacterType::Enemy)
                    ? render::CharacterLocomotionState::Move : render::CharacterLocomotionState::Idle), "non-NPC/Enemy ignores stored patrol");
            }
            runtime.Clear();
            auto unavailableActor = actorDefinition;
            unavailableActor.identity = (prefix + "_unavailable");
            unavailableActor.character.worldModelIdentity = "models/not_found.glb";
            Expect(registry.Register(unavailableActor).status == gameplay::RegisterGameplayDefinitionStatus::Registered, "Unavailable NPC/Enemy fixture");
            working.characters[0].definitionIdentity = unavailableActor.identity;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            const auto unavailableBefore = Render(patrolRenderer, target, working, runtime.Instances(), false, true);
            runtime.Advance(1);
            const auto unavailableAfter = Render(patrolRenderer, target, working, runtime.Instances(), false, true);
            Expect(ChangedHalf(unavailableBefore, unavailableAfter, true) > 0,
                "established unavailable placeholder follows transient instance position");
            Expect(actors(runtime).size() == 1 && !runtime.Instances()[0]->HasModel(), "unavailable visuals do not break eligible actor lifecycle");
            runtime.Clear();
            auto unavailableAnimationActor = actorDefinition;
            unavailableAnimationActor.identity = (prefix + "_bad_clip");
            unavailableAnimationActor.character.animations.moveAsset = "animations/absent";
            Expect(registry.Register(unavailableAnimationActor).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
                "missing animation assignment remains authored fixture");
            working.characters[0].definitionIdentity = unavailableAnimationActor.identity;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            runtime.Advance(1);
            Expect(actors(runtime).size() == 1 && runtime.Instances()[0]->HasModel()
                && runtime.Instances()[0]->Mode() == render::CharacterInstanceMode::Unavailable,
                "unavailable Move animation retains safe model and NPC/Enemy state without embedded fallback");
            for (const char* identity : {"characters/absent", "characters/Bad", "characters/player"})
            {
                working.characters[0].definitionIdentity = identity;
                runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
                Expect(actors(runtime).empty(), "missing malformed or Player reference cannot gain NPC/Enemy behavior");
            }
            runtime.Clear();
            // Real production shadow resources and an elevated receiver isolate NPC
            // caster pixels from the original M111 unlit rendering regression.
            for (const char* shader : {"world_lit.vs", "world_lit.fs", "shadow_depth.vs", "shadow_depth.fs"})
            {
                const auto destination = platform::RuntimeAssetPath(std::string("shaders/") + shader);
                std::filesystem::create_directories(destination.parent_path());
                std::filesystem::copy_file(std::filesystem::path(PLATFORMER_SOURCE_ASSET_ROOT) / "shaders" / shader,
                    destination, std::filesystem::copy_options::overwrite_existing);
            }
            SetWindowSize(512, 256);
            patrolRenderer.LoadRuntimeAssets(&registry);
            working = patrolAuthored;
            working.elevatedPlatforms = {{{0, 19, 0}, {20, 1, 20}}};
            working.environment.directionalRayDirection = {-1, -1, 0};
            working.environment.directionalEnabled = true;
            working.environment.directionalIntensity = 1;
            working.environment.directionalShadowsEnabled = false;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            const auto receiverWithoutActors = Render(patrolRenderer, target, working, {}, true);
            const auto actorsWithoutShadow = Render(patrolRenderer, target, working, runtime.Instances(), true);
            working.environment.directionalShadowsEnabled = true;
            const auto receiverWithShadow = Render(patrolRenderer, target, working, {}, true);
            const auto actorsWithShadow = Render(patrolRenderer, target, working, runtime.Instances(), true);
            int casterPixels = 0;
            auto sameColor = [](Color a, Color b) { return a.r == b.r && a.g == b.g && a.b == b.b; };
            for (std::size_t pixel = 0; pixel < actorsWithShadow.colors.size(); ++pixel)
            {
                // Exclude character geometry and other world casters. Only newly
                // shadowed receiver pixels contributed by the NPC instances remain.
                if (sameColor(receiverWithoutActors.colors[pixel], actorsWithoutShadow.colors[pixel])
                    && sameColor(receiverWithoutActors.colors[pixel], receiverWithShadow.colors[pixel])
                    && !sameColor(actorsWithoutShadow.colors[pixel], actorsWithShadow.colors[pixel])) ++casterPixels;
            }
            Expect(casterPixels > 5, "production directional shadow pass receives NPC/Enemy CharacterInstance caster geometry");
            runtime.Advance(0.3f);
            const auto movingShadow = Render(patrolRenderer, target, working, runtime.Instances(), true);
            Expect(ChangedHalf(actorsWithShadow, movingShadow, true) > 0, "animated moving NPC/Enemy remains safe in shared lit and shadow draw path");
            runtime.Clear();
            patrolRenderer.SetCharacterInstances({});
            return 0;
        };
        Expect(exercisePatrolRuntime([](const render::LevelCharacters& owner) { return owner.Npcs(); },
            gameplay::CharacterType::NPC, "characters/npc") == 0, "NPC production contract ran to completion");
        Expect(exercisePatrolRuntime([](const render::LevelCharacters& owner) { return owner.Enemies(); },
            gameplay::CharacterType::Enemy, "characters/enemy") == 0, "Enemy production contract ran to completion");
        runtime.Clear();
        renderer.SetCharacterInstances({});
        UnloadRenderTexture(target);
        Expect(world::AuthoredLevelDataEqual(canonicalSource, loaded.level), "canonical authored data unchanged");
    }
    CloseWindow();
    std::printf("CharacterPlacementTest: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
