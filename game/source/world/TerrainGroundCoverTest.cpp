#include "editor/TerrainGroundCover.h"
#include "editor/TerrainPickerCard.h"
#include "world/TerrainGroundCover.h"
#include "world/TerrainSculpt.h"

#include "assets/RuntimePng.h"
#include "assets/RuntimePngCutout.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name);
        ++gFailures;
    }
}

world::TerrainSpec MakeTerrain()
{
    return world::MakeDefaultTerrain();
}

bool AddCover(world::TerrainSpec& terrain, const char* identity = "textures/grass.png")
{
    return world::TryAddTerrainGroundCoverEntry(terrain, identity);
}

world::TerrainGroundCoverStampRequest CellStamp(
    const world::TerrainSpec& terrain,
    int ix,
    int iz,
    int entryIndex,
    float radius)
{
    world::TerrainGroundCoverStampRequest request{};
    request.entryIndex = entryIndex;
    request.radius = radius;
    request.centerX =
        terrain.origin.x + (static_cast<float>(ix) + 0.5f) * world::TerrainGroundCoverCellSizeX(terrain);
    request.centerZ =
        terrain.origin.z + (static_cast<float>(iz) + 0.5f) * world::TerrainGroundCoverCellSizeZ(terrain);
    return request;
}

void OccupyAll(world::TerrainSpec& terrain, int entryIndex, float density)
{
    world::EnsureTerrainGroundCoverGrid(terrain);
    terrain.groundCoverEntries[static_cast<std::size_t>(entryIndex)].density = density;
    const std::uint16_t paint = world::PackTerrainGroundCoverPaintFromEntry(
        terrain.groundCoverEntries[static_cast<std::size_t>(entryIndex)]);
    const unsigned char quantum = world::QuantizeTerrainGroundCoverDensity(density);
    const unsigned char bit = world::TerrainGroundCoverEntryBit(entryIndex);
    const int cells = terrain.groundCoverResolutionX * terrain.groundCoverResolutionZ;
    for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
    {
        terrain.groundCoverCells[static_cast<std::size_t>(cellIndex)] = static_cast<unsigned char>(
            terrain.groundCoverCells[static_cast<std::size_t>(cellIndex)] | bit);
        const int slot = world::TerrainGroundCoverDensitySlot(cellIndex, entryIndex);
        terrain.groundCoverDensityQuanta[static_cast<std::size_t>(slot)] = quantum;
        terrain.groundCoverPaintParams[static_cast<std::size_t>(slot)] = paint;
    }
}

int OccupiedCells(const world::TerrainSpec& terrain, int entryIndex)
{
    int count = 0;
    const unsigned char bit = world::TerrainGroundCoverEntryBit(entryIndex);
    for (unsigned char cell : terrain.groundCoverCells)
    {
        if ((cell & bit) != 0)
        {
            ++count;
        }
    }
    return count;
}

bool SameInstances(
    const std::vector<world::TerrainGroundCoverInstance>& first,
    const std::vector<world::TerrainGroundCoverInstance>& second)
{
    if (first.size() != second.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        const world::TerrainGroundCoverInstance& a = first[index];
        const world::TerrainGroundCoverInstance& b = second[index];
        if (a.entryIndex != b.entryIndex || a.position.x != b.position.x || a.position.y != b.position.y
            || a.position.z != b.position.z || a.width != b.width || a.height != b.height
            || a.axisX.x != b.axisX.x || a.axisY.y != b.axisY.y || a.axisZ.z != b.axisZ.z)
        {
            return false;
        }
    }
    return true;
}

int OccupiedCount(const world::TerrainSpec& terrain)
{
    int count = 0;
    for (unsigned char cell : terrain.groundCoverCells)
    {
        if (cell != 0)
        {
            ++count;
        }
    }
    return count;
}

std::filesystem::path TestCheckerPngPath()
{
#if defined(PLATFORMER_TEST_CHECKER_PNG)
    return std::filesystem::path{PLATFORMER_TEST_CHECKER_PNG}.lexically_normal();
#else
    return {};
#endif
}

std::filesystem::path TestGroundCoverTuftPngPath()
{
#if defined(PLATFORMER_TEST_GROUND_COVER_TUFT_PNG)
    return std::filesystem::path{PLATFORMER_TEST_GROUND_COVER_TUFT_PNG}.lexically_normal();
#else
    return {};
#endif
}

void WriteBytes(const std::filesystem::path& path, const std::uint8_t* data, std::size_t size)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
}

void CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        source, destination, std::filesystem::copy_options::overwrite_existing);
}
}

int main()
{
    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(world::TerrainGroundCoverIsAbsent(terrain), "default Terrain has no ground cover");
        Expect(AddCover(terrain), "palette add");
        Expect(terrain.groundCoverEntries.size() == 1, "one palette entry");
        Expect(terrain.groundCoverEntries[0].textureIdentity == "textures/grass.png", "texture identity");
        Expect(
            world::TerrainGroundCoverResolutionIsValid(terrain.groundCoverResolutionX)
                && world::TerrainGroundCoverResolutionIsValid(terrain.groundCoverResolutionZ),
            "add creates a compact grid");
        Expect(AddCover(terrain, "textures/grass.png"), "duplicate texture is a second slot");
        Expect(terrain.groundCoverEntries.size() == 2, "duplicate policy allows two slots");
        Expect(AddCover(terrain, "textures/clover.png"), "second distinct texture");
        Expect(AddCover(terrain, "textures/moss.png"), "third distinct texture");
        Expect(editor::TerrainGroundCoverPaletteIsFull(terrain), "palette reports full at 4");
        Expect(!AddCover(terrain, "textures/extra.png"), "palette maximum rejects a fifth entry");
        Expect(terrain.groundCoverEntries.size() == 4, "full palette stays at 4");
        Expect(
            world::TryRemoveTerrainGroundCoverEntry(terrain, 0),
            "remove remaps occupancy bits");
        Expect(terrain.groundCoverEntries.size() == 3, "removal drops one entry");
        Expect(terrain.groundCoverEntries[0].textureIdentity == "textures/grass.png", "later slots shift down");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddCover(terrain, "textures/a.png"), "paint entry 0");
        Expect(AddCover(terrain, "textures/b.png"), "paint entry 1");
        Expect(
            world::ApplyTerrainGroundCoverStamp(terrain, CellStamp(terrain, 2, 3, 0, 2.0f)),
            "paint entry 0");
        Expect(OccupiedCells(terrain, 0) > 0, "paint writes occupancy");
        Expect(OccupiedCells(terrain, 1) == 0, "paint does not occupy the other entry");
        Expect(
            world::ApplyTerrainGroundCoverStamp(terrain, CellStamp(terrain, 2, 3, 1, 2.0f)),
            "paint entry 1 over the same region");
        Expect(OccupiedCells(terrain, 0) > 0 && OccupiedCells(terrain, 1) > 0, "entries coexist spatially");
        world::TerrainGroundCoverStampRequest erase = CellStamp(terrain, 2, 3, 0, 2.0f);
        erase.operation = world::TerrainGroundCoverBrushOperation::Erase;
        Expect(world::ApplyTerrainGroundCoverStamp(terrain, erase), "erase selected entry");
        Expect(OccupiedCells(terrain, 0) == 0, "erase removes only the selected entry");
        Expect(OccupiedCells(terrain, 1) > 0, "coexisting entry survives erase");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddCover(terrain), "next-paint palette");
        terrain.groundCoverEntries[0].density = 8.0f;
        terrain.groundCoverEntries[0].minWidth = 0.2f;
        terrain.groundCoverEntries[0].maxWidth = 0.3f;
        terrain.groundCoverEntries[0].minHeight = 0.2f;
        terrain.groundCoverEntries[0].maxHeight = 0.3f;
        Expect(
            world::ApplyTerrainGroundCoverStamp(terrain, CellStamp(terrain, 1, 1, 0, 1.5f)),
            "paint captures current Next Paint");
        const int cell = world::TerrainGroundCoverCellIndex(terrain, 1, 1);
        const int slot = world::TerrainGroundCoverDensitySlot(cell, 0);
        const unsigned char paintedQuantum = terrain.groundCoverDensityQuanta[static_cast<std::size_t>(slot)];
        const std::uint16_t paintedStyle = terrain.groundCoverPaintParams[static_cast<std::size_t>(slot)];
        terrain.groundCoverEntries[0].density = 40.0f;
        terrain.groundCoverEntries[0].minWidth = 0.8f;
        terrain.groundCoverEntries[0].maxWidth = 1.2f;
        terrain.groundCoverEntries[0].minHeight = 0.9f;
        terrain.groundCoverEntries[0].maxHeight = 1.4f;
        Expect(
            terrain.groundCoverDensityQuanta[static_cast<std::size_t>(slot)] == paintedQuantum
                && terrain.groundCoverPaintParams[static_cast<std::size_t>(slot)] == paintedStyle,
            "changing Next Paint without painting leaves authored cells");
        Expect(
            world::ApplyTerrainGroundCoverStamp(terrain, CellStamp(terrain, 6, 6, 0, 1.5f)),
            "later paint captures the new Next Paint");
        const int later = world::TerrainGroundCoverDensitySlot(
            world::TerrainGroundCoverCellIndex(terrain, 6, 6), 0);
        Expect(
            terrain.groundCoverPaintParams[static_cast<std::size_t>(slot)] == paintedStyle
                && terrain.groundCoverPaintParams[static_cast<std::size_t>(later)] != paintedStyle,
            "repaint authors the current values only in the affected region");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddCover(terrain), "deterministic palette");
        OccupyAll(terrain, 0, 16.0f);
        std::vector<world::TerrainGroundCoverInstance> first;
        std::vector<world::TerrainGroundCoverInstance> second;
        world::BuildTerrainGroundCoverInstances(terrain, first);
        world::BuildTerrainGroundCoverInstances(terrain, second);
        Expect(!first.empty(), "dense cells generate instances");
        Expect(SameInstances(first, second), "identical authored data is deterministic");
        terrain.groundCoverSeed = 99u;
        std::vector<world::TerrainGroundCoverInstance> otherSeed;
        world::BuildTerrainGroundCoverInstances(terrain, otherSeed);
        Expect(otherSeed.size() == first.size() || !otherSeed.empty(), "a different seed still generates");
        bool different = otherSeed.size() != first.size();
        const std::size_t compare = first.size() < otherSeed.size() ? first.size() : otherSeed.size();
        for (std::size_t index = 0; !different && index < compare; ++index)
        {
            different = first[index].position.x != otherSeed[index].position.x
                || first[index].position.z != otherSeed[index].position.z
                || first[index].width != otherSeed[index].width;
        }
        Expect(different, "different seeds produce different deterministic distributions");

        bool allSameX = !first.empty();
        bool allSameZ = !first.empty();
        for (std::size_t index = 1; index < first.size(); ++index)
        {
            allSameX = allSameX && first[index].position.x == first[0].position.x;
            allSameZ = allSameZ && first[index].position.z == first[0].position.z;
        }
        Expect(!allSameX && !allSameZ, "placement is not a single row or column");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddCover(terrain), "height-follow palette");
        Expect(
            world::ApplyTerrainGroundCoverStamp(terrain, CellStamp(terrain, 3, 2, 0, 3.0f)),
            "paint for height following");
        std::vector<world::TerrainGroundCoverInstance> before;
        world::BuildTerrainGroundCoverInstances(terrain, before);
        Expect(!before.empty(), "instances exist before sculpt");
        const std::vector<unsigned char> cells = terrain.groundCoverCells;
        const std::vector<unsigned char> quanta = terrain.groundCoverDensityQuanta;
        const std::vector<std::uint16_t> paint = terrain.groundCoverPaintParams;
        world::TerrainSculptStampRequest raise{};
        raise.operation = world::TerrainSculptOperation::Raise;
        raise.centerX = terrain.origin.x + terrain.sizeX * 0.5f;
        raise.centerZ = terrain.origin.z + terrain.sizeZ * 0.5f;
        raise.radius = 8.0f;
        raise.strength = 1.0f;
        Expect(world::ApplyTerrainSculptStamp(terrain, raise), "sculpt raises the surface");
        std::vector<world::TerrainGroundCoverInstance> after;
        world::BuildTerrainGroundCoverInstances(terrain, after);
        Expect(after.size() == before.size(), "sculpt does not change instance count");
        Expect(
            terrain.groundCoverCells == cells && terrain.groundCoverDensityQuanta == quanta
                && terrain.groundCoverPaintParams == paint,
            "sculpt does not rewrite authored ground-cover maps");
        bool yMoved = false;
        bool yawFollows = true;
        for (std::size_t index = 0; index < before.size(); ++index)
        {
            yMoved = yMoved || after[index].position.y != before[index].position.y;
            yawFollows = yawFollows && after[index].position.x == before[index].position.x
                && after[index].position.z == before[index].position.z;
        }
        Expect(yMoved, "derived Y follows the current Terrain height");
        Expect(yawFollows, "authored XZ coverage is unchanged by sculpt");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        Expect(AddCover(terrain, "textures/a.png"), "group identity A");
        Expect(AddCover(terrain, "textures/b.png"), "group identity B");
        OccupyAll(terrain, 0, world::kMaxTerrainGroundCoverDensity);
        OccupyAll(terrain, 1, world::kMaxTerrainGroundCoverDensity);
        std::vector<world::TerrainGroundCoverInstance> instances;
        world::BuildTerrainGroundCoverInstances(terrain, instances);
        world::TerrainGroundCoverRenderPlan plan{};
        world::BuildTerrainGroundCoverRenderPlan(terrain, instances, plan);
        Expect(instances.size() > 64, "high-density ground cover generates many instances");
        Expect(plan.groups.size() == 2, "two texture identities are two render groups");
        const std::size_t submissions = world::TerrainGroundCoverInstancedSubmissionCount(plan);
        Expect(submissions == 2, "each identity is one instanced submission");
        Expect(submissions < instances.size(), "submissions are materially fewer than instances");
        Expect(
            instances.size() <= static_cast<std::size_t>(world::kMaxTerrainGroundCoverInstances),
            "generation respects the instance cap");
    }

    {
        world::LevelDefinition level{};
        level.hasTerrain = true;
        level.terrain = MakeTerrain();
        Expect(AddCover(level.terrain), "editor paint entry");
        editor::TerrainGroundCoverState state{};
        state.mode = true;
        state.operation = world::TerrainGroundCoverBrushOperation::Paint;
        const editor::EditorSelection selection{editor::EditorObjectKind::Terrain, 0};
        editor::Ray3 miss{};
        miss.origin = {0.0f, 30.0f, 0.0f};
        miss.direction = {0.0f, 1.0f, 0.0f};
        const editor::TerrainGroundCoverFrameResult missed = editor::TickTerrainGroundCover(
            state, level, selection, miss, true, true, false, false, false, false);
        Expect(!missed.mutatedWorkingCopy, "a ray that misses Terrain does not paint");
        editor::Ray3 hit{};
        hit.origin = {
            level.terrain.origin.x + level.terrain.sizeX * 0.5f,
            30.0f,
            level.terrain.origin.z + level.terrain.sizeZ * 0.5f};
        hit.direction = {0.0f, -1.0f, 0.0f};
        const editor::TerrainGroundCoverFrameResult painted = editor::TickTerrainGroundCover(
            state, level, selection, hit, true, true, false, false, false, false);
        Expect(painted.mutatedWorkingCopy, "a Terrain hit paints the working copy");
        Expect(OccupiedCount(level.terrain) > 0, "editor paint writes working-copy cells");
        state.operation = world::TerrainGroundCoverBrushOperation::Erase;
        state.radius = 64.0f;
        const editor::TerrainGroundCoverFrameResult erased = editor::TickTerrainGroundCover(
            state, level, selection, hit, true, true, false, false, false, false);
        Expect(erased.mutatedWorkingCopy, "editor erase mutates the working copy");
        Expect(OccupiedCount(level.terrain) == 0, "editor erase clears the hit region");
    }

    {
        world::LevelDefinition level{};
        level.hasTerrain = true;
        level.terrain = MakeTerrain();
        Expect(AddCover(level.terrain), "picker capture entry");
        editor::TerrainGroundCoverState state{};
        state.mode = true;
        const editor::EditorSelection selection{editor::EditorObjectKind::Terrain, 0};
        editor::Ray3 hit{};
        hit.origin = {
            level.terrain.origin.x + level.terrain.sizeX * 0.5f,
            30.0f,
            level.terrain.origin.z + level.terrain.sizeZ * 0.5f};
        hit.direction = {0.0f, -1.0f, 0.0f};
        editor::NoteTerrainGroundCoverPickerOpen(state, true);
        Expect(editor::TerrainGroundCoverUiBlocksPointer(state), "open picker blocks the pointer");
        const editor::TerrainGroundCoverFrameResult blocked = editor::TickTerrainGroundCover(
            state, level, selection, hit, true, true, false, false, false, false);
        Expect(!blocked.mutatedWorkingCopy, "picker interaction does not paint behind the UI");
        Expect(state.pickerPointerLock, "picker latches pointer lock");
        editor::NoteTerrainGroundCoverPickerOpen(state, false);
        editor::TickTerrainGroundCover(
            state, level, selection, hit, false, false, true, false, false, false);
        Expect(!state.pickerPointerLock, "release clears picker pointer lock");
        const editor::TerrainGroundCoverFrameResult captured = editor::TickTerrainGroundCover(
            state, level, selection, hit, true, true, false, true, false, false);
        Expect(!captured.mutatedWorkingCopy, "ImGui mouse capture does not paint");
        Expect(
            !editor::TerrainGroundCoverBrushPreviewShouldDraw(true, true, state)
                || !editor::TerrainGroundCoverUiBlocksPointer(state),
            "brush preview hides while picker UI is blocking");
        editor::NoteTerrainGroundCoverPickerOpen(state, true);
        Expect(
            !editor::TerrainGroundCoverBrushPreviewShouldDraw(true, true, state),
            "brush preview hides while the picker is open");
    }

    {
        world::TerrainSpec terrain = MakeTerrain();
        editor::TerrainGroundCoverState state{};
        Expect(
            editor::TryAddTerrainGroundCoverPaletteEntry(terrain, state, "textures/grass.png"),
            "picker add writes a valid texture");
        Expect(state.selectedEntry == 0, "adding selects the new entry");
        Expect(
            !editor::TryAddTerrainGroundCoverPaletteEntry(terrain, state, "models/test_static.glb"),
            "invalid asset is rejected");
        Expect(terrain.groundCoverEntries.size() == 1, "invalid add leaves the palette");
        std::vector<editor::TerrainGroundCoverPickerItem> catalog{
            {"textures/grass.png", "grass.png"},
            {"textures/clover.png", "clover.png"}};
        Expect(
            editor::FilterTerrainGroundCoverPickerItems(catalog, "zzz").empty(),
            "empty search reports no matches");
        Expect(
            editor::FilterTerrainGroundCoverPickerItems(catalog, "clo").size() == 1,
            "search filters catalog identities");
        Expect(
            std::string(editor::TerrainGroundCoverMissingTextureStatusText()).find("missing")
                != std::string::npos,
            "missing referenced asset has a status");
        Expect(editor::TerrainGroundCoverPaletteIsEmpty(world::MakeDefaultTerrain()), "empty palette status");
        Expect(
            std::string(editor::TerrainGroundCoverPickerStatusText({}, {}, false)).find("cutout")
                != std::string::npos,
            "empty picker reports no compatible cutout textures");
        Expect(
            editor::TerrainPickerCardShouldActivate(true, 20.0f, 20.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "thumbnail click activates the ground-cover card");
        Expect(
            editor::TerrainPickerCardShouldActivate(true, 20.0f, 72.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "name click activates the same ground-cover card");
        Expect(
            !editor::TerrainPickerCardShouldActivate(true, 80.0f, 20.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "pointer outside the card does not activate");
        Expect(
            !editor::TerrainPickerCardShouldActivate(false, 20.0f, 20.0f, 0.0f, 0.0f, 64.0f, 16.0f),
            "a single card activation requires a press");
    }

    {
        constexpr std::uint8_t kOpaqueRgbPng[] = {
            137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 1, 0, 0, 0, 1,
            8, 2, 0, 0, 0, 144, 119, 83, 222, 0, 0, 0, 12, 73, 68, 65, 84, 120, 156, 99, 56, 161,
            33, 2, 0, 2, 192, 1, 5, 4, 252, 64, 27, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};
        constexpr std::uint8_t kOpaqueRgbaPng[] = {
            137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 2, 0, 0, 0, 2,
            8, 6, 0, 0, 0, 114, 182, 13, 36, 0, 0, 0, 21, 73, 68, 65, 84, 120, 156, 99, 228, 18,
            145, 251, 207, 192, 192, 192, 192, 4, 34, 64, 24, 0, 17, 212, 1, 63, 43, 103, 238, 166,
            0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};
        constexpr std::uint8_t kCutoutRgbaPng[] = {
            137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 0, 2, 0, 0, 0, 2,
            8, 6, 0, 0, 0, 114, 182, 13, 36, 0, 0, 0, 24, 73, 68, 65, 84, 120, 156, 99, 208, 216,
            98, 244, 159, 75, 68, 142, 129, 17, 68, 48, 48, 48, 48, 0, 0, 35, 47, 2, 135, 240, 21,
            87, 138, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130};
        Expect(
            !assets::RuntimePngHasUsefulCutoutAlpha(kOpaqueRgbPng, sizeof(kOpaqueRgbPng)),
            "opaque RGB is not a ground-cover cutout");
        Expect(
            !assets::RuntimePngHasUsefulCutoutAlpha(kOpaqueRgbaPng, sizeof(kOpaqueRgbaPng)),
            "opaque RGBA is not a ground-cover cutout");
        Expect(
            assets::RuntimePngHasUsefulCutoutAlpha(kCutoutRgbaPng, sizeof(kCutoutRgbaPng)),
            "RGBA with discarded alpha is a ground-cover cutout");

        const std::filesystem::path checker = TestCheckerPngPath();
        const std::filesystem::path tuft = TestGroundCoverTuftPngPath();
        Expect(std::filesystem::is_regular_file(checker), "opaque checker fixture exists");
        Expect(std::filesystem::is_regular_file(tuft), "cutout tuft fixture exists");
        Expect(
            !assets::RuntimePngFileHasUsefulCutoutAlpha(checker),
            "test_checker.png stays an ordinary opaque texture");
        Expect(
            assets::RuntimePngFileHasUsefulCutoutAlpha(tuft),
            "test_ground_cover_tuft.png is a valid cutout");

        const std::filesystem::path tempRoot =
            (std::filesystem::temp_directory_path() / "platformer_m96_cover_cutout").lexically_normal();
        std::error_code error;
        std::filesystem::remove_all(tempRoot, error);
        const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
        CopyFile(checker, sourceRoot / "textures" / "test_checker.png");
        CopyFile(tuft, sourceRoot / "textures" / "test_ground_cover_tuft.png");
        WriteBytes(sourceRoot / "textures" / "dirt.png", kOpaqueRgbPng, sizeof(kOpaqueRgbPng));
        const std::vector<std::string> discovered = assets::CollectSourceRuntimePngIdentities(sourceRoot);
        Expect(discovered.size() == 3, "Development catalog still lists every runtime PNG");
        std::vector<editor::TerrainGroundCoverPickerItem> catalog;
        for (const std::string& identity : discovered)
        {
            catalog.push_back({identity, assets::RuntimePngDisplayName(identity)});
        }
        const std::vector<editor::TerrainGroundCoverPickerItem> compatible =
            editor::CollectCompatibleTerrainGroundCoverPickerItems(catalog, sourceRoot);
        Expect(compatible.size() == 1, "picker keeps only useful-alpha cutout textures");
        Expect(
            compatible[0].canonicalIdentity == "textures/test_ground_cover_tuft.png",
            "valid Ground Cover tuft is accepted");
        Expect(
            editor::FilterTerrainGroundCoverPickerItems(compatible, "tuft").size() == 1,
            "search still filters the compatible catalog");
        std::filesystem::remove_all(tempRoot, error);
    }

    {
        world::TerrainSpec vegetation = MakeTerrain();
        Expect(world::TryAddTerrainVegetationEntry(vegetation, "models/test_static.glb"), "M93 palette stays");
        vegetation.vegetationEntries[0].density = 4.0f;
        world::TerrainVegetationStampRequest vegPaint{};
        vegPaint.entryIndex = 0;
        vegPaint.radius = 3.0f;
        vegPaint.centerX = vegetation.origin.x + vegetation.sizeX * 0.5f;
        vegPaint.centerZ = vegetation.origin.z + vegetation.sizeZ * 0.5f;
        Expect(world::ApplyTerrainVegetationStamp(vegetation, vegPaint), "M93 paint still works");
        Expect(AddCover(vegetation, "textures/grass.png"), "ground cover is independent of vegetation");
        Expect(
            world::ApplyTerrainGroundCoverStamp(vegetation, CellStamp(vegetation, 1, 1, 0, 2.0f)),
            "ground-cover paint does not clear vegetation");
        Expect(!vegetation.vegetationCells.empty(), "M93 occupancy remains");
        std::vector<world::TerrainVegetationInstance> vegInstances;
        world::BuildTerrainVegetationInstances(vegetation, vegInstances);
        world::TerrainVegetationRenderPlan vegPlan{};
        world::BuildTerrainVegetationRenderPlan(vegetation, vegInstances, vegPlan);
        Expect(!vegInstances.empty(), "M94 vegetation still derives instances");
        Expect(
            world::TerrainVegetationInstancedSubmissionCount(vegPlan, 1) < vegInstances.size()
                || vegInstances.size() <= 1,
            "M94 GPU grouping remains available");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d TerrainGroundCover test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("TerrainGroundCover tests passed.\n");
    return 0;
}
