// Milestone 84: imported Base Color / Base Color Texture presentation.
// CPU-only. No window, raylib, or GPU.

#include "assets/ModelMaterialPresentation.h"
#include "assets/StaticGlb.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
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

bool NearlyEqual(float a, float b, float epsilon = 0.0005f)
{
    return std::fabs(a - b) <= epsilon;
}

bool ColorNear(const assets::ModelMaterialPresentation& presentation, float r, float g, float b, float a)
{
    return NearlyEqual(presentation.baseColorR, r) && NearlyEqual(presentation.baseColorG, g)
        && NearlyEqual(presentation.baseColorB, b) && NearlyEqual(presentation.baseColorA, a);
}

std::filesystem::path FixturePath(const char* macro)
{
    if (macro == nullptr || macro[0] == '\0')
    {
        return {};
    }
    return std::filesystem::path{macro}.lexically_normal();
}

void AppendU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value >> 16));
    out.push_back(static_cast<std::uint8_t>(value >> 24));
}

void Pad4(std::vector<std::uint8_t>& bytes, std::uint8_t pad)
{
    while ((bytes.size() % 4) != 0)
    {
        bytes.push_back(pad);
    }
}

std::vector<std::uint8_t> PackGlb(const std::string& json, const std::vector<std::uint8_t>& bin)
{
    std::vector<std::uint8_t> jsonBytes(json.begin(), json.end());
    Pad4(jsonBytes, static_cast<std::uint8_t>(' '));
    std::vector<std::uint8_t> binBytes = bin;
    Pad4(binBytes, 0);
    const std::uint32_t total =
        12 + 8 + static_cast<std::uint32_t>(jsonBytes.size()) + 8
        + static_cast<std::uint32_t>(binBytes.size());
    std::vector<std::uint8_t> out;
    out.reserve(total);
    AppendU32(out, 0x46546C67);
    AppendU32(out, 2);
    AppendU32(out, total);
    AppendU32(out, static_cast<std::uint32_t>(jsonBytes.size()));
    AppendU32(out, 0x4E4F534A);
    out.insert(out.end(), jsonBytes.begin(), jsonBytes.end());
    AppendU32(out, static_cast<std::uint32_t>(binBytes.size()));
    AppendU32(out, 0x004E4942);
    out.insert(out.end(), binBytes.begin(), binBytes.end());
    return out;
}

std::vector<std::uint8_t> UntexturedColorGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}],"
        "\"materials\":[{\"pbrMetallicRoughness\":{"
        "\"baseColorFactor\":[0.92,0.45,0.18,1.0]}}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> DefaultMaterialGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}],"
        "\"materials\":[{\"name\":\"Material\"}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> EmbeddedTextureGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}],"
        "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":4}],"
        "\"images\":[{\"bufferView\":0,\"mimeType\":\"image/png\"}],"
        "\"textures\":[{\"source\":0}],"
        "\"materials\":[{\"pbrMetallicRoughness\":{"
        "\"baseColorFactor\":[1.0,0.5,0.25,1.0],"
        "\"baseColorTexture\":{\"index\":0}}}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> MissingTextureIndexGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}],"
        "\"materials\":[{\"pbrMetallicRoughness\":{"
        "\"baseColorFactor\":[0.1,0.2,0.3,1.0],"
        "\"baseColorTexture\":{\"index\":0}}}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> ZeroAlphaGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}],"
        "\"materials\":[{\"pbrMetallicRoughness\":{"
        "\"baseColorFactor\":[0.2,0.4,0.6,0.0]}}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> ExternalImageGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}],"
        "\"images\":[{\"uri\":\"sidecar.png\"}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> NoMaterialsGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4}]}",
        {0, 0, 0, 0});
}

void ExpectExtract(
    const std::vector<std::uint8_t>& bytes,
    std::vector<assets::ModelMaterialPresentation>& out,
    const char* name)
{
    std::string error;
    Expect(assets::TryExtractGlbMaterialPresentations(bytes, out, &error), name);
    if (!error.empty())
    {
        std::fprintf(stderr, "  extract error (%s): %s\n", name, error.c_str());
    }
}
}

int main()
{
    using assets::BaseColorTextureDependency;
    using assets::ModelMaterialPresentation;

    {
        ModelMaterialPresentation imported{};
        imported.baseColorR = 0.92f;
        imported.baseColorG = 0.45f;
        imported.baseColorB = 0.18f;
        imported.baseColorA = 1.0f;
        const ModelMaterialPresentation resolved = assets::ResolveMaterialFallback(imported);
        Expect(!resolved.usedFallback, "valid untextured color is preserved");
        Expect(ColorNear(resolved, 0.92f, 0.45f, 0.18f, 1.0f), "preserved RGB matches import");
        Expect(!resolved.hasBaseColorTexture, "untextured has no texture");
        Expect(
            resolved.textureDependency == BaseColorTextureDependency::None,
            "untextured dependency is none");
    }

    {
        ModelMaterialPresentation imported{};
        imported.baseColorA = 0.0f;
        const ModelMaterialPresentation resolved = assets::ResolveMaterialFallback(imported);
        Expect(resolved.usedFallback, "zero alpha uses fallback");
        Expect(ColorNear(resolved, 1.0f, 1.0f, 1.0f, 1.0f), "zero alpha becomes opaque white");
    }

    {
        ModelMaterialPresentation imported{};
        imported.baseColorR = std::numeric_limits<float>::quiet_NaN();
        const ModelMaterialPresentation resolved = assets::ResolveMaterialFallback(imported);
        Expect(resolved.usedFallback, "non-finite color uses fallback");
        Expect(ColorNear(resolved, 1.0f, 1.0f, 1.0f, 1.0f), "non-finite color becomes white");
    }

    Expect(
        assets::ResolveBaseColorTextureDependency(false, false, false)
            == BaseColorTextureDependency::None,
        "no texture dependency");
    Expect(
        assets::ResolveBaseColorTextureDependency(true, true, false)
            == BaseColorTextureDependency::Embedded,
        "embedded texture dependency");
    Expect(
        assets::ResolveBaseColorTextureDependency(true, false, true)
            == BaseColorTextureDependency::ExternalRejected,
        "external texture dependency is rejected");
    Expect(
        assets::ResolveBaseColorTextureDependency(true, false, false)
            == BaseColorTextureDependency::Missing,
        "claimed texture without image is missing");

    {
        ModelMaterialPresentation imported{};
        imported.hasBaseColorTexture = true;
        imported.textureDependency = BaseColorTextureDependency::Missing;
        imported.baseColorR = 0.1f;
        imported.baseColorG = 0.2f;
        imported.baseColorB = 0.3f;
        imported.baseColorA = 1.0f;
        const ModelMaterialPresentation resolved = assets::ResolveMaterialFallback(imported);
        Expect(resolved.usedFallback, "missing texture uses fallback");
        Expect(!resolved.hasBaseColorTexture, "missing texture does not stay claimed");
        Expect(ColorNear(resolved, 0.1f, 0.2f, 0.3f, 1.0f), "missing texture keeps base color");
        Expect(
            resolved.textureDependency == BaseColorTextureDependency::Missing,
            "missing dependency is retained for diagnostics");
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        ExpectExtract(UntexturedColorGlb(), materials, "untextured color GLB extracts");
        Expect(materials.size() == 1, "untextured color has one material");
        if (!materials.empty())
        {
            Expect(!materials[0].usedFallback, "untextured color does not fallback");
            Expect(ColorNear(materials[0], 0.92f, 0.45f, 0.18f, 1.0f), "untextured color preserved");
            Expect(!materials[0].hasBaseColorTexture, "untextured color has no texture");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        ExpectExtract(DefaultMaterialGlb(), materials, "default material GLB extracts");
        Expect(materials.size() == 1, "default material count");
        if (!materials.empty())
        {
            Expect(!materials[0].usedFallback, "omitted pbr uses glTF white default, not failure");
            Expect(ColorNear(materials[0], 1.0f, 1.0f, 1.0f, 1.0f), "default material is white");
            Expect(!materials[0].hasBaseColorTexture, "default material is untextured");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        ExpectExtract(EmbeddedTextureGlb(), materials, "embedded textured GLB extracts");
        Expect(materials.size() == 1, "embedded textured count");
        if (!materials.empty())
        {
            Expect(materials[0].hasBaseColorTexture, "embedded texture is recognized");
            Expect(
                materials[0].textureDependency == BaseColorTextureDependency::Embedded,
                "embedded dependency");
            Expect(ColorNear(materials[0], 1.0f, 0.5f, 0.25f, 1.0f), "textured base color preserved");
            Expect(!materials[0].usedFallback, "valid embedded texture does not fallback");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        ExpectExtract(MissingTextureIndexGlb(), materials, "missing texture index extracts");
        Expect(materials.size() == 1, "missing texture material count");
        if (!materials.empty())
        {
            Expect(materials[0].usedFallback, "missing texture index falls back");
            Expect(!materials[0].hasBaseColorTexture, "missing texture is not claimed");
            Expect(
                materials[0].textureDependency == BaseColorTextureDependency::Missing,
                "missing texture dependency");
            Expect(ColorNear(materials[0], 0.1f, 0.2f, 0.3f, 1.0f), "missing texture keeps color");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        ExpectExtract(ZeroAlphaGlb(), materials, "zero-alpha GLB extracts");
        Expect(materials.size() == 1, "zero-alpha count");
        if (!materials.empty())
        {
            Expect(materials[0].usedFallback, "zero alpha fallback");
            Expect(ColorNear(materials[0], 1.0f, 1.0f, 1.0f, 1.0f), "zero alpha becomes opaque white");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        ExpectExtract(NoMaterialsGlb(), materials, "GLB with no materials extracts");
        Expect(materials.empty(), "no materials array is valid untextured");
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        std::string error;
        const assets::StaticGlbValidation validation =
            assets::ValidateStaticGlbBytes(ExternalImageGlb());
        Expect(
            validation.status == assets::StaticGlbStatus::Incompatible,
            "external image URI is incompatible");
        Expect(
            validation.message.find("embedded") != std::string::npos,
            "external image diagnostic names embedded requirement");
        Expect(
            !assets::TryExtractGlbMaterialPresentations(ExternalImageGlb(), materials, &error),
            "external image does not extract as a supported material");
        Expect(materials.empty(), "rejected external image leaves no materials");
    }

    const std::filesystem::path player = FixturePath(
#if defined(PLATFORMER_PLAYER_GLB)
        PLATFORMER_PLAYER_GLB
#else
        ""
#endif
    );
    const std::filesystem::path testStatic = FixturePath(
#if defined(PLATFORMER_TEST_STATIC_GLB)
        PLATFORMER_TEST_STATIC_GLB
#else
        ""
#endif
    );
    const std::filesystem::path testAuthored = FixturePath(
#if defined(PLATFORMER_TEST_AUTHORED_GLB)
        PLATFORMER_TEST_AUTHORED_GLB
#else
        ""
#endif
    );
    const std::filesystem::path testTextured = FixturePath(
#if defined(PLATFORMER_TEST_TEXTURED_GLB)
        PLATFORMER_TEST_TEXTURED_GLB
#else
        ""
#endif
    );

    Expect(std::filesystem::is_regular_file(player), "player.glb fixture exists");
    Expect(std::filesystem::is_regular_file(testStatic), "test_static.glb fixture exists");
    Expect(std::filesystem::is_regular_file(testAuthored), "test_authored.glb fixture exists");
    Expect(std::filesystem::is_regular_file(testTextured), "test_textured.glb fixture exists");

    {
        std::vector<ModelMaterialPresentation> materials;
        std::string error;
        Expect(
            assets::TryExtractGlbMaterialPresentationsFromFile(player, materials, &error),
            "player.glb extracts");
        Expect(materials.size() == 2, "player.glb has two untextured materials");
        if (materials.size() >= 2)
        {
            Expect(!materials[0].hasBaseColorTexture, "player body is untextured");
            Expect(!materials[1].hasBaseColorTexture, "player nose is untextured");
            Expect(ColorNear(materials[0], 0.847f, 0.376f, 0.282f, 1.0f), "player body color");
            Expect(ColorNear(materials[1], 0.18f, 0.12f, 0.10f, 1.0f), "player nose color");
            Expect(!materials[0].usedFallback && !materials[1].usedFallback, "player colors kept");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        Expect(
            assets::TryExtractGlbMaterialPresentationsFromFile(testStatic, materials, nullptr),
            "test_static.glb extracts");
        Expect(materials.size() == 1, "test_static has one material");
        if (!materials.empty())
        {
            Expect(!materials[0].hasBaseColorTexture, "test_static remains untextured");
            Expect(ColorNear(materials[0], 0.92f, 0.45f, 0.18f, 1.0f), "test_static base color");
            Expect(!materials[0].usedFallback, "test_static does not fallback");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        Expect(
            assets::TryExtractGlbMaterialPresentationsFromFile(testAuthored, materials, nullptr),
            "test_authored.glb extracts");
        Expect(materials.size() == 1, "test_authored has one material");
        if (!materials.empty())
        {
            Expect(!materials[0].hasBaseColorTexture, "test_authored is untextured");
            Expect(ColorNear(materials[0], 1.0f, 1.0f, 1.0f, 1.0f), "test_authored default white");
            Expect(!materials[0].usedFallback, "test_authored white is the glTF default");
        }
    }

    {
        std::vector<ModelMaterialPresentation> materials;
        Expect(
            assets::TryExtractGlbMaterialPresentationsFromFile(testTextured, materials, nullptr),
            "test_textured.glb extracts");
        Expect(materials.size() == 1, "test_textured has one material");
        if (!materials.empty())
        {
            Expect(materials[0].hasBaseColorTexture, "test_textured has Base Color Texture");
            Expect(
                materials[0].textureDependency == BaseColorTextureDependency::Embedded,
                "test_textured texture is embedded");
            Expect(ColorNear(materials[0], 1.0f, 1.0f, 1.0f, 1.0f), "test_textured default white factor");
            Expect(!materials[0].usedFallback, "test_textured does not fallback");
        }
        Expect(
            assets::ValidateStaticGlbFile(testTextured).status == assets::StaticGlbStatus::Ok,
            "test_textured remains a valid static GLB");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d ModelMaterialPresentationTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("ModelMaterialPresentationTest passed\n");
    return 0;
}
