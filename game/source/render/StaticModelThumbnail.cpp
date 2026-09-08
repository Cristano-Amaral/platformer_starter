#include "render/StaticModelThumbnail.h"

#include "assets/StaticGlb.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>
#include <string>

namespace render
{
namespace
{
constexpr int kMaxGenerationsPerFrame = 1;
constexpr Color kThumbnailBackground{56, 60, 72, 255};

bool ModelHasRenderableMesh(const Model& model)
{
    if (model.meshCount <= 0 || model.meshes == nullptr)
    {
        return false;
    }
    for (int i = 0; i < model.meshCount; ++i)
    {
        if (model.meshes[i].vertexCount > 0)
        {
            return true;
        }
    }
    return false;
}

editor::ThumbnailModelBounds BoundsFromRaylib(const BoundingBox& box)
{
    editor::ThumbnailModelBounds bounds{};
    bounds.min = {box.min.x, box.min.y, box.min.z};
    bounds.max = {box.max.x, box.max.y, box.max.z};
    return bounds;
}

Camera3D CameraFromFrame(const editor::ThumbnailCameraFrame& frame)
{
    Camera3D camera{};
    camera.position = Vector3{frame.position.x, frame.position.y, frame.position.z};
    camera.target = Vector3{frame.target.x, frame.target.y, frame.target.z};
    camera.up = Vector3{frame.up.x, frame.up.y, frame.up.z};
    camera.fovy = frame.fieldOfViewY;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}
}

StaticModelThumbnailStore::~StaticModelThumbnailStore()
{
    Shutdown();
}

void StaticModelThumbnailStore::BeginFrame()
{
    generationsThisFrame = 0;
}

void StaticModelThumbnailStore::Shutdown()
{
    for (auto& pair : entries)
    {
        UnloadEntry(pair.second);
    }
    entries.clear();
}

void StaticModelThumbnailStore::UnloadEntry(Entry& entry)
{
    if (entry.gpuId != 0)
    {
        Texture2D texture{};
        texture.id = entry.gpuId;
        texture.width = entry.width;
        texture.height = entry.height;
        texture.mipmaps = 1;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        UnloadTexture(texture);
        entry.gpuId = 0;
        entry.width = 0;
        entry.height = 0;
    }
}

bool StaticModelThumbnailStore::LoadFromCache(
    Entry& entry,
    std::string_view canonicalIdentity,
    const editor::ThumbnailSourceStamp& stamp,
    const std::filesystem::path& cacheRoot)
{
    std::filesystem::path imagePath;
    std::filesystem::path metaPath;
    std::string error;
    if (!editor::TryResolveThumbnailCachePaths(
            canonicalIdentity, cacheRoot, imagePath, metaPath, error))
    {
        return false;
    }
    if (!editor::ThumbnailCacheIsValid(canonicalIdentity, stamp, cacheRoot))
    {
        return false;
    }
    const Texture2D texture = LoadTexture(imagePath.string().c_str());
    if (texture.id == 0)
    {
        return false;
    }
    UnloadEntry(entry);
    entry.gpuId = texture.id;
    entry.width = texture.width;
    entry.height = texture.height;
    entry.stamp = stamp;
    entry.failed = false;
    return true;
}

bool StaticModelThumbnailStore::Generate(
    Entry& entry,
    std::string_view canonicalIdentity,
    const std::filesystem::path& sourcePath,
    const editor::ThumbnailSourceStamp& stamp,
    const std::filesystem::path& cacheRoot)
{
    const assets::StaticGlbValidation validation = assets::ValidateStaticGlbFile(sourcePath);
    if (validation.status != assets::StaticGlbStatus::Ok)
    {
        return false;
    }

    const Model model = LoadModel(sourcePath.string().c_str());
    if (!ModelHasRenderableMesh(model))
    {
        UnloadModel(model);
        return false;
    }

    const BoundingBox box = GetModelBoundingBox(model);
    const editor::ThumbnailCameraFrame frame =
        editor::MakeThumbnailCameraFrame(BoundsFromRaylib(box));
    if (!std::isfinite(frame.position.x) || !std::isfinite(frame.position.y)
        || !std::isfinite(frame.position.z) || !std::isfinite(frame.target.x)
        || !std::isfinite(frame.fieldOfViewY))
    {
        UnloadModel(model);
        return false;
    }

    const RenderTexture2D target = LoadRenderTexture(
        editor::kStaticModelThumbnailWidth, editor::kStaticModelThumbnailHeight);
    if (target.id == 0)
    {
        UnloadModel(model);
        return false;
    }

    const Camera3D camera = CameraFromFrame(frame);
    BeginTextureMode(target);
    ClearBackground(kThumbnailBackground);
    rlSetClipPlanes(static_cast<double>(frame.nearPlane), static_cast<double>(frame.farPlane));
    BeginMode3D(camera);
    DrawModel(model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    EndMode3D();
    EndTextureMode();
    UnloadModel(model);

    Image image = LoadImageFromTexture(target.texture);
    UnloadRenderTexture(target);
    if (image.data == nullptr)
    {
        return false;
    }
    ImageFlipVertical(&image);

    std::filesystem::path imagePath;
    std::filesystem::path metaPath;
    std::string error;
    bool wroteCache = false;
    if (editor::TryResolveThumbnailCachePaths(
            canonicalIdentity, cacheRoot, imagePath, metaPath, error))
    {
        std::error_code createError;
        std::filesystem::create_directories(cacheRoot, createError);
        if (!createError)
        {
            wroteCache = ExportImage(image, imagePath.string().c_str());
            if (wroteCache)
            {
                editor::ThumbnailCacheMeta meta{};
                meta.schemaVersion = editor::kStaticModelThumbnailSchemaVersion;
                meta.canonicalIdentity = std::string(canonicalIdentity);
                meta.stamp = stamp;
                editor::WriteThumbnailCacheMeta(metaPath, meta);
            }
        }
    }
    (void)wroteCache;

    const Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id == 0)
    {
        return false;
    }

    UnloadEntry(entry);
    entry.gpuId = texture.id;
    entry.width = texture.width;
    entry.height = texture.height;
    entry.stamp = stamp;
    entry.failed = false;
    return true;
}

void StaticModelThumbnailStore::Ensure(
    std::string_view canonicalIdentity,
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& cacheRoot)
{
    if (canonicalIdentity.empty())
    {
        return;
    }
    const std::string identity(canonicalIdentity);
    Entry& entry = entries[identity];

    editor::ThumbnailSourceStamp stamp{};
    const bool haveStamp = editor::ReadThumbnailSourceStamp(sourcePath, stamp);
    const editor::ThumbnailEnsureDecision decision = editor::ClassifyThumbnailEnsure(
        entry.gpuId != 0, entry.failed, haveStamp, entry.stamp, stamp);
    if (decision == editor::ThumbnailEnsureDecision::ReuseReady
        || decision == editor::ThumbnailEnsureDecision::ReuseFailed)
    {
        return;
    }

    UnloadEntry(entry);
    entry.failed = false;
    if (haveStamp && LoadFromCache(entry, identity, stamp, cacheRoot))
    {
        return;
    }
    if (generationsThisFrame >= kMaxGenerationsPerFrame)
    {
        return;
    }
    ++generationsThisFrame;
    if (!haveStamp || !Generate(entry, identity, sourcePath, stamp, cacheRoot))
    {
        UnloadEntry(entry);
        entry.failed = true;
        entry.stamp = stamp;
        if (lastFailureMessage.empty())
        {
            lastFailureMessage = "Thumbnail generation failed for " + identity + ".";
        }
    }
}

unsigned int StaticModelThumbnailStore::TextureGpuId(std::string_view canonicalIdentity) const
{
    const auto found = entries.find(std::string(canonicalIdentity));
    if (found == entries.end())
    {
        return 0;
    }
    return found->second.gpuId;
}

bool StaticModelThumbnailStore::HasReadyTexture(std::string_view canonicalIdentity) const
{
    return TextureGpuId(canonicalIdentity) != 0;
}

bool StaticModelThumbnailStore::IsFailed(std::string_view canonicalIdentity) const
{
    const auto found = entries.find(std::string(canonicalIdentity));
    return found != entries.end() && found->second.failed;
}

void StaticModelThumbnailStore::Forget(std::string_view canonicalIdentity)
{
    const auto found = entries.find(std::string(canonicalIdentity));
    if (found == entries.end())
    {
        return;
    }
    UnloadEntry(found->second);
    entries.erase(found);
}

void StaticModelThumbnailStore::AllowRetryAll()
{
    for (auto& pair : entries)
    {
        if (pair.second.failed)
        {
            pair.second.failed = false;
            pair.second.stamp = {};
        }
    }
}

bool StaticModelThumbnailStore::ConsumeLastFailure(std::string& message)
{
    if (lastFailureMessage.empty())
    {
        return false;
    }
    message = lastFailureMessage;
    lastFailureMessage.clear();
    return true;
}
}
