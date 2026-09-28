#include "render/LevelCharacters.h"
#include "render/Renderer.h"
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
    const world::LevelDefinition& active, std::span<render::CharacterInstance* const> instances)
{
    renderer.SetCharacterInstances(instances);
    renderer.SetCharacterPlacements({});
    BeginTextureMode(target);
    ClearBackground({17, 23, 31, 255});
    const gameplay::Player player{{100.0f, 100.0f, 0.0f}, world::kPlayerVisualSize};
    std::vector<std::uint8_t> collectibles(active.collectibles.size(), 1);
    std::vector<std::uint8_t> pickups(active.itemPickups.size(), 1);
    const render::CameraView camera{{0.0f, 21.0f, 10.0f}, {0.0f, 21.0f, 0.0f}, {0, 1, 0}, 40.0f};
    renderer.DrawWorld(player, {}, {}, camera, active, {}, {}, {}, {100,100,0}, {1,1,1}, {},
        false, false, collectibles, 0, pickups, -1, {}, {}, -1, "", 0, false, 0,
        false, 0, {}, {}, {}, {}, {}, false, true);
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
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
        const auto canonical = loaded.level;
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
        for (auto type : {gameplay::CharacterType::Player, gameplay::CharacterType::Enemy,
                gameplay::CharacterType::NPC, gameplay::CharacterType::Animal})
        {
            runtime.Clear();
            registry.FindMutable(staticDefinition.identity)->character.type = type;
            runtime.Rebuild(working.characters, registry, PLATFORMER_SOURCE_ASSET_ROOT);
            Expect(runtime.Instances()[0]->IsStatic()
                && runtime.Instances()[0]->Locomotion() == render::CharacterLocomotionState::Idle,
                "Character Type does not activate gameplay behavior");
        }
        runtime.Clear();
        renderer.SetCharacterInstances({});
        UnloadRenderTexture(target);
        Expect(world::AuthoredLevelDataEqual(canonical, loaded.level), "canonical authored data unchanged");
    }
    CloseWindow();
    std::printf("CharacterPlacementTest: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
