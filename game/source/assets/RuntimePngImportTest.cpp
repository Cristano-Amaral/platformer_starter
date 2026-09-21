#include "assets/RuntimePng.h"
#include "assets/RuntimePngDelete.h"
#include "assets/RuntimePngImport.h"
#include "assets/SourceTextureCatalog.h"

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

std::filesystem::path TestCheckerPngPath()
{
#if defined(PLATFORMER_TEST_CHECKER_PNG)
    return std::filesystem::path{PLATFORMER_TEST_CHECKER_PNG}.lexically_normal();
#else
    return {};
#endif
}

std::filesystem::path MakeTempRoot()
{
    const std::filesystem::path root =
        (std::filesystem::temp_directory_path() / "platformer_m89_png_import").lexically_normal();
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

bool PathIsRegularFile(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

bool PathExists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

void WriteBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(
        reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

void CopyFile(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    std::filesystem::create_directories(destination.parent_path());
    std::filesystem::copy_file(
        source, destination, std::filesystem::copy_options::overwrite_existing);
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
}

int main()
{
    using assets::ImportRuntimePng;
    using assets::RuntimePngImportStatus;
    using assets::SourceTextureCatalog;

    const std::filesystem::path fixture = TestCheckerPngPath();
    Expect(PathIsRegularFile(fixture), "canonical test_checker.png fixture exists");

    const std::filesystem::path tempRoot = MakeTempRoot();
    const std::filesystem::path sourceRoot = (tempRoot / "source").lexically_normal();
    const std::filesystem::path cookedRoot = (tempRoot / "cooked").lexically_normal();
    const std::filesystem::path stagedRoot =
        (tempRoot / "staged" / "Development" / "assets").lexically_normal();
    std::filesystem::create_directories(sourceRoot);

    {
        SourceTextureCatalog catalog;
        catalog.Refresh(sourceRoot);
        Expect(catalog.Count() == 0, "empty textures root is a valid catalog");
    }

    {
        const std::filesystem::path external = tempRoot / "external" / "grass.png";
        CopyFile(fixture, external);
        SourceTextureCatalog catalog;
        const assets::RuntimePngImportResult imported =
            ImportRuntimePng(external, sourceRoot, &catalog, cookedRoot);
        Expect(
            imported.status == RuntimePngImportStatus::Imported, "valid PNG import succeeds");
        Expect(imported.canonicalIdentity == "textures/grass.png", "imported identity is textures/grass.png");
        Expect(imported.recipe == std::string(assets::kRuntimePngRecipe), "import records existing recipe");
        Expect(
            imported.cookStatus == assets::RuntimePngCookWriteStatus::CopiedUnchanged,
            "small PNG uses byte-identical recipe cook");
        Expect(catalog.Find("textures/grass.png") != nullptr, "catalog sees imported texture");
        Expect(
            PathIsRegularFile(sourceRoot / "textures" / "grass.png"), "source PNG was copied");
        Expect(
            PathIsRegularFile(cookedRoot / "textures" / "grass.png"), "cooked PNG was written");
        Expect(
            ReadBytes(sourceRoot / "textures" / "grass.png")
                == ReadBytes(cookedRoot / "textures" / "grass.png"),
            "within-512 cook is byte-identical to source");
        Expect(
            assets::CollectSourceRuntimePngIdentities(sourceRoot)
                == catalog.Identities(),
            "shared listing primitive matches the catalog");
    }

    {
        const std::filesystem::path jpg = tempRoot / "external" / "grass.jpg";
        CopyFile(fixture, jpg);
        const assets::RuntimePngImportResult imported = ImportRuntimePng(jpg, sourceRoot);
        Expect(
            imported.status == RuntimePngImportStatus::UnsupportedExtension,
            "non-png extension is rejected");
        Expect(!PathExists(sourceRoot / "textures" / "grass.jpg"), "rejected jpg is not copied");
    }

    {
        const std::filesystem::path upper = tempRoot / "external" / "Dirt.PNG";
        CopyFile(fixture, upper);
        const assets::RuntimePngImportResult imported = ImportRuntimePng(upper, sourceRoot);
        Expect(
            imported.status == RuntimePngImportStatus::UnsupportedExtension,
            "uppercase PNG extension is rejected");
    }

    {
        const std::filesystem::path spaced = tempRoot / "external" / "not safe.png";
        CopyFile(fixture, spaced);
        const assets::RuntimePngImportResult imported = ImportRuntimePng(spaced, sourceRoot);
        Expect(
            imported.status == RuntimePngImportStatus::UnsafeName
                || imported.status == RuntimePngImportStatus::UnsupportedExtension,
            "unsafe name is rejected");
    }

    {
        const std::filesystem::path junk = tempRoot / "external" / "junk.png";
        WriteBytes(junk, {'n', 'o', 't', ' ', 'p', 'n', 'g'});
        const assets::RuntimePngImportResult imported = ImportRuntimePng(junk, sourceRoot);
        Expect(imported.status == RuntimePngImportStatus::InvalidPng, "invalid PNG bytes rejected");
        Expect(!PathExists(sourceRoot / "textures" / "junk.png"), "invalid PNG is not copied");
    }

    {
        const std::filesystem::path again = tempRoot / "external" / "grass.png";
        CopyFile(fixture, again);
        const assets::RuntimePngImportResult imported = ImportRuntimePng(again, sourceRoot, nullptr, cookedRoot);
        Expect(imported.status == RuntimePngImportStatus::Collision, "existing identity does not overwrite");
        Expect(
            ReadBytes(sourceRoot / "textures" / "grass.png") == ReadBytes(fixture),
            "collision leaves the original source bytes");
    }

    {
        WriteBytes(sourceRoot / "textures" / "notes.txt", {'x'});
        std::filesystem::create_directories(sourceRoot / "textures" / "nested");
        CopyFile(fixture, sourceRoot / "textures" / "nested" / "hidden.png");
        WriteBytes(sourceRoot / "textures" / ".hidden.png", {0x89, 'P', 'N', 'G'});
        SourceTextureCatalog catalog;
        catalog.Refresh(sourceRoot);
        Expect(catalog.Find("textures/grass.png") != nullptr, "valid source PNG remains listed");
        Expect(catalog.Find("textures/notes.txt") == nullptr, "unrelated files are excluded");
        Expect(catalog.Find("textures/nested/hidden.png") == nullptr, "nested PNGs are excluded");
        Expect(catalog.Find("textures/.hidden.png") == nullptr, "hidden PNGs are excluded");
        const std::vector<std::string> identities = catalog.Identities();
        Expect(
            identities.size() >= 1 && identities == assets::CollectSourceRuntimePngIdentities(sourceRoot),
            "catalog ordering is deterministic and shared");
        for (std::size_t index = 1; index < identities.size(); ++index)
        {
            Expect(identities[index - 1] < identities[index], "catalog identities sort ascending");
        }
    }

    {
        CopyFile(fixture, sourceRoot / "textures" / "disposable.png");
        CopyFile(fixture, cookedRoot / "textures" / "disposable.png");
        CopyFile(fixture, stagedRoot / "textures" / "disposable.png");
        SourceTextureCatalog catalog;
        catalog.Refresh(sourceRoot);
        Expect(catalog.Find("textures/disposable.png") != nullptr, "disposable texture is listed");
        assets::RuntimePngDeleteRoots roots{};
        roots.sourceRoot = sourceRoot;
        roots.cookedRoot = cookedRoot;
        roots.stagedRoots = {stagedRoot};
        const assets::RuntimePngDeleteResult deleted =
            assets::DeleteRuntimePng("textures/disposable.png", roots, &catalog);
        Expect(assets::RuntimePngDeleteSucceeded(deleted.status), "physical texture delete succeeds");
        catalog.Refresh(sourceRoot);
        Expect(catalog.Find("textures/disposable.png") == nullptr, "catalog refresh after delete");
        Expect(!PathExists(sourceRoot / "textures" / "disposable.png"), "source deleted");
        Expect(!PathExists(cookedRoot / "textures" / "disposable.png"), "cooked deleted");
        Expect(!PathExists(stagedRoot / "textures" / "disposable.png"), "staged deleted");
        Expect(catalog.Find("textures/grass.png") != nullptr, "unrelated texture remains");
    }

    {
        std::string identity;
        std::filesystem::path destination;
        std::string error;
        Expect(
            !assets::TryResolveRuntimePngImportDestination(
                sourceRoot, "C:/abs/grass.png", identity, destination, error),
            "absolute name is rejected");
        Expect(
            !assets::TryParseRuntimePngIdentity("textures\\grass.png", identity, &error),
            "backslash identity is rejected");
        Expect(
            assets::TryResolveRuntimePngCookDestination(
                cookedRoot, "textures/grass.png", destination, error),
            "cook destination maps to cooked/textures/grass.png");
        Expect(
            destination.lexically_relative(cookedRoot).generic_string() == "textures/grass.png",
            "cook destination keeps runtime identity");
    }

    RemoveTree(tempRoot);
    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d RuntimePngImportTest checks failed\n", gFailures);
        return 1;
    }
    return 0;
}
