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
        app.playerHealth.currentHealth = 41;
        app.runTimerState.elapsedSeconds = 12;
        app.inventory.TryAdd("items/master_key", 1, app.gameplayDefinitions);
        app.physicsWorld.UpdateMovingPlatform(0.5f);
        app.levelCharacters.Rebuild(app.levelDefinition.characters, app.gameplayDefinitions, platform::RuntimeAssetRoot());
        return true;
    }
    static void ManualRespawn(Application& app) { app.PerformRespawn(gameplay::RespawnReason::Manual); }
    static void FallRespawn(Application& app) { app.PerformRespawn(gameplay::RespawnReason::Fall); }
    static void RestartRun(Application& app) { app.RestartRun(); }
    static void Advance(Application& app, float deltaSeconds) { app.AdvanceLevelCharacters(deltaSeconds, false); }
    static const render::LevelCharacters& Characters(const Application& app) { return app.levelCharacters; }
    static const world::LevelDefinition& Authored(const Application& app) { return app.levelDefinition; }
    static physics::MovingPlatformState MovingPlatform(const Application& app) { return app.physicsWorld.GetMovingPlatform(); }
    static bool ManualPlayerRulesPreserved(const Application& app)
    {
        const auto position = app.player.Position();
        return position.x == 4 && position.y == 2 && position.z == 0
            && app.respawnState.activeCheckpointIndex == 0 && app.respawnState.deathCount == 3
            && app.playerHealth.currentHealth == 41 && app.runTimerState.elapsedSeconds == 12
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
    const world::LevelDefinition& active, std::span<render::CharacterInstance* const> instances, bool screen = false, bool placeholders = false)
{
    renderer.SetCharacterInstances(instances);
    renderer.SetCharacterPlacements(placeholders ? std::span<const world::CharacterPlacementSpec>(active.characters)
        : std::span<const world::CharacterPlacementSpec>{});
    if (screen) BeginDrawing(); else BeginTextureMode(target);
    ClearBackground({17, 23, 31, 255});
    const gameplay::Player player{{100.0f, 100.0f, 0.0f}, world::kPlayerVisualSize};
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
        const auto empty = Render(renderer, target, active, {});
        const auto both = Render(renderer, target, active, runtime.Instances());
        Expect(ChangedHalf(empty, both, true) > 20 && ChangedHalf(empty, both, false) > 20,
            "two authored instances reach actual production Renderer simultaneously at distinct transforms");
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
            Expect(registry.Register(actorDefinition).status == gameplay::RegisterGameplayDefinitionStatus::Registered,
                "NPC/Enemy fixture registers through production registry");
            auto retargetActor = *registry.Find("characters/retarget_target");
            retargetActor.identity = (prefix + "_retarget");
            retargetActor.character.type = runtimeType;
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
            for (const char* asset : {"models/player.glb", "models/humanoid_animations.glb", "shaders/world_lit.vs", "shaders/world_lit.fs"})
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
                core::Application app;
                using Access = core::ApplicationLifecycleTestAccess;
                Expect(Access::Configure(app, restartAuthored, registry), "production Application lifecycle fixture initializes");
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
                    const auto oldActorHandle = actors(appCharacters)[0].handle;
                    const auto oldInstanceHandle = appCharacters.Instances()[0]->Handle();
                    const auto platformBefore = Access::MovingPlatform(app);
                    Access::ManualRespawn(app);
                    Expect(Access::ManualPlayerRulesPreserved(app), "ordinary R preserves Player checkpoint inventory health timer and death-count authority");
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
                Access::RestartRun(app);
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
