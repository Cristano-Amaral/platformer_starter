#pragma once

// Milestone 84: engine-owned imported-material presentation. CPU-only.
// Not a Material Editor, PBR graph, authored .material format, or Level
// override. Runtime GPU objects stay on the raylib Model that loaded them.

#include <cmath>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace assets
{
enum class BaseColorTextureDependency
{
    None,
    Embedded,
    ExternalRejected,
    Missing,
};

const char* BaseColorTextureDependencyName(BaseColorTextureDependency dependency);

inline constexpr float kDefaultBaseColorR = 1.0f;
inline constexpr float kDefaultBaseColorG = 1.0f;
inline constexpr float kDefaultBaseColorB = 1.0f;
inline constexpr float kDefaultBaseColorA = 1.0f;

struct ModelMaterialPresentation
{
    float baseColorR = kDefaultBaseColorR;
    float baseColorG = kDefaultBaseColorG;
    float baseColorB = kDefaultBaseColorB;
    float baseColorA = kDefaultBaseColorA;
    bool hasBaseColorTexture = false;
    BaseColorTextureDependency textureDependency = BaseColorTextureDependency::None;
    bool usedFallback = false;
};

inline bool BaseColorChannelsAreUsable(const ModelMaterialPresentation& presentation)
{
    return std::isfinite(presentation.baseColorR) && std::isfinite(presentation.baseColorG)
        && std::isfinite(presentation.baseColorB) && std::isfinite(presentation.baseColorA)
        && presentation.baseColorR >= 0.0f && presentation.baseColorG >= 0.0f
        && presentation.baseColorB >= 0.0f && presentation.baseColorA > 0.0f
        && presentation.baseColorR <= 1.0f && presentation.baseColorG <= 1.0f
        && presentation.baseColorB <= 1.0f && presentation.baseColorA <= 1.0f;
}

inline BaseColorTextureDependency ResolveBaseColorTextureDependency(
    bool hasBaseColorTexture,
    bool textureEmbedded,
    bool textureExternal)
{
    if (!hasBaseColorTexture)
    {
        return BaseColorTextureDependency::None;
    }
    if (textureExternal)
    {
        return BaseColorTextureDependency::ExternalRejected;
    }
    if (textureEmbedded)
    {
        return BaseColorTextureDependency::Embedded;
    }
    return BaseColorTextureDependency::Missing;
}

inline ModelMaterialPresentation ResolveMaterialFallback(ModelMaterialPresentation imported)
{
    ModelMaterialPresentation result = imported;
    if (!BaseColorChannelsAreUsable(result))
    {
        result.baseColorR = kDefaultBaseColorR;
        result.baseColorG = kDefaultBaseColorG;
        result.baseColorB = kDefaultBaseColorB;
        result.baseColorA = kDefaultBaseColorA;
        result.usedFallback = true;
    }

    if (result.textureDependency == BaseColorTextureDependency::ExternalRejected
        || result.textureDependency == BaseColorTextureDependency::Missing)
    {
        result.hasBaseColorTexture = false;
        result.usedFallback = true;
    }
    else if (!result.hasBaseColorTexture)
    {
        result.textureDependency = BaseColorTextureDependency::None;
    }
    return result;
}

bool TryExtractGlbMaterialPresentations(
    std::span<const std::uint8_t> data,
    std::vector<ModelMaterialPresentation>& out,
    std::string* error = nullptr);
bool TryExtractGlbMaterialPresentationsFromFile(
    const std::filesystem::path& path,
    std::vector<ModelMaterialPresentation>& out,
    std::string* error = nullptr);
}
