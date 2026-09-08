#include "assets/StaticGlb.h"

#include <cctype>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace assets
{
namespace
{
constexpr std::uint32_t kGlbMagic = 0x46546C67; // glTF little-endian
constexpr std::uint32_t kGlbVersion = 2;
constexpr std::uint32_t kJsonChunkType = 0x4E4F534A; // JSON
constexpr std::uint32_t kBinChunkType = 0x004E4942; // BIN\0
constexpr std::size_t kGlbHeaderSize = 12;
constexpr std::size_t kGlbChunkHeaderSize = 8;

struct JsonSlice
{
    std::string_view text;
};

void SkipWs(std::string_view text, std::size_t& index)
{
    while (index < text.size()
        && (text[index] == ' ' || text[index] == '\t' || text[index] == '\n'
            || text[index] == '\r'))
    {
        ++index;
    }
}

bool SkipJsonString(std::string_view text, std::size_t& index)
{
    if (index >= text.size() || text[index] != '"')
    {
        return false;
    }
    ++index;
    while (index < text.size())
    {
        const char ch = text[index];
        if (ch == '\\')
        {
            index += 2;
            continue;
        }
        if (ch == '"')
        {
            ++index;
            return true;
        }
        ++index;
    }
    return false;
}

bool SkipJsonValue(std::string_view text, std::size_t& index);

bool SkipJsonObjectOrArray(std::string_view text, std::size_t& index, char open, char close)
{
    if (index >= text.size() || text[index] != open)
    {
        return false;
    }
    ++index;
    SkipWs(text, index);
    if (index < text.size() && text[index] == close)
    {
        ++index;
        return true;
    }
    while (index < text.size())
    {
        if (open == '{')
        {
            if (!SkipJsonString(text, index))
            {
                return false;
            }
            SkipWs(text, index);
            if (index >= text.size() || text[index] != ':')
            {
                return false;
            }
            ++index;
            SkipWs(text, index);
        }
        if (!SkipJsonValue(text, index))
        {
            return false;
        }
        SkipWs(text, index);
        if (index >= text.size())
        {
            return false;
        }
        if (text[index] == ',')
        {
            ++index;
            SkipWs(text, index);
            continue;
        }
        if (text[index] == close)
        {
            ++index;
            return true;
        }
        return false;
    }
    return false;
}

bool SkipJsonValue(std::string_view text, std::size_t& index)
{
    SkipWs(text, index);
    if (index >= text.size())
    {
        return false;
    }
    const char ch = text[index];
    if (ch == '"')
    {
        return SkipJsonString(text, index);
    }
    if (ch == '{')
    {
        return SkipJsonObjectOrArray(text, index, '{', '}');
    }
    if (ch == '[')
    {
        return SkipJsonObjectOrArray(text, index, '[', ']');
    }
    if (ch == 't' || ch == 'f' || ch == 'n')
    {
        const std::string_view rest = text.substr(index);
        if (rest.starts_with("true"))
        {
            index += 4;
            return true;
        }
        if (rest.starts_with("false"))
        {
            index += 5;
            return true;
        }
        if (rest.starts_with("null"))
        {
            index += 4;
            return true;
        }
        return false;
    }
    if (ch == '-' || (ch >= '0' && ch <= '9'))
    {
        if (ch == '-')
        {
            ++index;
        }
        if (index >= text.size() || text[index] < '0' || text[index] > '9')
        {
            return false;
        }
        while (index < text.size() && text[index] >= '0' && text[index] <= '9')
        {
            ++index;
        }
        if (index < text.size() && text[index] == '.')
        {
            ++index;
            while (index < text.size() && text[index] >= '0' && text[index] <= '9')
            {
                ++index;
            }
        }
        if (index < text.size() && (text[index] == 'e' || text[index] == 'E'))
        {
            ++index;
            if (index < text.size() && (text[index] == '+' || text[index] == '-'))
            {
                ++index;
            }
            while (index < text.size() && text[index] >= '0' && text[index] <= '9')
            {
                ++index;
            }
        }
        return true;
    }
    return false;
}

bool DecodeJsonString(std::string_view quoted, std::string& decoded)
{
    if (quoted.size() < 2 || quoted.front() != '"' || quoted.back() != '"')
    {
        return false;
    }
    decoded.clear();
    for (std::size_t i = 1; i + 1 < quoted.size(); ++i)
    {
        const char ch = quoted[i];
        if (ch == '\\')
        {
            ++i;
            if (i + 1 >= quoted.size())
            {
                return false;
            }
            decoded.push_back(quoted[i]);
            continue;
        }
        decoded.push_back(ch);
    }
    return true;
}

bool FindObjectMember(std::string_view object, std::string_view key, JsonSlice& value)
{
    std::size_t index = 0;
    SkipWs(object, index);
    if (index >= object.size() || object[index] != '{')
    {
        return false;
    }
    ++index;
    SkipWs(object, index);
    if (index < object.size() && object[index] == '}')
    {
        return false;
    }
    while (index < object.size())
    {
        const std::size_t keyStart = index;
        if (!SkipJsonString(object, index))
        {
            return false;
        }
        std::string decoded;
        if (!DecodeJsonString(object.substr(keyStart, index - keyStart), decoded))
        {
            return false;
        }
        SkipWs(object, index);
        if (index >= object.size() || object[index] != ':')
        {
            return false;
        }
        ++index;
        SkipWs(object, index);
        const std::size_t valueStart = index;
        if (!SkipJsonValue(object, index))
        {
            return false;
        }
        if (decoded == key)
        {
            value.text = object.substr(valueStart, index - valueStart);
            return true;
        }
        SkipWs(object, index);
        if (index >= object.size())
        {
            return false;
        }
        if (object[index] == ',')
        {
            ++index;
            SkipWs(object, index);
            continue;
        }
        return false;
    }
    return false;
}

bool ArrayIsEmpty(std::string_view array)
{
    std::size_t index = 0;
    SkipWs(array, index);
    if (index >= array.size() || array[index] != '[')
    {
        return false;
    }
    ++index;
    SkipWs(array, index);
    return index < array.size() && array[index] == ']';
}

bool ArrayIsNonEmpty(std::string_view array)
{
    std::size_t index = 0;
    SkipWs(array, index);
    if (index >= array.size() || array[index] != '[')
    {
        return false;
    }
    ++index;
    SkipWs(array, index);
    return index < array.size() && array[index] != ']';
}

bool ForEachArrayObject(std::string_view array, const auto& visitor)
{
    std::size_t index = 0;
    SkipWs(array, index);
    if (index >= array.size() || array[index] != '[')
    {
        return false;
    }
    ++index;
    SkipWs(array, index);
    if (index < array.size() && array[index] == ']')
    {
        return true;
    }
    while (index < array.size())
    {
        SkipWs(array, index);
        const std::size_t start = index;
        if (!SkipJsonValue(array, index))
        {
            return false;
        }
        if (!visitor(array.substr(start, index - start)))
        {
            return false;
        }
        SkipWs(array, index);
        if (index >= array.size())
        {
            return false;
        }
        if (array[index] == ',')
        {
            ++index;
            continue;
        }
        return array[index] == ']';
    }
    return false;
}

std::uint32_t ReadU32(std::span<const std::uint8_t> data, std::size_t offset)
{
    return static_cast<std::uint32_t>(data[offset])
        | (static_cast<std::uint32_t>(data[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(data[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

StaticGlbValidation Fail(StaticGlbStatus status, std::string message)
{
    StaticGlbValidation result{};
    result.status = status;
    result.message = std::move(message);
    return result;
}

StaticGlbValidation Ok()
{
    StaticGlbValidation result{};
    result.status = StaticGlbStatus::Ok;
    result.message = "static GLB is compatible";
    return result;
}

bool HasUnsafeWindowsDeviceName(std::string_view stem)
{
    std::string upper;
    upper.reserve(stem.size());
    for (char ch : stem)
    {
        upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
    }
    const auto isDevice = [&](std::string_view name) {
        if (upper == name)
        {
            return true;
        }
        if (upper.size() == name.size() + 1 && upper.starts_with(name)
            && upper.back() >= '1' && upper.back() <= '9')
        {
            return name == "COM" || name == "LPT";
        }
        return false;
    };
    return isDevice("CON") || isDevice("PRN") || isDevice("AUX") || isDevice("NUL")
        || isDevice("COM") || isDevice("LPT");
}
}

const char* StaticGlbStatusName(StaticGlbStatus status)
{
    switch (status)
    {
    case StaticGlbStatus::Ok:
        return "Ok";
    case StaticGlbStatus::Missing:
        return "Missing";
    case StaticGlbStatus::NotAFile:
        return "NotAFile";
    case StaticGlbStatus::UnsupportedExtension:
        return "UnsupportedExtension";
    case StaticGlbStatus::UnsafeName:
        return "UnsafeName";
    case StaticGlbStatus::InvalidContainer:
        return "InvalidContainer";
    case StaticGlbStatus::Incompatible:
        return "Incompatible";
    }
    return "InvalidContainer";
}

bool HasStaticGlbExtension(std::string_view fileName)
{
    return fileName.size() > kStaticGlbExtension.size()
        && fileName.ends_with(kStaticGlbExtension);
}

bool IsSafeStaticGlbFileName(std::string_view fileName, std::string* reason)
{
    const auto fail = [&](const char* text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
        return false;
    };
    if (fileName.empty())
    {
        return fail("file name is empty");
    }
    if (!HasStaticGlbExtension(fileName))
    {
        return fail("file name must end with .glb");
    }
    const std::string_view stem =
        fileName.substr(0, fileName.size() - kStaticGlbExtension.size());
    if (stem.empty() || stem == "." || stem == "..")
    {
        return fail("file name stem is invalid");
    }
    if (stem.starts_with('.'))
    {
        return fail("hidden file names are not imported");
    }
    if (fileName.find(kStaticGlbImportTempSuffix) != std::string_view::npos)
    {
        return fail("temporary import files are not canonical assets");
    }
    for (char ch : fileName)
    {
        const unsigned char byte = static_cast<unsigned char>(ch);
        if (byte < 32 || ch == '/' || ch == '\\' || ch == ':' || ch == '*' || ch == '?'
            || ch == '"' || ch == '<' || ch == '>' || ch == '|')
        {
            return fail("file name contains an unsafe character");
        }
    }
    if (fileName.front() == ' ' || fileName.back() == ' ' || stem.back() == '.'
        || stem.back() == ' ')
    {
        return fail("file name has unsafe leading or trailing whitespace");
    }
    if (HasUnsafeWindowsDeviceName(stem))
    {
        return fail("file name is a reserved Windows device name");
    }
    return true;
}

std::string CanonicalStaticModelIdentity(std::string_view fileName)
{
    std::string identity;
    identity.reserve(kStaticModelsLogicalDirectory.size() + 1 + fileName.size());
    identity.append(kStaticModelsLogicalDirectory);
    identity.push_back('/');
    identity.append(fileName);
    return identity;
}

bool TryParseStaticModelIdentity(
    std::string_view canonicalIdentity,
    std::string& fileName,
    std::string* reason)
{
    fileName.clear();
    const auto fail = [&](const char* text) {
        if (reason != nullptr)
        {
            *reason = text;
        }
        return false;
    };
    if (canonicalIdentity.empty())
    {
        return fail("canonical identity is empty");
    }
    if (canonicalIdentity.find('\\') != std::string_view::npos)
    {
        return fail("canonical identity must use posix separators");
    }
    const std::string prefix = std::string(kStaticModelsLogicalDirectory) + "/";
    if (!canonicalIdentity.starts_with(prefix))
    {
        return fail("canonical identity must be models/<filename>.glb");
    }
    const std::string_view remainder = canonicalIdentity.substr(prefix.size());
    std::string nameReason;
    if (!IsSafeStaticGlbFileName(remainder, &nameReason))
    {
        return fail(nameReason.empty() ? "canonical identity file name is unsafe" : nameReason.c_str());
    }
    if (CanonicalStaticModelIdentity(remainder) != canonicalIdentity)
    {
        return fail("canonical identity is not normalized");
    }
    fileName = std::string(remainder);
    return true;
}

std::string StaticModelDisplayName(std::string_view canonicalIdentity)
{
    std::string fileName;
    if (!TryParseStaticModelIdentity(canonicalIdentity, fileName, nullptr))
    {
        return {};
    }
    return fileName;
}

StaticGlbValidation ValidateStaticGlbBytes(std::span<const std::uint8_t> data)
{
    if (data.size() < kGlbHeaderSize)
    {
        return Fail(StaticGlbStatus::InvalidContainer, "GLB is too small to contain a header");
    }
    if (ReadU32(data, 0) != kGlbMagic)
    {
        return Fail(StaticGlbStatus::InvalidContainer, "not a GLB (missing glTF magic)");
    }
    const std::uint32_t version = ReadU32(data, 4);
    if (version != kGlbVersion)
    {
        return Fail(
            StaticGlbStatus::Incompatible,
            "unsupported GLB version (expected glTF 2.0 binary)");
    }
    const std::uint32_t declaredLength = ReadU32(data, 8);
    if (declaredLength != data.size())
    {
        return Fail(StaticGlbStatus::InvalidContainer, "GLB length does not match file size");
    }

    std::string_view json;
    bool sawJson = false;
    bool sawBin = false;
    std::size_t offset = kGlbHeaderSize;
    while (offset < data.size())
    {
        if (offset + kGlbChunkHeaderSize > data.size())
        {
            return Fail(StaticGlbStatus::InvalidContainer, "truncated GLB chunk header");
        }
        const std::uint32_t chunkLength = ReadU32(data, offset);
        const std::uint32_t chunkType = ReadU32(data, offset + 4);
        offset += kGlbChunkHeaderSize;
        if (offset + chunkLength > data.size())
        {
            return Fail(StaticGlbStatus::InvalidContainer, "truncated GLB chunk payload");
        }
        if ((chunkLength % 4) != 0)
        {
            return Fail(StaticGlbStatus::InvalidContainer, "GLB chunk length is not 4-byte aligned");
        }
        if (chunkType == kJsonChunkType)
        {
            if (sawJson)
            {
                return Fail(StaticGlbStatus::InvalidContainer, "GLB contains more than one JSON chunk");
            }
            json = std::string_view(
                reinterpret_cast<const char*>(data.data() + offset), chunkLength);
            sawJson = true;
        }
        else if (chunkType == kBinChunkType)
        {
            sawBin = true;
        }
        else
        {
            return Fail(StaticGlbStatus::Incompatible, "GLB contains an unsupported chunk type");
        }
        offset += chunkLength;
    }
    if (!sawJson)
    {
        return Fail(StaticGlbStatus::InvalidContainer, "GLB is missing the JSON chunk");
    }
    if (!sawBin)
    {
        return Fail(
            StaticGlbStatus::Incompatible, "static GLB must embed binary data in a BIN chunk");
    }
    while (!json.empty() && (json.back() == ' ' || json.back() == '\0'))
    {
        json.remove_suffix(1);
    }
    if (json.empty() || json.front() != '{')
    {
        return Fail(StaticGlbStatus::InvalidContainer, "GLB JSON chunk is not an object");
    }

    JsonSlice asset{};
    if (!FindObjectMember(json, "asset", asset) || asset.text.empty() || asset.text.front() != '{')
    {
        return Fail(StaticGlbStatus::Incompatible, "GLB JSON is missing asset.version");
    }
    JsonSlice versionSlice{};
    if (!FindObjectMember(asset.text, "version", versionSlice))
    {
        return Fail(StaticGlbStatus::Incompatible, "GLB JSON is missing asset.version");
    }
    std::string versionText;
    if (!DecodeJsonString(versionSlice.text, versionText) || !versionText.starts_with("2"))
    {
        return Fail(StaticGlbStatus::Incompatible, "GLB JSON is not glTF 2.x");
    }

    JsonSlice meshes{};
    if (!FindObjectMember(json, "meshes", meshes) || !ArrayIsNonEmpty(meshes.text))
    {
        return Fail(StaticGlbStatus::Incompatible, "static GLB must contain at least one mesh");
    }

    JsonSlice animations{};
    if (FindObjectMember(json, "animations", animations) && !ArrayIsEmpty(animations.text))
    {
        return Fail(StaticGlbStatus::Incompatible, "animated GLB files are not supported");
    }
    JsonSlice skins{};
    if (FindObjectMember(json, "skins", skins) && !ArrayIsEmpty(skins.text))
    {
        return Fail(StaticGlbStatus::Incompatible, "skeletal / skinned GLB files are not supported");
    }

    JsonSlice buffers{};
    if (FindObjectMember(json, "buffers", buffers))
    {
        const bool buffersOk = ForEachArrayObject(buffers.text, [](std::string_view item) {
            if (item.empty() || item.front() != '{')
            {
                return false;
            }
            JsonSlice uri{};
            if (!FindObjectMember(item, "uri", uri))
            {
                return true;
            }
            std::string uriText;
            if (!DecodeJsonString(uri.text, uriText) || !uriText.empty())
            {
                return false;
            }
            return true;
        });
        if (!buffersOk)
        {
            return Fail(
                StaticGlbStatus::Incompatible,
                "GLB must be self-contained (no external buffer URIs)");
        }
    }

    JsonSlice images{};
    if (FindObjectMember(json, "images", images))
    {
        const bool imagesOk = ForEachArrayObject(images.text, [](std::string_view item) {
            if (item.empty() || item.front() != '{')
            {
                return false;
            }
            JsonSlice uri{};
            if (FindObjectMember(item, "uri", uri))
            {
                std::string uriText;
                if (!DecodeJsonString(uri.text, uriText))
                {
                    return false;
                }
                return uriText.starts_with("data:");
            }
            JsonSlice bufferView{};
            return FindObjectMember(item, "bufferView", bufferView);
        });
        if (!imagesOk)
        {
            return Fail(
                StaticGlbStatus::Incompatible,
                "GLB images must be embedded (bufferView or data URI); external files are rejected");
        }
    }

    std::size_t walk = 0;
    SkipWs(json, walk);
    if (!SkipJsonValue(json, walk))
    {
        return Fail(StaticGlbStatus::InvalidContainer, "GLB JSON is malformed");
    }
    SkipWs(json, walk);
    if (walk != json.size())
    {
        return Fail(StaticGlbStatus::InvalidContainer, "GLB JSON has trailing garbage");
    }

    return Ok();
}

StaticGlbValidation ValidateStaticGlbFile(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return Fail(StaticGlbStatus::Missing, "input path is empty");
    }
    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
    {
        return Fail(StaticGlbStatus::Missing, "input path does not exist");
    }
    if (!std::filesystem::is_regular_file(path, error) || error)
    {
        return Fail(StaticGlbStatus::NotAFile, "input path is not a regular file");
    }

    const std::string fileName = path.filename().string();
    if (!HasStaticGlbExtension(fileName))
    {
        return Fail(StaticGlbStatus::UnsupportedExtension, "only self-contained .glb files are supported");
    }
    std::string nameReason;
    if (!IsSafeStaticGlbFileName(fileName, &nameReason))
    {
        return Fail(StaticGlbStatus::UnsafeName, nameReason);
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return Fail(StaticGlbStatus::Missing, "input file could not be opened");
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0)
    {
        return Fail(StaticGlbStatus::InvalidContainer, "input file size could not be read");
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (size > 0
        && !stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size)))
    {
        return Fail(StaticGlbStatus::InvalidContainer, "input file could not be read");
    }
    return ValidateStaticGlbBytes(bytes);
}
}
