#include "assets/StaticGlb.h"
#include "assets/StaticGlbImport.h"
#include "assets/StaticModelCatalog.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
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

std::filesystem::path TestStaticGlbPath()
{
#if defined(PLATFORMER_TEST_STATIC_GLB)
    return std::filesystem::path{PLATFORMER_TEST_STATIC_GLB}.lexically_normal();
#else
    return {};
#endif
}

std::filesystem::path MakeTempRoot()
{
    const std::filesystem::path root =
        (std::filesystem::temp_directory_path() / "platformer_m47_import").lexically_normal();
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);
    return root;
}

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::remove_all(path, error);
}

void WriteBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void WriteText(const std::filesystem::path& path, std::string_view text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size <= 0)
    {
        return {};
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
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

const char* kMinimalStaticJson =
    "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],\"buffers\":[{\"byteLength\":4}]}";

std::vector<std::uint8_t> MinimalStaticGlb()
{
    return PackGlb(kMinimalStaticJson, {0, 0, 0, 0});
}

std::vector<std::uint8_t> AnimatedGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],\"animations\":[{}],"
        "\"buffers\":[{\"byteLength\":4}]}",
        {0, 0, 0, 0});
}

std::vector<std::uint8_t> ExternalBufferGlb()
{
    return PackGlb(
        "{\"asset\":{\"version\":\"2.0\"},\"meshes\":[{}],"
        "\"buffers\":[{\"byteLength\":4,\"uri\":\"mesh.bin\"}]}",
        {0, 0, 0, 0});
}

bool FilesEqual(const std::filesystem::path& left, const std::filesystem::path& right)
{
    return ReadBytes(left) == ReadBytes(right);
}
}

int main()
{
    using assets::ImportStaticGlb;
    using assets::StaticGlbImportStatus;
    using assets::StaticModelCatalog;

    const std::filesystem::path fixture = TestStaticGlbPath();
    Expect(std::filesystem::is_regular_file(fixture), "canonical test_static.glb fixture exists");
    const assets::StaticGlbValidation fixtureOk = assets::ValidateStaticGlbFile(fixture);
    Expect(fixtureOk.status == assets::StaticGlbStatus::Ok, "canonical test_static.glb validates");

    const std::filesystem::path tempRoot = MakeTempRoot();
    const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
    std::filesystem::create_directories(sourceRoot);

    {
        StaticModelCatalog catalog;
        catalog.Refresh(sourceRoot);
        Expect(catalog.Count() == 0, "empty models directory is a valid catalog");
        Expect(catalog.Entries().empty(), "empty catalog has no entries");
    }

    {
        const std::filesystem::path external = tempRoot / "external" / "crate.glb";
        WriteBytes(external, ReadBytes(fixture));
        StaticModelCatalog catalog;
        const assets::StaticGlbImportResult imported = ImportStaticGlb(external, sourceRoot, &catalog);
        Expect(imported.status == StaticGlbImportStatus::Imported, "valid static GLB imports");
        Expect(imported.canonicalIdentity == "models/crate.glb", "identity is models/<filename>.glb");
        Expect(imported.canonicalIdentity.find(':') == std::string::npos, "identity has no drive letter");
        Expect(imported.canonicalIdentity.find('\\') == std::string::npos, "identity uses posix separators");
        Expect(!imported.destinationPath.empty() && imported.destinationPath.is_absolute(),
            "destination is absolute");
        Expect(imported.destinationPath.generic_string().find(external.generic_string())
                == std::string::npos,
            "identity/destination do not retain the original external path");
        Expect(std::filesystem::is_regular_file(imported.destinationPath), "canonical source file exists");
        Expect(FilesEqual(external, imported.destinationPath), "imported bytes match the source GLB");
        Expect(catalog.Count() == 1, "imported asset appears once");
        const assets::StaticModelCatalogEntry* found = catalog.Find("models/crate.glb");
        Expect(found != nullptr, "catalog finds imported identity");
        Expect(found != nullptr && found->assetType == "static_glb", "asset type is static_glb");
        Expect(
            catalog.Entries()[0].canonicalIdentity == "models/crate.glb",
            "single entry identity matches");
    }

    {
        const std::filesystem::path zebra = tempRoot / "external" / "zebra.glb";
        const std::filesystem::path alpha = tempRoot / "external" / "alpha.glb";
        WriteBytes(zebra, MinimalStaticGlb());
        WriteBytes(alpha, MinimalStaticGlb());
        StaticModelCatalog catalog;
        Expect(ImportStaticGlb(zebra, sourceRoot, &catalog).status == StaticGlbImportStatus::Imported,
            "second valid import zebra");
        Expect(ImportStaticGlb(alpha, sourceRoot, &catalog).status == StaticGlbImportStatus::Imported,
            "third valid import alpha");
        Expect(catalog.Count() == 3, "catalog contains crate, alpha, zebra");
        Expect(catalog.Entries()[0].canonicalIdentity == "models/alpha.glb", "order 0 alpha");
        Expect(catalog.Entries()[1].canonicalIdentity == "models/crate.glb", "order 1 crate");
        Expect(catalog.Entries()[2].canonicalIdentity == "models/zebra.glb", "order 2 zebra");
        catalog.Refresh(sourceRoot);
        Expect(catalog.Count() == 3, "refresh keeps the same three entries");
        Expect(catalog.Entries()[0].canonicalIdentity == "models/alpha.glb", "refresh keeps sort");
    }

    {
        const std::filesystem::path png = tempRoot / "external" / "picture.png";
        WriteText(png, "not a glb");
        const assets::StaticGlbImportResult rejected = ImportStaticGlb(png, sourceRoot);
        Expect(rejected.status == StaticGlbImportStatus::UnsupportedExtension, "png extension rejected");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "picture.png"), "png was not copied");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "picture.glb"), "png did not become a glb");
    }

    {
        const assets::StaticGlbImportResult missing =
            ImportStaticGlb(tempRoot / "missing" / "nope.glb", sourceRoot);
        Expect(missing.status == StaticGlbImportStatus::Missing, "missing file rejected");
        const std::filesystem::path directoryInput = tempRoot / "external" / "dir.glb";
        std::filesystem::create_directories(directoryInput);
        const assets::StaticGlbImportResult notFile = ImportStaticGlb(directoryInput, sourceRoot);
        Expect(notFile.status == StaticGlbImportStatus::NotAFile, "directory input rejected");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "dir.glb"), "directory was not imported");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "nope.glb"), "missing input left no dest");
    }

    {
        std::string identity;
        std::filesystem::path destination;
        std::string error;
        Expect(
            !assets::TryResolveStaticGlbImportDestination(
                sourceRoot, "../escape.glb", identity, destination, error),
            "slash traversal file name rejected");
        Expect(
            !assets::TryResolveStaticGlbImportDestination(
                sourceRoot, "..\\escape.glb", identity, destination, error),
            "backslash traversal file name rejected");
        Expect(
            !assets::TryResolveStaticGlbImportDestination(
                std::filesystem::path("relative-root"), "crate.glb", identity, destination, error),
            "relative source root rejected");
        Expect(
            !assets::IsSafeStaticGlbFileName("models/../secret.glb"),
            "identity-like traversal name is unsafe");
    }

    {
        const std::filesystem::path original = sourceRoot / "models" / "crate.glb";
        const auto originalBytes = ReadBytes(original);
        const std::filesystem::path again = tempRoot / "external" / "crate.glb";
        const assets::StaticGlbImportResult collision = ImportStaticGlb(again, sourceRoot);
        Expect(collision.status == StaticGlbImportStatus::Collision, "existing destination collides");
        Expect(ReadBytes(original) == originalBytes, "collision does not overwrite existing bytes");
        Expect(std::filesystem::is_regular_file(original), "canonical file remains after collision");
    }

    {
        const std::filesystem::path bad = tempRoot / "external" / "broken.glb";
        WriteText(bad, "glTF this is not a real container");
        StaticModelCatalog catalog;
        catalog.Refresh(sourceRoot);
        const std::size_t before = catalog.Count();
        const assets::StaticGlbImportResult invalid = ImportStaticGlb(bad, sourceRoot, &catalog);
        Expect(
            invalid.status == StaticGlbImportStatus::InvalidContainer
                || invalid.status == StaticGlbImportStatus::Incompatible,
            "invalid GLB rejected");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "broken.glb"), "invalid GLB not copied");
        catalog.Refresh(sourceRoot);
        Expect(catalog.Count() == before, "invalid import does not add a catalog entry");
        Expect(catalog.Find("models/broken.glb") == nullptr, "invalid identity is absent");
    }

    {
        const std::filesystem::path animated = tempRoot / "external" / "hero.glb";
        WriteBytes(animated, AnimatedGlb());
        StaticModelCatalog catalog;
        catalog.Refresh(sourceRoot);
        const std::size_t countBefore = catalog.Count();
        const assets::StaticGlbImportResult rejected = ImportStaticGlb(animated, sourceRoot, &catalog);
        Expect(rejected.status == StaticGlbImportStatus::Incompatible, "animated GLB rejected");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "hero.glb"), "animated GLB not copied");
        catalog.Refresh(sourceRoot);
        Expect(catalog.Count() == countBefore, "animated import leaves catalog unchanged");
    }

    {
        const std::filesystem::path externalUri = tempRoot / "external" / "sidecar.glb";
        WriteBytes(externalUri, ExternalBufferGlb());
        const assets::StaticGlbImportResult rejected = ImportStaticGlb(externalUri, sourceRoot);
        Expect(rejected.status == StaticGlbImportStatus::Incompatible, "external buffer URI rejected");
        Expect(!std::filesystem::exists(sourceRoot / "models" / "sidecar.glb"), "sidecar GLB not copied");
    }

    {
        WriteText(sourceRoot / "models" / "notes.txt", "not an asset");
        WriteText(sourceRoot / "models" / "mesh.gltf", "{}");
        WriteBytes(sourceRoot / "models" / "junk.glb", {1, 2, 3, 4});
        StaticModelCatalog catalog;
        catalog.Refresh(sourceRoot);
        Expect(catalog.Find("models/notes.txt") == nullptr, "txt is not a static model entry");
        Expect(catalog.Find("models/mesh.gltf") == nullptr, "gltf json is not a static model entry");
        Expect(catalog.Find("models/junk.glb") == nullptr, "invalid glb is excluded from catalog");
        Expect(catalog.Find("models/crate.glb") != nullptr, "valid imported glb remains");
    }

    {
        StaticModelCatalog catalog;
        catalog.Refresh(sourceRoot);
        const std::size_t before = catalog.Count();
        std::filesystem::remove(sourceRoot / "models" / "alpha.glb");
        catalog.Refresh(sourceRoot);
        Expect(catalog.Count() == before - 1, "refresh drops a deleted source file");
        Expect(catalog.Find("models/alpha.glb") == nullptr, "deleted identity is gone");
        Expect(catalog.Find("models/crate.glb") != nullptr, "remaining identity survives refresh");
    }

    {
        const std::filesystem::path stagedRoot = tempRoot / "staged" / "assets";
        const std::filesystem::path stagedCopy = stagedRoot / "models" / "crate.glb";
        const std::string sourceIdentity = "models/crate.glb";
        Expect(
            (sourceRoot / "models" / "crate.glb") != stagedCopy,
            "canonical source path is not the staged runtime path");
        Expect(sourceIdentity.find(':') == std::string::npos, "source identity stays relative");
    }

    {
        const std::filesystem::path extra = tempRoot / "external" / "prop.glb";
        WriteBytes(extra, MinimalStaticGlb());
        const assets::StaticGlbImportResult imported = ImportStaticGlb(extra, sourceRoot);
        Expect(imported.status == StaticGlbImportStatus::Imported, "immutability import succeeds");
        Expect(
            imported.message.find("not a level object") != std::string::npos,
            "success message states that import is not a level object");
    }

    {
        Expect(assets::CanonicalStaticModelIdentity("Box.glb") == "models/Box.glb",
            "identity preserves filename case");
        Expect(!assets::HasStaticGlbExtension("Box.GLB"), "uppercase .GLB is unsupported");
        Expect(!assets::IsSafeStaticGlbFileName("Box.GLB"), "uppercase extension is rejected");
        Expect(assets::HasStaticGlbExtension("Box.glb"), "lowercase .glb is required");
    }

    {
        const std::filesystem::path leftover = sourceRoot / "models" / "prop.glb.importing.tmp";
        Expect(!std::filesystem::exists(leftover), "successful import leaves no temp sibling");
        StaticModelCatalog catalog;
        catalog.Refresh(sourceRoot);
        for (const auto& entry : catalog.Entries())
        {
            Expect(
                entry.canonicalIdentity.find(".importing.tmp") == std::string::npos,
                "catalog never lists import temporaries");
        }
    }

    RemoveTree(tempRoot);
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d StaticGlbImportTest failure(s)\n", gFailures);
        return 1;
    }
    std::printf("StaticGlbImportTest passed\n");
    return 0;
}
