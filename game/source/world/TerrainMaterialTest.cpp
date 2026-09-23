#include "assets/RuntimePng.h"
#include "editor/TerrainPaint.h"
#include "world/Terrain.h"
#include "world/TerrainGeometry.h"
#include "world/TerrainMaterialShading.h"
#include "world/TerrainSculpt.h"

#include <cmath>
#include <cstdio>
#include <limits>
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

bool NearlyEqual(float a, float b, float epsilon = 1.0e-5f)
{
    return std::fabs(a - b) <= epsilon;
}

bool TexNear(world::TerrainTexCoord a, world::TerrainTexCoord b, float epsilon = 1.0e-5f)
{
    return NearlyEqual(a.u, b.u, epsilon) && NearlyEqual(a.v, b.v, epsilon);
}

world::TerrainGeometry MakeGeometry(const world::TerrainSpec& terrain)
{
    world::TerrainGeometry geometry{};
    Expect(world::GenerateTerrainGeometry(terrain, geometry), "generate geometry fixture");
    return geometry;
}
}

int main()
{
    Expect(
        std::string(assets::kRuntimeTexturesLogicalDirectory) == "textures",
        "M84 texture directory is textures/");
    Expect(
        assets::RuntimePngIdentityIsValid("textures/test_checker.png"),
        "valid M84-style texture identity");
    Expect(!assets::RuntimePngIdentityIsValid(""), "empty identity is not a texture reference");
    Expect(
        !assets::RuntimePngIdentityIsValid("C:/textures/test_checker.png"),
        "absolute path is rejected");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures\\test_checker.png"),
        "backslash identity is rejected");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures/../test_checker.png"),
        "parent-directory identity is rejected");
    Expect(
        !assets::RuntimePngIdentityIsValid("models/test_static.glb"),
        "model identity is not a Terrain texture");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures/test_checker.PNG"),
        "extension is case-sensitive .png");
    Expect(
        !assets::RuntimePngIdentityIsValid("textures/my texture.png"),
        "spaces are not a single Level token");
    Expect(
        world::TerrainTextureIdentityIsValid(""),
        "no assignment is a valid empty identity");
    Expect(
        world::TerrainTextureIdentityIsValid("textures/test_checker.png"),
        "assigned identity follows M84 convention");
    Expect(
        !world::TerrainTextureIdentityIsValid("textures/missing space.png"),
        "invalid assigned identity is rejected");

    Expect(world::TerrainTextureTilingIsValid(world::kDefaultTerrainTextureTiling), "default tiling");
    Expect(world::TerrainTextureTilingIsValid(world::kMinTerrainTextureTiling), "min tiling");
    Expect(world::TerrainTextureTilingIsValid(world::kMaxTerrainTextureTiling), "max tiling");
    Expect(!world::TerrainTextureTilingIsValid(0.0f), "zero tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(-1.0f), "negative tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(world::kMaxTerrainTextureTiling + 1.0f),
        "over-max tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(std::numeric_limits<float>::quiet_NaN()),
        "non-finite tiling rejected");
    Expect(!world::TerrainTextureTilingIsValid(std::numeric_limits<float>::infinity()),
        "infinite tiling rejected");

    world::TerrainSpec terrain = world::MakeDefaultTerrain();
    Expect(world::TerrainSpecIsValid(terrain), "default Terrain remains valid");
    Expect(terrain.textureIdentity.empty(), "default has no texture assignment");
    Expect(terrain.textureTiling == world::kDefaultTerrainTextureTiling, "default tiling 0.25");
    Expect(
        !world::TerrainMaterialRecordShouldWrite(terrain),
        "default material is omitted from Level text");

    const world::TerrainSpec beforeAssign = terrain;
    Expect(
        world::TryAssignTerrainTextureIdentity(terrain, "textures/test_checker.png"),
        "first valid assignment changes authored data");
    Expect(terrain.textureIdentity == "textures/test_checker.png", "assignment stored");
    Expect(world::TerrainHeightsEqual(beforeAssign, terrain), "assignment does not change heights");
    Expect(
        world::TerrainMeshDataEqual(beforeAssign, terrain),
        "assignment does not change mesh/UV data");
    Expect(!world::TerrainSpecEqual(beforeAssign, terrain), "assignment is a semantic change");
    Expect(
        !world::TryAssignTerrainTextureIdentity(terrain, "textures/test_checker.png"),
        "same-assignment is a no-op");
    Expect(
        !world::TryAssignTerrainTextureIdentity(terrain, "C:/abs/grass.png"),
        "invalid identity is not stored");
    Expect(terrain.textureIdentity == "textures/test_checker.png", "invalid assign leaves identity");

    const world::TerrainSpec textured = terrain;
    Expect(world::TryClearTerrainTextureIdentity(terrain), "clear assigned texture");
    Expect(!world::TryClearTerrainTextureIdentity(terrain), "clear empty is a no-op");
    Expect(terrain.textureIdentity.empty(), "cleared identity is empty");
    Expect(world::TerrainHeightsEqual(textured, terrain), "clear does not change heights");

    terrain = world::MakeDefaultTerrain();
    Expect(!world::TrySetTerrainTextureTiling(terrain, world::kDefaultTerrainTextureTiling),
        "same tiling is a no-op");
    Expect(world::TrySetTerrainTextureTiling(terrain, 0.5f), "tiling change is authored");
    Expect(terrain.textureTiling == 0.5f, "tiling stored");
    Expect(terrain.heights == world::MakeDefaultTerrain().heights, "tiling does not change heights");
    Expect(!world::TrySetTerrainTextureTiling(terrain, 0.0f), "invalid tiling is a no-op");
    Expect(terrain.textureTiling == 0.5f, "invalid tiling leaves value");
    Expect(world::TerrainMaterialRecordShouldWrite(terrain), "custom tiling is written");

    world::TerrainSpec fallbackInvalid = world::MakeDefaultTerrain();
    fallbackInvalid.textureIdentity = "not-a-texture";
    Expect(!world::TerrainSpecIsValid(fallbackInvalid), "invalid identity fails spec validation");
    fallbackInvalid.textureIdentity.clear();
    fallbackInvalid.textureTiling = 0.0f;
    Expect(!world::TerrainSpecIsValid(fallbackInvalid), "out-of-bounds tiling fails spec validation");

    const world::TerrainSpec flat = world::MakeDefaultTerrain();
    world::TerrainGeometry flatGeometry = MakeGeometry(flat);
    Expect(static_cast<int>(flatGeometry.texcoords.size()) == 45, "flat UV count");
    Expect(
        TexNear(flatGeometry.texcoords[0], {0.0f, 0.0f}),
        "sample(0,0) UV is origin-relative zero");
    Expect(
        TexNear(
            world::TerrainSampleTexCoord(flat, 8, 4),
            {flat.sizeX * flat.textureTiling, flat.sizeZ * flat.textureTiling}),
        "opposite corner UV uses world XZ * tiling");
    Expect(
        TexNear(
            flatGeometry.texcoords[static_cast<std::size_t>(world::TerrainHeightIndex(flat, 8, 4))],
            world::TerrainSampleTexCoord(flat, 8, 4)),
        "generated UVs match the XZ formula");

    world::TerrainSpec rectangular = world::MakeDefaultTerrain();
    rectangular.sizeX = 16.0f;
    rectangular.sizeZ = 8.0f;
    rectangular.textureTiling = 0.25f;
    world::TerrainGeometry rectangularGeometry = MakeGeometry(rectangular);
    const world::TerrainTexCoord rectU = world::TerrainSampleTexCoord(rectangular, 8, 0);
    const world::TerrainTexCoord rectV = world::TerrainSampleTexCoord(rectangular, 0, 4);
    Expect(NearlyEqual(rectU.u, 4.0f) && NearlyEqual(rectU.v, 0.0f), "rectangular U along +X");
    Expect(NearlyEqual(rectV.u, 0.0f) && NearlyEqual(rectV.v, 2.0f), "rectangular V along +Z");
    Expect(
        NearlyEqual(rectU.u / rectangular.sizeX, rectV.v / rectangular.sizeZ),
        "planar mapping keeps isotropic world scale on rectangular Terrain");

    world::TerrainSpec raised = flat;
    raised.heights[static_cast<std::size_t>(world::TerrainHeightIndex(flat, 4, 2))] = 1.5f;
    world::TerrainGeometry raisedGeometry = MakeGeometry(raised);
    Expect(static_cast<int>(raisedGeometry.texcoords.size()) == 45, "raised UV count");
    for (int iz = 0; iz < flat.resolutionZ; ++iz)
    {
        for (int ix = 0; ix < flat.resolutionX; ++ix)
        {
            const int index = world::TerrainHeightIndex(flat, ix, iz);
            Expect(
                TexNear(
                    flatGeometry.texcoords[static_cast<std::size_t>(index)],
                    raisedGeometry.texcoords[static_cast<std::size_t>(index)]),
                "UVs stay stable when only heights change");
        }
    }

    world::TerrainSpec material = world::MakeDefaultTerrain();
    const std::vector<float> heightsBefore = material.heights;
    Expect(world::TryAssignTerrainTextureIdentity(material, "textures/test_checker.png"),
        "material assign fixture");
    Expect(world::TrySetTerrainTextureTiling(material, 1.0f), "material tiling fixture");
    Expect(material.heights == heightsBefore, "material edits do not modify heights");

    const std::string identityBefore = material.textureIdentity;
    const float tilingBefore = material.textureTiling;
    const core::Vec3 center = world::TerrainSamplePosition(material, 4, 2);
    world::TerrainSculptStampRequest raise{};
    raise.operation = world::TerrainSculptOperation::Raise;
    raise.centerX = center.x;
    raise.centerZ = center.z;
    raise.radius = 2.0f;
    raise.strength = 0.5f;
    Expect(world::ApplyTerrainSculptStamp(material, raise), "sculpt textured Terrain");
    Expect(material.textureIdentity == identityBefore, "sculpt does not change texture identity");
    Expect(material.textureTiling == tilingBefore, "sculpt does not change tiling");
    Expect(material.heights != heightsBefore, "sculpt still changes heights");

    world::TerrainSpec active = world::MakeDefaultTerrain();
    world::TerrainSpec working = active;
    Expect(world::TryAssignTerrainTextureIdentity(working, "textures/test_checker.png"),
        "workingCopy material edit");
    Expect(world::TrySetTerrainTextureTiling(working, 0.5f), "workingCopy tiling edit");
    Expect(!world::TerrainSpecEqual(working, active), "workingCopy differs before Apply");
    Expect(active.textureIdentity.empty(), "active identity unchanged before Apply");
    Expect(
        active.textureTiling == world::kDefaultTerrainTextureTiling,
        "active tiling unchanged before Apply");
    const world::TerrainSpec promoted = working;
    Expect(promoted.textureIdentity == working.textureIdentity, "Apply promotes identity");
    Expect(promoted.textureTiling == working.textureTiling, "Apply promotes tiling");
    Expect(world::TerrainHeightsEqual(promoted, working), "Apply preserves heights");

    Expect(
        world::TerrainNormalMapIdentityIsCompatible("textures/rock_NormalGL.png"),
        "AmbientCG NormalGL suffix is a Normal map");
    Expect(
        world::TerrainNormalMapIdentityIsCompatible("textures/cliff_NormalDX.png"),
        "AmbientCG NormalDX suffix is a Normal map");
    Expect(
        world::TerrainNormalMapIdentityIsCompatible("textures/dirt_Normal.png"),
        "_Normal suffix is a Normal map");
    Expect(
        !world::TerrainNormalMapIdentityIsCompatible("textures/test_checker.png"),
        "ordinary albedo PNG is not a Normal map");
    Expect(
        world::TerrainRoughnessMapIdentityIsCompatible("textures/rock_Roughness.png"),
        "_Roughness suffix is a Roughness map");
    Expect(
        !world::TerrainRoughnessMapIdentityIsCompatible("textures/test_checker.png"),
        "ordinary albedo PNG is not a Roughness map");
    Expect(
        !world::TerrainNormalMapIdentityIsCompatible("textures/test_ground_cover_tuft.png"),
        "cutout PNG is not labeled a Normal map");

    world::TerrainSpec channels = world::MakeDefaultTerrain();
    const world::TerrainSpec channelsActive = channels;
    Expect(
        world::TryAssignTerrainLayerNormal(channels, 0, "textures/rock_NormalGL.png"),
        "assign layer 0 Normal");
    Expect(channels.normalIdentity == "textures/rock_NormalGL.png", "Normal identity stored");
    Expect(channels.textureIdentity.empty(), "Normal assign does not invent albedo");
    Expect(
        !world::TryAssignTerrainLayerNormal(channels, 0, "textures/rock_NormalGL.png"),
        "same Normal is a no-op");
    Expect(
        world::TryAssignTerrainLayerNormal(channels, 0, "textures/dirt_Normal.png"),
        "replace layer 0 Normal");
    Expect(channels.normalIdentity == "textures/dirt_Normal.png", "replaced Normal stored");
    Expect(
        !world::TryAssignTerrainLayerNormal(channels, 0, "C:/abs/n.png"),
        "invalid Normal is rejected");
    Expect(channels.normalIdentity == "textures/dirt_Normal.png", "invalid Normal leaves identity");
    Expect(world::TryClearTerrainLayerNormal(channels, 0), "clear layer 0 Normal");
    Expect(channels.normalIdentity.empty(), "cleared Normal is empty");
    Expect(!world::TryClearTerrainLayerNormal(channels, 0), "clear empty Normal is a no-op");

    Expect(
        world::TryAssignTerrainLayerRoughness(channels, 0, "textures/rock_Roughness.png"),
        "assign layer 0 Roughness");
    Expect(
        world::TryAssignTerrainLayerRoughness(channels, 0, "textures/dirt_rough.png"),
        "replace layer 0 Roughness");
    Expect(channels.roughnessIdentity == "textures/dirt_rough.png", "replaced Roughness stored");
    Expect(world::TryClearTerrainLayerRoughness(channels, 0), "clear layer 0 Roughness");
    Expect(channels.roughnessIdentity.empty(), "cleared Roughness is empty");
    Expect(world::TerrainSpecEqual(channels, channelsActive), "cleared channels match original");
    Expect(channelsActive.normalIdentity.empty(), "active Normal unchanged before Apply");
    Expect(channelsActive.roughnessIdentity.empty(), "active Roughness unchanged before Apply");

    Expect(
        world::TryAssignTerrainTextureIdentity(channels, "textures/grass.png"),
        "base albedo for extra layer");
    Expect(
        world::TryAddTerrainMaterialLayer(channels, "textures/dirt.png"),
        "extra layer for channels");
    Expect(
        world::TryAssignTerrainLayerNormal(channels, 1, "textures/dirt_NormalGL.png"),
        "assign extra-layer Normal");
    Expect(
        world::TryAssignTerrainLayerRoughness(channels, 1, "textures/dirt_Roughness.png"),
        "assign extra-layer Roughness");
    Expect(
        world::TerrainLayerNormalIdentity(channels, 1) == "textures/dirt_NormalGL.png",
        "extra Normal attached to layer 1");
    Expect(
        world::TryAddTerrainMaterialLayer(channels, "textures/rock.png"),
        "second extra layer");
    Expect(
        world::TryAssignTerrainLayerNormal(channels, 2, "textures/rock_NormalGL.png"),
        "layer 2 Normal");
    Expect(world::TryRemoveTerrainMaterialLayer(channels, 1), "remove layer 1 remaps extras");
    Expect(
        world::TerrainLayerNormalIdentity(channels, 1) == "textures/rock_NormalGL.png",
        "remaining extra keeps its own Normal after remap");
    Expect(
        world::TerrainLayerTextureIdentity(channels, 1) == "textures/rock.png",
        "remaining extra keeps its albedo after remap");

    Expect(
        world::TerrainReferencesTextureIdentity(channels, "textures/rock_NormalGL.png"),
        "delete guard sees Normal identity");
    Expect(
        world::TerrainReferencesTextureIdentity(channels, "textures/dirt_Roughness.png") == false,
        "removed layer Roughness is no longer referenced");
    channels.extraLayers[0].roughnessIdentity = "textures/rock_Roughness.png";
    Expect(
        world::TerrainReferencesTextureIdentity(channels, "textures/rock_Roughness.png"),
        "delete guard sees Roughness identity");

    std::vector<std::string> collected;
    world::CollectTerrainTextureIdentities(channels, collected);
    bool foundNormal = false;
    bool foundRough = false;
    for (const std::string& identity : collected)
    {
        foundNormal = foundNormal || identity == "textures/rock_NormalGL.png";
        foundRough = foundRough || identity == "textures/rock_Roughness.png";
    }
    Expect(foundNormal, "cook collection includes Normal-only identity");
    Expect(foundRough, "cook collection includes Roughness-only identity");

    world::TerrainSpec missing = world::MakeDefaultTerrain();
    missing.normalIdentity = "textures/missing_NormalGL.png";
    missing.roughnessIdentity = "textures/missing_Roughness.png";
    Expect(world::TerrainSpecIsValid(missing), "missing files stay valid authored identities");

    const core::Vec3 flatN = world::TerrainFlatTangentNormal();
    Expect(flatN.x == 0.0f && flatN.y == 0.0f && flatN.z == 1.0f, "flat tangent normal is +Z");
    const world::TerrainTangentBasis flatBasis = world::BuildTerrainTangentBasis({0.0f, 1.0f, 0.0f});
    Expect(NearlyEqual(flatBasis.tangent.x, 1.0f) && NearlyEqual(flatBasis.tangent.y, 0.0f)
            && NearlyEqual(flatBasis.tangent.z, 0.0f),
        "flat Terrain tangent follows +X / U");
    Expect(NearlyEqual(flatBasis.bitangent.x, 0.0f) && NearlyEqual(flatBasis.bitangent.y, 0.0f)
            && NearlyEqual(flatBasis.bitangent.z, 1.0f),
        "flat Terrain bitangent follows +Z / V");
    const core::Vec3 worldFromFlat =
        world::TerrainTangentToWorld(flatBasis, world::TerrainFlatTangentNormal());
    Expect(NearlyEqual(worldFromFlat.x, 0.0f) && NearlyEqual(worldFromFlat.y, 1.0f)
            && NearlyEqual(worldFromFlat.z, 0.0f),
        "neutral tangent normal does not perturb geometric +Y");
    const world::TerrainTangentBasis slopeBasis =
        world::BuildTerrainTangentBasis(world::TerrainNormalizeOr({-0.5f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}));
    Expect(world::TerrainNormalBlendIsStable(slopeBasis.normal), "sloped basis normal is unit");
    Expect(world::TerrainNormalBlendIsStable(slopeBasis.tangent), "sloped basis tangent is unit");
    Expect(world::TerrainNormalBlendIsStable(slopeBasis.bitangent), "sloped basis bitangent is unit");

    float weights[world::kMaxTerrainMaterialLayers]{};
    core::Vec3 normals[world::kMaxTerrainMaterialLayers]{};
    weights[0] = 0.7f;
    weights[1] = 0.3f;
    normals[0] = world::TerrainFlatTangentNormal();
    normals[1] = world::DecodeTerrainNormalMapRgb(1.0f, 0.5f, 0.5f);
    const core::Vec3 blended = world::BlendTerrainTangentNormals(weights, normals, 2);
    Expect(world::TerrainNormalBlendIsStable(blended), "blended normal is unit and finite");
    Expect(blended.x > 0.0f, "painted Normal pulls the blend away from flat");
    weights[0] = 1.0f;
    weights[1] = 0.0f;
    const core::Vec3 unpainted = world::BlendTerrainTangentNormals(weights, normals, 2);
    Expect(NearlyEqual(unpainted.x, 0.0f) && NearlyEqual(unpainted.y, 0.0f)
            && NearlyEqual(unpainted.z, 1.0f),
        "no-paint 100% layer0 keeps the flat tangent normal");
    const float defaultRough = world::kDefaultTerrainRoughness;
    Expect(defaultRough > 0.5f && defaultRough < 0.9f, "default roughness is matte, not mirror-like");
    Expect(
        world::TerrainSpecularIntensityFromRoughness(defaultRough)
            < world::TerrainSpecularIntensityFromRoughness(0.1f),
        "low roughness has a stronger specular response");
    Expect(
        world::TerrainSpecularIntensityFromRoughness(1.0f) == 0.0f,
        "max roughness has no Terrain specular");

    std::vector<std::string> catalog{
        "textures/test_checker.png",
        "textures/rock_NormalGL.png",
        "textures/rock_Roughness.png",
        "textures/test_ground_cover_tuft.png"};
    const std::vector<std::string> normalsFiltered = editor::FilterTerrainMaterialChannelIdentities(
        catalog, editor::TerrainMaterialChannelKind::Normal, "");
    Expect(normalsFiltered.size() == 1 && normalsFiltered[0] == "textures/rock_NormalGL.png",
        "Normal picker keeps only the naming-convention map");
    const std::vector<std::string> roughnessFiltered = editor::FilterTerrainMaterialChannelIdentities(
        catalog, editor::TerrainMaterialChannelKind::Roughness, "rock");
    Expect(
        roughnessFiltered.size() == 1 && roughnessFiltered[0] == "textures/rock_Roughness.png",
        "Roughness picker search stays on compatible maps");
    const std::vector<std::string> albedoFiltered = editor::FilterTerrainMaterialChannelIdentities(
        catalog, editor::TerrainMaterialChannelKind::Albedo, "");
    Expect(albedoFiltered.size() == 4, "Albedo picker still lists ordinary Content Browser textures");
    Expect(
        world::TerrainNormalMapIdentityIsDirectX("textures/cliff_NormalDX.png"),
        "DirectX suffix is recognized");
    Expect(
        !world::TerrainNormalMapIdentityIsDirectX("textures/rock_NormalGL.png"),
        "OpenGL suffix is not treated as DirectX");
    Expect(
        world::FlipDirectXNormalGreenQuantum(0) == 255
            && world::FlipDirectXNormalGreenQuantum(255) == 0
            && world::FlipDirectXNormalGreenQuantum(64) == 191,
        "DirectX green channel is inverted at load");

    const std::vector<std::string> emptyCatalog{};
    editor::TerrainPaintState picker{};
    Expect(
        editor::RequestTerrainMaterialChannelPicker(
            picker, 0, editor::TerrainMaterialChannelKind::Albedo),
        "Albedo Replace requests the Albedo picker");
    Expect(picker.pickerOpenRequested, "Albedo request latches an open flag");
    Expect(picker.pickerChannel == editor::TerrainMaterialChannelKind::Albedo, "Albedo channel retained");
    Expect(picker.pickerLayer == 0, "Albedo request keeps layer 0");
    Expect(
        editor::ConsumeTerrainMaterialChannelPickerOpenRequest(picker),
        "Albedo request is consumed for OpenPopup");
    Expect(!picker.pickerOpenRequested, "consume clears the Albedo open request");
    Expect(
        !editor::ConsumeTerrainMaterialChannelPickerOpenRequest(picker),
        "second consume does not reopen");

    Expect(
        editor::RequestTerrainMaterialChannelPicker(
            picker, 2, editor::TerrainMaterialChannelKind::Normal),
        "Normal Assign requests the Normal picker");
    Expect(picker.pickerChannel == editor::TerrainMaterialChannelKind::Normal, "Normal channel retained");
    Expect(picker.pickerLayer == 2, "requested layer index is retained");
    const editor::TerrainMaterialChannelPickerView emptyNormal =
        editor::BuildTerrainMaterialChannelPickerView(picker, emptyCatalog);
    Expect(emptyNormal.identities.empty(), "empty Normal catalog still builds a view");
    Expect(
        std::string(emptyNormal.statusText).find("No compatible Normal textures found")
            != std::string::npos,
        "empty Normal catalog has an explicit empty state");
    Expect(picker.pickerOpenRequested, "Normal picker opens even with zero compatible assets");
    Expect(editor::ConsumeTerrainMaterialChannelPickerOpenRequest(picker), "Normal request is consumed");

    Expect(
        editor::RequestTerrainMaterialChannelPicker(
            picker, 1, editor::TerrainMaterialChannelKind::Roughness),
        "Roughness Assign requests the Roughness picker");
    Expect(
        picker.pickerChannel == editor::TerrainMaterialChannelKind::Roughness,
        "Roughness channel retained");
    Expect(picker.pickerLayer == 1, "Roughness request keeps the requested layer");
    const editor::TerrainMaterialChannelPickerView emptyRough =
        editor::BuildTerrainMaterialChannelPickerView(picker, emptyCatalog);
    Expect(emptyRough.identities.empty(), "empty Roughness catalog still builds a view");
    Expect(
        std::string(emptyRough.statusText).find("No compatible Roughness textures found")
            != std::string::npos,
        "empty Roughness catalog has an explicit empty state");
    Expect(picker.pickerOpenRequested, "Roughness picker opens even with zero compatible assets");
    Expect(
        editor::ConsumeTerrainMaterialChannelPickerOpenRequest(picker),
        "Roughness request is consumed");

    picker.pickerChannel = editor::TerrainMaterialChannelKind::Albedo;
    picker.pickerFilter.clear();
    const editor::TerrainMaterialChannelPickerView albedoFromCatalog =
        editor::BuildTerrainMaterialChannelPickerView(picker, catalog);
    Expect(albedoFromCatalog.identities.size() == 4, "Albedo view lists Color/cutout/checker PNGs");
    picker.pickerChannel = editor::TerrainMaterialChannelKind::Normal;
    const editor::TerrainMaterialChannelPickerView normalFromCatalog =
        editor::BuildTerrainMaterialChannelPickerView(picker, catalog);
    Expect(
        normalFromCatalog.identities.size() == 1
            && normalFromCatalog.identities[0] == "textures/rock_NormalGL.png",
        "Normal view does not apply to ordinary albedo PNGs");

    world::TerrainSpec pickerWorking = world::MakeDefaultTerrain();
    const world::TerrainSpec pickerActive = pickerWorking;
    Expect(
        editor::TryAssignTerrainMaterialChannel(
            pickerWorking, 0, editor::TerrainMaterialChannelKind::Albedo, "textures/test_checker.png"),
        "selecting Albedo mutates workingCopy");
    Expect(
        pickerWorking.textureIdentity == "textures/test_checker.png"
            && pickerActive.textureIdentity.empty(),
        "Albedo selection does not mutate active");
    Expect(
        editor::TryAssignTerrainMaterialChannel(
            pickerWorking, 0, editor::TerrainMaterialChannelKind::Normal, "textures/rock_NormalGL.png"),
        "selecting Normal mutates workingCopy");
    Expect(pickerActive.normalIdentity.empty(), "Normal selection does not mutate active");
    Expect(
        editor::TryAssignTerrainMaterialChannel(
            pickerWorking,
            0,
            editor::TerrainMaterialChannelKind::Roughness,
            "textures/rock_Roughness.png"),
        "selecting Roughness mutates workingCopy");
    Expect(pickerActive.roughnessIdentity.empty(), "Roughness selection does not mutate active");
    const world::TerrainSpec pickerApplied = pickerWorking;
    Expect(
        pickerApplied.normalIdentity == "textures/rock_NormalGL.png"
            && pickerApplied.roughnessIdentity == "textures/rock_Roughness.png",
        "Apply promotes picker channel identities");
    Expect(
        editor::TryClearTerrainMaterialChannel(
            pickerWorking, 0, editor::TerrainMaterialChannelKind::Normal),
        "Clear works after optional Normal assignment");
    Expect(pickerWorking.normalIdentity.empty(), "Clear returns Normal to None");
    Expect(
        !editor::TryClearTerrainMaterialChannel(
            pickerWorking, 0, editor::TerrainMaterialChannelKind::Normal),
        "Clear stays a no-op while Normal is None");

    editor::TerrainPaintState paint{};
    Expect(!editor::TerrainPaintUiBlocksPointer(paint), "picker starts closed");
    editor::NoteTerrainPaintPickerOpen(paint, true);
    Expect(editor::TerrainPaintUiBlocksPointer(paint), "open picker captures pointer");
    editor::NoteTerrainPaintPickerOpen(paint, false);
    Expect(paint.pickerPointerLock, "release keeps pointer lock until mouse up");
    paint.pickerPointerLock = false;
    Expect(!editor::TerrainPaintUiBlocksPointer(paint), "closed picker does not capture pointer");
    Expect(
        editor::TerrainPickerCardShouldActivate(true, 4.0f, 4.0f, 0.0f, 0.0f, 64.0f, 16.0f),
        "picker card thumbnail is clickable");
    Expect(
        editor::TerrainPickerCardShouldActivate(true, 8.0f, 70.0f, 0.0f, 0.0f, 64.0f, 16.0f),
        "picker card name band is clickable");
    Expect(
        !editor::TerrainPickerCardShouldActivate(true, 80.0f, 4.0f, 0.0f, 0.0f, 64.0f, 16.0f),
        "outside the card does not activate");

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Terrain material test(s) failed.\n", gFailures);
        return 1;
    }
    std::printf("Terrain material tests passed.\n");
    return 0;
}
