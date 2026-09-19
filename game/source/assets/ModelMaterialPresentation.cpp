#include "assets/ModelMaterialPresentation.h"

#include <cstdint>
#include <fstream>

namespace assets
{
const char* BaseColorTextureDependencyName(BaseColorTextureDependency dependency)
{
    switch (dependency)
    {
    case BaseColorTextureDependency::None:
        return "none";
    case BaseColorTextureDependency::Embedded:
        return "embedded";
    case BaseColorTextureDependency::ExternalRejected:
        return "external_rejected";
    case BaseColorTextureDependency::Missing:
        return "missing";
    }
    return "unknown";
}

bool TryExtractGlbMaterialPresentationsFromFile(
    const std::filesystem::path& path,
    std::vector<ModelMaterialPresentation>& out,
    std::string* error)
{
    out.clear();
    if (path.empty())
    {
        if (error != nullptr)
        {
            *error = "input path is empty";
        }
        return false;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        if (error != nullptr)
        {
            *error = "input file could not be opened";
        }
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0)
    {
        if (error != nullptr)
        {
            *error = "input file size could not be read";
        }
        return false;
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (size > 0
        && !stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)))
    {
        if (error != nullptr)
        {
            *error = "input file could not be read";
        }
        return false;
    }
    return TryExtractGlbMaterialPresentations(bytes, out, error);
}
}
