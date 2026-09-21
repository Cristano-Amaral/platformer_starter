#include "assets/RuntimePng.h"
#include "world/Terrain.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainPaint.h"
#include "world/TerrainSculpt.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

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

bool NearlyEqual(float a, float b, float epsilon = 1.0e-4f)
{
    return std::fabs(a - b) <= epsilon;
}

world::TerrainSpec MakeGrid(int resolutionX, int resolutionZ, float sizeX, float sizeZ)
{
    world::TerrainSpec terrain = world::MakeDefaultTerrain();
    terrain.resolutionX = resolutionX;
    terrain.resolutionZ = resolutionZ;
    terrain.sizeX = sizeX;
    terrain.sizeZ = sizeZ;
    world::ResizeTerrainHeights(terrain);
    return terrain;
}

bool SampleWeightsEqual(
    const world::TerrainSpec& terrain,
    int sample,
    float w0,
    float w1,
    float w2,
    float w3,
    float epsilon = 1.0e-4f)
{
    return NearlyEqual(world::TerrainSampleLayerWeight(terrain, sample, 0), w0, epsilon)
        && NearlyEqual(world::TerrainSampleLayerWeight(terrain, sample, 1), w1, epsilon)
        && NearlyEqual(world::TerrainSampleLayerWeight(terrain, sample, 2), w2, epsilon)
        && NearlyEqual(world::TerrainSampleLayerWeight(terrain, sample, 3), w3, epsilon);
}

bool WeightsFinite(const world::TerrainSpec& terrain)
{
    const int samples = world::TerrainSampleCount(terrain);
    for (int sample = 0; sample < samples; ++sample)
    {
        float sum = 0.0f;
        for (int layer = 0; layer < world::kMaxTerrainMaterialLayers; ++layer)
        {
            const float weight = world::TerrainSampleLayerWeight(terrain, sample, layer);
            if (!std::isfinite(weight) || weight < 0.0f || weight > 1.0f)
            {
                return false;
            }
            sum += weight;
        }
        if (!std::isfinite(sum) || sum < 0.999f || sum > 1.001f)
        {
            return false;
        }
    }
    return true;
}
}

int main()
{
    Expect(world::kMaxTerrainMaterialLayers == 4, "bounded layer count is 4");
    world::TerrainSpec terrain = MakeGrid(5, 5, 8.0f, 8.0f);
    Expect(world::TerrainMaterialLayerCount(terrain) == 1, "default has only layer 0");
    Expect(world::TerrainMaterialWeightsAreDefault(terrain), "unpainted weights are default");
    Expect(SampleWeightsEqual(terrain, 0, 1.0f, 0.0f, 0.0f, 0.0f), "layer 0 weight is 1");
    Expect(world::TryAddTerrainMaterialLayer(terrain, "textures/dirt.png"), "add second layer");
    Expect(world::TerrainMaterialLayerCount(terrain) == 2, "two assigned layers");
    Expect(world::TerrainMaterialWeightsAreDefault(terrain), "add layer does not change weights");
    Expect(SampleWeightsEqual(terrain, 12, 1.0f, 0.0f, 0.0f, 0.0f), "unpainted extra layer is 0");
    Expect(
        !world::TryAddTerrainMaterialLayer(terrain, "textures/dirt.png"),
        "duplicate layer is rejected");
    Expect(world::TerrainMaterialLayerCount(terrain) == 2, "duplicate add is a no-op");
    Expect(
        !world::TryAssignTerrainTextureIdentity(terrain, "textures/dirt.png"),
        "layer 0 cannot duplicate an extra layer");
    Expect(world::TryAddTerrainMaterialLayer(terrain, "textures/rock.png"), "add third layer");
    Expect(world::TryAddTerrainMaterialLayer(terrain, "textures/sand.png"), "add fourth layer");
    Expect(world::TerrainMaterialLayerCount(terrain) == 4, "four layers is the cap");
    Expect(
        !world::TryAddTerrainMaterialLayer(terrain, "textures/cobble.png"),
        "fifth layer is rejected");

    const core::Vec3 center = world::TerrainSamplePosition(terrain, 2, 2);
    world::TerrainPaintStampRequest paint{};
    paint.layer = 1;
    paint.centerX = center.x;
    paint.centerZ = center.z;
    paint.radius = 2.0f;
    paint.strength = 1.0f;
    Expect(world::ApplyTerrainPaintStamp(terrain, paint), "center stamp paints layer 1");
    Expect(!world::TerrainMaterialWeightsAreDefault(terrain), "paint mutates weights");
    Expect(
        world::TerrainSampleLayerWeight(terrain, world::TerrainHeightIndex(terrain, 2, 2), 1) > 0.9f,
        "full-strength center converges toward selected layer");
    Expect(
        world::TerrainSampleLayerWeight(terrain, world::TerrainHeightIndex(terrain, 2, 2), 0) < 0.1f,
        "other layers decrease at the center");
    Expect(WeightsFinite(terrain), "painted weights stay finite and normalized");

    const int edgeIndex = world::TerrainHeightIndex(terrain, 0, 0);
    const core::Vec3 edge = world::TerrainSamplePosition(terrain, 0, 0);
    const float edgeDistance = world::TerrainSculptDistanceXZ(edge.x, edge.z, center.x, center.z);
    if (edgeDistance >= 2.0f)
    {
        Expect(
            SampleWeightsEqual(terrain, edgeIndex, 1.0f, 0.0f, 0.0f, 0.0f),
            "outside radius is unchanged");
    }

    world::TerrainSpec strengthA = MakeGrid(5, 5, 8.0f, 8.0f);
    Expect(world::TryAddTerrainMaterialLayer(strengthA, "textures/dirt.png"), "strength fixture A");
    world::TerrainSpec strengthB = strengthA;
    world::TerrainPaintStampRequest weak = paint;
    weak.strength = 0.25f;
    world::TerrainPaintStampRequest strong = paint;
    strong.strength = 1.0f;
    Expect(world::ApplyTerrainPaintStamp(strengthA, weak), "weak stamp");
    Expect(world::ApplyTerrainPaintStamp(strengthB, strong), "strong stamp");
    const int mid = world::TerrainHeightIndex(strengthA, 2, 2);
    Expect(
        world::TerrainSampleLayerWeight(strengthB, mid, 1)
            > world::TerrainSampleLayerWeight(strengthA, mid, 1),
        "higher Strength paints faster");

    world::TerrainSpec falloffGrid = MakeGrid(5, 5, 8.0f, 8.0f);
    Expect(world::TryAddTerrainMaterialLayer(falloffGrid, "textures/dirt.png"), "falloff fixture");
    paint.strength = 1.0f;
    Expect(world::ApplyTerrainPaintStamp(falloffGrid, paint), "falloff stamp");
    const int neighbor = world::TerrainHeightIndex(falloffGrid, 3, 2);
    Expect(
        world::TerrainSampleLayerWeight(falloffGrid, mid, 1)
            > world::TerrainSampleLayerWeight(falloffGrid, neighbor, 1),
        "linear falloff is stronger at the center");

    world::TerrainSpec converge = MakeGrid(3, 3, 4.0f, 4.0f);
    Expect(world::TryAddTerrainMaterialLayer(converge, "textures/dirt.png"), "converge fixture");
    const core::Vec3 convergeCenter = world::TerrainSamplePosition(converge, 1, 1);
    world::TerrainPaintStampRequest repeat{};
    repeat.layer = 1;
    repeat.centerX = convergeCenter.x;
    repeat.centerZ = convergeCenter.z;
    repeat.radius = 4.0f;
    repeat.strength = 0.5f;
    for (int i = 0; i < 16; ++i)
    {
        world::ApplyTerrainPaintStamp(converge, repeat);
    }
    const int convergeIndex = world::TerrainHeightIndex(converge, 1, 1);
    Expect(
        NearlyEqual(world::TerrainSampleLayerWeight(converge, convergeIndex, 1), 1.0f, 1.0e-3f),
        "repeated stamps converge to the selected layer");
    Expect(WeightsFinite(converge), "converged weights stay valid");

    world::TerrainSpec strokeGrid = MakeGrid(5, 5, 8.0f, 8.0f);
    Expect(world::TryAddTerrainMaterialLayer(strokeGrid, "textures/dirt.png"), "stroke fixture");
    world::TerrainPaintStroke stroke{};
    world::TerrainPaintStampRequest strokeReq{};
    strokeReq.layer = 1;
    const core::Vec3 start = world::TerrainSamplePosition(strokeGrid, 0, 2);
    const core::Vec3 end = world::TerrainSamplePosition(strokeGrid, 4, 2);
    strokeReq.centerX = start.x;
    strokeReq.centerZ = start.z;
    strokeReq.radius = 2.0f;
    strokeReq.strength = 1.0f;
    Expect(
        world::BeginTerrainPaintStroke(stroke, strokeGrid, strokeReq),
        "stroke begin paints");
    const world::TerrainSpec afterBegin = strokeGrid;
    Expect(
        !world::ContinueTerrainPaintStroke(
            stroke, strokeGrid, strokeReq, start.x, start.z),
        "stationary held cursor does not accumulate");
    Expect(world::TerrainSpecEqual(afterBegin, strokeGrid), "held cursor is not frame-rate dependent");
    Expect(
        world::ContinueTerrainPaintStroke(stroke, strokeGrid, strokeReq, end.x, end.z),
        "drag produces additional stamps");
    Expect(!world::TerrainSpecEqual(afterBegin, strokeGrid), "spatial travel paints further samples");
    world::EndTerrainPaintStroke(stroke);

    world::TerrainSpec miss = MakeGrid(3, 3, 4.0f, 4.0f);
    Expect(world::TryAddTerrainMaterialLayer(miss, "textures/dirt.png"), "outside fixture");
    const world::TerrainSpec missBefore = miss;
    world::TerrainPaintStampRequest outside{};
    outside.layer = 1;
    outside.centerX = 100.0f;
    outside.centerZ = 100.0f;
    outside.radius = 1.0f;
    outside.strength = 1.0f;
    Expect(!world::ApplyTerrainPaintStamp(miss, outside), "painting outside Terrain is a no-op");
    Expect(world::TerrainSpecEqual(missBefore, miss), "outside stamp leaves weights unchanged");

    world::TerrainSpec nanCheck = MakeGrid(3, 3, 4.0f, 4.0f);
    Expect(world::TryAddTerrainMaterialLayer(nanCheck, "textures/dirt.png"), "nan fixture");
    world::TerrainPaintStampRequest nanReq{};
    nanReq.layer = 1;
    nanReq.centerX = world::TerrainSamplePosition(nanCheck, 1, 1).x;
    nanReq.centerZ = world::TerrainSamplePosition(nanCheck, 1, 1).z;
    nanReq.radius = 8.0f;
    nanReq.strength = 1.0f;
    world::ApplyTerrainPaintStamp(nanCheck, nanReq);
    Expect(WeightsFinite(nanCheck), "paint never produces NaN/Inf");

    world::TerrainSpec sculptCoexist = MakeGrid(5, 5, 8.0f, 8.0f);
    Expect(world::TryAddTerrainMaterialLayer(sculptCoexist, "textures/dirt.png"), "coexist fixture");
    world::ApplyTerrainPaintStamp(sculptCoexist, paint);
    std::vector<float> paintedWeights = sculptCoexist.materialWeights;
    const float paintedHeight = sculptCoexist.heights[static_cast<std::size_t>(mid)];
    world::TerrainSculptStampRequest raise{};
    raise.operation = world::TerrainSculptOperation::Raise;
    raise.centerX = center.x;
    raise.centerZ = center.z;
    raise.radius = 2.0f;
    raise.strength = 0.5f;
    Expect(world::ApplyTerrainSculptStamp(sculptCoexist, raise), "Raise after paint");
    Expect(sculptCoexist.materialWeights == paintedWeights, "Raise preserves weights");
    Expect(sculptCoexist.heights[static_cast<std::size_t>(mid)] != paintedHeight, "Raise changes height");

    world::TerrainSculptStampRequest lower = raise;
    lower.operation = world::TerrainSculptOperation::Lower;
    Expect(world::ApplyTerrainSculptStamp(sculptCoexist, lower), "Lower after paint");
    Expect(sculptCoexist.materialWeights == paintedWeights, "Lower preserves weights");

    world::TerrainSculptStampRequest smooth = raise;
    smooth.operation = world::TerrainSculptOperation::Smooth;
    world::ApplyTerrainSculptStamp(sculptCoexist, smooth);
    Expect(sculptCoexist.materialWeights == paintedWeights, "Smooth preserves weights");

    world::TerrainSculptStampRequest flatten = raise;
    flatten.operation = world::TerrainSculptOperation::Flatten;
    flatten.flattenWorldY = center.y;
    world::ApplyTerrainSculptStamp(sculptCoexist, flatten);
    Expect(sculptCoexist.materialWeights == paintedWeights, "Flatten preserves weights");

    const std::vector<float> heightsAfterSculpt = sculptCoexist.heights;
    world::ApplyTerrainPaintStamp(sculptCoexist, paint);
    Expect(sculptCoexist.heights == heightsAfterSculpt, "paint does not modify heights");

    world::TerrainSpec active = world::MakeDefaultTerrain();
    world::TerrainSpec working = active;
    Expect(world::TryAddTerrainMaterialLayer(working, "textures/dirt.png"), "workingCopy add layer");
    world::ApplyTerrainPaintStamp(working, paint);
    Expect(!world::TerrainSpecEqual(working, active), "workingCopy paint does not mutate active");
    const world::TerrainSpec promoted = working;
    Expect(world::TerrainSpecEqual(promoted, working), "Apply promotes layers and weights");

    world::TerrainSpec unused = MakeGrid(3, 3, 4.0f, 4.0f);
    Expect(world::TryAssignTerrainTextureIdentity(unused, "textures/grass.png"), "unused base");
    Expect(world::TryAddTerrainMaterialLayer(unused, "textures/dirt.png"), "unused extra");
    Expect(world::TryAddTerrainMaterialLayer(unused, "textures/rock.png"), "unused second extra");
    Expect(world::TryRemoveTerrainMaterialLayer(unused, 1), "remove unused non-last layer");
    Expect(world::TerrainMaterialLayerCount(unused) == 2, "unused remove compact");
    Expect(unused.extraLayers[0].textureIdentity == "textures/rock.png", "remaining extra remaps");
    Expect(world::TerrainMaterialWeightsAreDefault(unused), "unused remove leaves default weights");

    world::TerrainSpec used = MakeGrid(5, 5, 8.0f, 8.0f);
    Expect(world::TryAddTerrainMaterialLayer(used, "textures/dirt.png"), "used extra");
    Expect(world::TryAddTerrainMaterialLayer(used, "textures/rock.png"), "used second extra");
    paint.layer = 1;
    world::ApplyTerrainPaintStamp(used, paint);
    const float usedCenter = world::TerrainSampleLayerWeight(
        used, world::TerrainHeightIndex(used, 2, 2), 1);
    Expect(usedCenter > 0.0f, "used layer has painted weight");
    Expect(world::TryRemoveTerrainMaterialLayer(used, 1), "remove used layer");
    Expect(world::TerrainMaterialLayerCount(used) == 2, "used remove compact");
    Expect(used.extraLayers[0].textureIdentity == "textures/rock.png", "used remove remaps remaining");
    Expect(
        world::TerrainSampleLayerWeight(used, world::TerrainHeightIndex(used, 2, 2), 1) == 0.0f
            || world::TerrainSampleLayerWeight(used, world::TerrainHeightIndex(used, 2, 2), 1) < 1.0e-3f,
        "removed layer index is gone");
    Expect(
        world::TerrainSampleLayerWeight(used, world::TerrainHeightIndex(used, 2, 2), 0) > 0.5f,
        "removed contribution folds into layer 0");
    Expect(WeightsFinite(used), "remove keeps normalized weights");
    Expect(!world::TryRemoveTerrainMaterialLayer(used, 0), "layer 0 cannot be removed");

    Expect(world::TerrainPaintStampSpacing(2.0f) == 0.5f, "stamp spacing is 0.25 * radius");
    Expect(
        world::SanitizeTerrainPaintRadius(0.0f) == world::kMinTerrainPaintRadius, "radius min clamp");
    Expect(
        world::SanitizeTerrainPaintStrength(2.0f) == world::kMaxTerrainPaintStrength,
        "strength max clamp");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d TerrainPaint test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("TerrainPaint tests passed.\n");
    return 0;
}
