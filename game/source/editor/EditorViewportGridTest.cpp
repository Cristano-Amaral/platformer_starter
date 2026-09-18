#include "editor/EditorViewportGrid.h"

#include "editor/EditorGizmo.h"
#include "editor/EditorPicking.h"
#include "editor/EditorSelection.h"
#include "render/Renderer.h"
#include "world/LevelDefinition.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s\n", name.c_str());
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) <= epsilon;
}

bool LineIsAxisAlignedToXZ(const editor::EditorViewportGridLine& line)
{
    return NearlyEqual(line.start.y, editor::kEditorViewportGridY)
        && NearlyEqual(line.end.y, editor::kEditorViewportGridY)
        && (NearlyEqual(line.start.x, line.end.x) || NearlyEqual(line.start.z, line.end.z));
}

int CountKind(
    const editor::EditorViewportGridLines& lines,
    editor::EditorViewportGridLineKind kind)
{
    int count = 0;
    const int limited = lines.count > editor::kEditorViewportGridMaxLines
        ? editor::kEditorViewportGridMaxLines
        : (lines.count < 0 ? 0 : lines.count);
    for (int index = 0; index < limited; ++index)
    {
        if (lines.items[index].kind == kind)
        {
            ++count;
        }
    }
    return count;
}

float ConstantCoord(const editor::EditorViewportGridLine& line, bool constantX)
{
    return constantX ? line.start.x : line.start.z;
}
}

int main()
{
    {
        const editor::EditorViewportGridPreferences defaults =
            editor::MakeDefaultEditorViewportGridPreferences();
        Expect(defaults.visible, "default Grid visibility is enabled");
        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        snap.enabled = false;
        Expect(defaults.visible, "Snap disabled does not hide default Grid");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(snap.translateIncrement)
                == editor::kDefaultTranslateSnapIncrement,
            "default minor spacing follows default Translate increment");
    }

    {
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(0.25f) == 0.25f,
            "Translate 0.25 is minor spacing 0.25");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(0.50f) == 0.50f,
            "Translate 0.50 is minor spacing 0.50");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(-4.0f)
                == editor::kDefaultTranslateSnapIncrement,
            "invalid Translate increment keeps Grid spacing safe");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(0.0f)
                == editor::kDefaultTranslateSnapIncrement,
            "zero Translate increment keeps Grid spacing safe");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(5000.0f)
                == editor::kMaxSnapIncrement,
            "huge Translate increment clamps Grid spacing");
    }

    {
        editor::EditorViewportGridGenerateRequest request{};
        request.translateIncrement = 0.25f;
        request.focusX = 0.0f;
        request.focusZ = 0.0f;
        request.halfExtent = 6.0f;
        const editor::EditorViewportGridLines atQuarter =
            editor::GenerateEditorViewportGridLines(request);
        request.translateIncrement = 0.50f;
        const editor::EditorViewportGridLines atHalf =
            editor::GenerateEditorViewportGridLines(request);
        Expect(atQuarter.count > 0 && atHalf.count > 0, "bounded generation produces lines");
        Expect(
            atQuarter.count <= editor::kEditorViewportGridMaxLines,
            "0.25 generation stays bounded");
        Expect(
            atHalf.count <= editor::kEditorViewportGridMaxLines,
            "0.50 generation stays bounded");
        Expect(atHalf.count < atQuarter.count, "larger Translate increment thins Grid lines");

        world::LevelDefinition authored{};
        authored.id = "level_01";
        authored.ground = {{0.0f, -0.25f, 0.0f}, {10.0f, 0.5f, 8.0f}};
        const world::LevelDefinition before = authored;
        request.translateIncrement = 0.50f;
        (void)editor::GenerateEditorViewportGridLines(request);
        Expect(
            authored.id == before.id && authored.ground.center.x == before.ground.center.x
                && authored.ground.size.x == before.ground.size.x,
            "changing Translate increment does not mutate authored Level data");
    }

    {
        editor::EditorSnapPreferences snap = editor::MakeDefaultEditorSnapPreferences();
        editor::EditorViewportGridPreferences grid =
            editor::MakeDefaultEditorViewportGridPreferences();
        snap.enabled = false;
        Expect(grid.visible, "Grid visible stays true while Snap is off");
        grid.visible = true;
        snap.enabled = true;
        Expect(grid.visible, "enabling Snap does not own Grid visibility");
        grid.visible = false;
        snap.enabled = true;
        Expect(!grid.visible, "Snap on does not force Grid visible");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(snap.translateIncrement) == 0.25f,
            "Snap enabled does not change Grid spacing source");
    }

    {
        Expect(
            editor::ClassifyEditorViewportGridIndex(1) == editor::EditorViewportGridLineKind::Minor,
            "index 1 is minor");
        Expect(
            editor::ClassifyEditorViewportGridIndex(4) == editor::EditorViewportGridLineKind::Major,
            "cadence 4 marks index 4 major");
        Expect(
            editor::ClassifyEditorViewportGridIndex(-4) == editor::EditorViewportGridLineKind::Major,
            "cadence 4 marks index -4 major");
        Expect(
            editor::ClassifyEditorViewportGridIndex(5) == editor::EditorViewportGridLineKind::Minor,
            "index 5 is minor at cadence 4");
        Expect(editor::kEditorViewportGridMajorCadence == 4, "selected major cadence is 4");
    }

    {
        editor::EditorViewportGridGenerateRequest request{};
        request.translateIncrement = 0.25f;
        request.focusX = 0.0f;
        request.focusZ = 0.0f;
        request.halfExtent = 6.0f;
        const editor::EditorViewportGridLines lines =
            editor::GenerateEditorViewportGridLines(request);
        Expect(lines.count > 4, "origin-centered window emits several lines");
        Expect(
            editor::EditorViewportGridIncludesOriginAxis(
                lines, editor::EditorViewportGridLineKind::AxisX),
            "world X axis is identified separately");
        Expect(
            editor::EditorViewportGridIncludesOriginAxis(
                lines, editor::EditorViewportGridLineKind::AxisZ),
            "world Z axis is identified separately");
        Expect(CountKind(lines, editor::EditorViewportGridLineKind::Major) > 0, "major lines exist");
        Expect(CountKind(lines, editor::EditorViewportGridLineKind::Minor) > 0, "minor lines exist");

        for (int index = 0; index < lines.count; ++index)
        {
            const editor::EditorViewportGridLine& line = lines.items[index];
            Expect(LineIsAxisAlignedToXZ(line), "grid line lies on Y=0 XZ plane");
            if (line.kind == editor::EditorViewportGridLineKind::AxisZ)
            {
                Expect(NearlyEqual(line.start.x, 0.0f) && NearlyEqual(line.end.x, 0.0f),
                    "Z axis is X=0");
            }
            else if (line.kind == editor::EditorViewportGridLineKind::AxisX)
            {
                Expect(NearlyEqual(line.start.z, 0.0f) && NearlyEqual(line.end.z, 0.0f),
                    "X axis is Z=0");
            }
            else
            {
                const bool constantX = NearlyEqual(line.start.x, line.end.x);
                const float coord = ConstantCoord(line, constantX);
                Expect(
                    editor::EditorViewportGridIsWorldMultiple(coord, 0.25f),
                    "ordinary line sits on a Translate-increment multiple");
                if (line.kind == editor::EditorViewportGridLineKind::Major)
                {
                    Expect(
                        editor::EditorViewportGridIsWorldMultiple(
                            coord, 0.25f * static_cast<float>(editor::kEditorViewportGridMajorCadence)),
                        "major line sits on cadence multiple");
                }
            }
        }
    }

    {
        editor::EditorViewportGridGenerateRequest dense{};
        dense.translateIncrement = editor::kMinSnapIncrement;
        dense.focusX = 12.0f;
        dense.focusZ = -8.0f;
        dense.halfExtent = editor::kEditorViewportGridMaxHalfExtent;
        const editor::EditorViewportGridLines lines =
            editor::GenerateEditorViewportGridLines(dense);
        Expect(lines.count > 0, "tiny increment still emits a finite grid");
        Expect(
            lines.count <= editor::kEditorViewportGridMaxLines,
            "tiny increment does not produce an unbounded line count");
        Expect(
            editor::EditorViewportGridVisualStride(
                dense.translateIncrement, dense.halfExtent)
                > 1,
            "dense views increase visual stride without changing authored increment");
        Expect(
            editor::EffectiveEditorViewportGridMinorSpacing(dense.translateIncrement)
                == editor::kMinSnapIncrement,
            "visual stride does not rewrite Translate increment");
        for (int index = 0; index < lines.count; ++index)
        {
            const editor::EditorViewportGridLine& line = lines.items[index];
            const bool constantX = NearlyEqual(line.start.x, line.end.x);
            const float coord = ConstantCoord(line, constantX);
            Expect(
                editor::EditorViewportGridIsWorldMultiple(coord, editor::kMinSnapIncrement),
                "skipped-density lines remain on world multiples");
        }
    }

    {
        const editor::EditorViewportGridGenerateRequest fromCamera =
            editor::MakeEditorViewportGridGenerateRequest(
                0.25f,
                {0.0f, 8.0f, 12.0f},
                {0.0f, 0.0f, 0.0f});
        Expect(NearlyEqual(fromCamera.focusX, 0.0f, 0.05f), "look-at Y=0 focus X stays world origin");
        Expect(NearlyEqual(fromCamera.focusZ, 0.0f, 0.05f), "look-at Y=0 focus Z stays world origin");
        Expect(
            fromCamera.halfExtent >= editor::kEditorViewportGridMinHalfExtent
                && fromCamera.halfExtent <= editor::kEditorViewportGridMaxHalfExtent,
            "view extent is finite");
        const editor::EditorViewportGridLines shifted =
            editor::GenerateEditorViewportGridLines(
                editor::MakeEditorViewportGridGenerateRequest(
                    0.25f,
                    {20.0f, 6.0f, 20.0f},
                    {20.0f, 0.0f, 20.0f}));
        bool anyNearCamera = false;
        for (int index = 0; index < shifted.count; ++index)
        {
            const editor::EditorViewportGridLine& line = shifted.items[index];
            const bool constantX = NearlyEqual(line.start.x, line.end.x);
            const float coord = ConstantCoord(line, constantX);
            if (std::fabs(coord - 20.0f) <= 6.0f)
            {
                anyNearCamera = true;
            }
            Expect(
                editor::EditorViewportGridIsWorldMultiple(coord, 0.25f),
                "camera-window lines stay world aligned");
        }
        Expect(anyNearCamera, "bounded window follows camera without leaving world alignment");
    }

    {
        const char* oldLayout =
            "[Window][Hierarchy]\n"
            "Pos=8,8\n"
            "Size=340,400\n"
            "[Platformer3D.Snap][Settings]\n"
            "Enabled=0\n"
            "Translate=0.5\n";
        const editor::EditorViewportGridPreferences fromOld =
            editor::ParseEditorViewportGridPreferencesFromLayoutText(oldLayout);
        Expect(fromOld.visible, "older layout without M77 fields loads Grid visible");

        editor::EditorViewportGridPreferences hidden =
            editor::MakeDefaultEditorViewportGridPreferences();
        hidden.visible = false;
        const std::string merged =
            editor::MergeEditorViewportGridPreferencesIntoLayoutText(oldLayout, hidden);
        Expect(merged.find("[Window][Hierarchy]") != std::string::npos,
            "grid merge keeps existing ImGui windows");
        Expect(merged.find("[Platformer3D.Snap][Settings]") != std::string::npos,
            "grid merge keeps M76 Snap section");
        const editor::EditorViewportGridPreferences roundTrip =
            editor::ParseEditorViewportGridPreferencesFromLayoutText(merged);
        Expect(!roundTrip.visible, "layout round-trip preserves Grid hidden");
        const editor::EditorSnapPreferences snapAfter =
            editor::ParseEditorSnapPreferencesFromLayoutText(merged);
        Expect(!snapAfter.enabled && snapAfter.translateIncrement == 0.5f,
            "grid merge does not rewrite Snap preferences");

        const char* invalidLayout =
            "[Platformer3D.ViewportGrid][Settings]\n"
            "Visible=maybe\n";
        const editor::EditorViewportGridPreferences sanitized =
            editor::ParseEditorViewportGridPreferencesFromLayoutText(invalidLayout);
        Expect(sanitized.visible, "invalid Visible falls back to default true");

        const std::filesystem::path temp =
            std::filesystem::temp_directory_path() / "platformer3d_m77_grid";
        std::filesystem::remove_all(temp);
        std::filesystem::create_directories(temp);
        const std::filesystem::path layoutPath = temp / "editor_layout.ini";
        {
            std::ofstream out(layoutPath, std::ios::binary);
            out << oldLayout;
        }
        Expect(
            editor::LoadEditorViewportGridPreferencesFromLayoutPath(layoutPath).visible,
            "missing M77 fields load Grid visible from disk");
        Expect(
            editor::SaveEditorViewportGridPreferencesToLayoutPath(layoutPath, hidden),
            "layout save writes M77 section");
        Expect(
            !editor::LoadEditorViewportGridPreferencesFromLayoutPath(layoutPath).visible,
            "disk round-trip preserves Grid hidden");
        std::string onDisk;
        {
            std::ifstream in(layoutPath);
            std::string line;
            while (std::getline(in, line))
            {
                onDisk += line;
                onDisk += '\n';
            }
        }
        Expect(onDisk.find("[Window][Hierarchy]") != std::string::npos,
            "disk merge keeps Hierarchy window");
        Expect(onDisk.find("Translate=0.5") != std::string::npos,
            "disk merge keeps Translate increment");
        std::filesystem::remove_all(temp);

        Expect(
            editor::LoadEditorViewportGridPreferencesFromLayoutPath({}).visible,
            "empty path uses Grid visible default");
    }

    {
        world::LevelDefinition working{};
        working.id = "level_01";
        working.ground = {{0.0f, -0.25f, 0.0f}, {10.0f, 0.5f, 8.0f}};
        const world::LevelDefinition baseline = working;
        editor::EditorViewportGridPreferences grid{};
        grid.visible = false;
        Expect(
            working.id == baseline.id && working.ground.center.y == baseline.ground.center.y,
            "Grid preference is not LevelDefinition data");
        Expect(
            world::AuthoredLevelDataEqual(working, baseline),
            "toggling Grid does not dirty authored equality");
    }

    {
        world::LevelDefinition level{};
        level.id = "level_01";
        level.initialSpawnVisualCenter = {8.0f, 0.8f, 0.0f};
        level.ground = {{0.0f, -0.25f, 0.0f}, {10.0f, 0.5f, 8.0f}};
        level.elevatedPlatforms.push_back({{2.0f, 1.0f, 0.0f}, {2.0f, 0.5f, 2.0f}});
        const editor::EditorPickingWorldState worldState =
            editor::AuthoredPickingWorldState(level);
        const editor::EditorPickingSet set = editor::BuildPickingSet(level, worldState);
        const int proxyCount = static_cast<int>(set.proxies.size());
        Expect(proxyCount > 0, "authored picking set is non-empty");
        for (const editor::PickingProxy& proxy : set.proxies)
        {
            Expect(proxy.selection.kind != editor::EditorObjectKind::None, "proxy has a kind");
        }

        editor::EditorViewportGridGenerateRequest request{};
        request.translateIncrement = 0.25f;
        request.focusX = 0.0f;
        request.focusZ = 0.0f;
        request.halfExtent = 8.0f;
        const editor::EditorViewportGridLines lines =
            editor::GenerateEditorViewportGridLines(request);
        Expect(lines.count > 0, "grid lines exist beside picking");
        const editor::EditorPickingSet after = editor::BuildPickingSet(level, worldState);
        Expect(
            static_cast<int>(after.proxies.size()) == proxyCount,
            "generating Grid does not add picking proxies");

        const editor::Ray3 down{{0.0f, 5.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
        const editor::EditorSelection hit = editor::PickNearest(down, set);
        Expect(hit.kind == editor::EditorObjectKind::Ground, "Y=0 ray still picks Ground");
        Expect(
            editor::PickNearest(down, after).kind == editor::EditorObjectKind::Ground,
            "Grid generation does not change Ground pick");
    }

    {
        const core::Vec3 origin{0.0f, 1.0f, 0.0f};
        const float length = 2.0f;
        const float hitRadius = editor::GizmoHitRadius(length);
        const editor::Ray3 alongX{{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
        const editor::Ray3 alongGround{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
        Expect(
            editor::PickGizmoHandle(alongX, origin, length, hitRadius) == editor::EditorAxis::X,
            "gizmo X hit is unchanged");
        Expect(
            editor::PickGizmoHandle(alongGround, origin, length, hitRadius)
                == editor::EditorAxis::None,
            "Y=0 grid plane is not a gizmo handle");
        editor::EditorViewportGridGenerateRequest request{};
        request.translateIncrement = 0.25f;
        (void)editor::GenerateEditorViewportGridLines(request);
        Expect(
            editor::PickGizmoHandle(alongX, origin, length, hitRadius) == editor::EditorAxis::X,
            "Grid generation does not change gizmo hit testing");
    }

    {
        Expect(!editor::GameplayRendersWorldGrid(), "Gameplay never renders a world grid");
        Expect(
            !editor::EditorViewportGridShouldDraw(false, true),
            "leaving F2 does not draw the authoring grid even if Grid visible is true");
        Expect(
            editor::EditorViewportGridShouldDraw(true, true),
            "F2 with Grid visible draws the M77 authoring grid");
        Expect(
            !editor::EditorViewportGridShouldDraw(true, false),
            "F2 with Grid hidden draws no authoring grid");
        Expect(
            !editor::EditorViewportGridShouldDraw(false, false),
            "Gameplay with Grid hidden still draws no grid");

        render::DebugWorldOverlay gameplayOverlay{};
        Expect(
            gameplayOverlay.editorViewportGridLineCount == 0,
            "default DrawWorld overlay requests no editor grid");
        Expect(
            !render::DrawWorldDrawsLegacyRaylibGrid(),
            "DrawWorld no longer includes the legacy raylib DrawGrid");
        Expect(
            !render::DrawWorldDrawsEditorViewportGrid(gameplayOverlay),
            "Gameplay/Release DrawWorld does not draw the M77 grid");

        render::DebugWorldOverlay editorOverlay{};
        editor::EditorViewportGridGenerateRequest request{};
        request.translateIncrement = 0.25f;
        request.focusX = 0.0f;
        request.focusZ = 0.0f;
        request.halfExtent = 6.0f;
        const editor::EditorViewportGridLines lines =
            editor::GenerateEditorViewportGridLines(request);
        editorOverlay.editorViewportGridLineCount = lines.count;
        Expect(
            render::DrawWorldDrawsEditorViewportGrid(editorOverlay),
            "F2 overlay with generated lines draws the M77 grid");

        render::DebugWorldOverlay afterLeaveEditor{};
        Expect(
            !render::DrawWorldDrawsEditorViewportGrid(afterLeaveEditor),
            "a new Gameplay overlay after F2 does not keep M77 grid lines");
        editor::EditorViewportGridPreferences stillVisible =
            editor::MakeDefaultEditorViewportGridPreferences();
        Expect(stillVisible.visible, "Grid preference can remain visible after leaving F2");
        Expect(
            !editor::EditorViewportGridShouldDraw(false, stillVisible.visible),
            "Grid preference has no Gameplay rendering effect");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d editor viewport grid test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Editor viewport grid tests passed.\n");
    return 0;
}
