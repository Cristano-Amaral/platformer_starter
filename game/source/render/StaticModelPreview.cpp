#include "render/StaticModelPreview.h"

#include "assets/StaticGlb.h"

#include "raylib.h"
#include "rlgl.h"

#include <cmath>

namespace render
{
namespace
{
constexpr Color kPreviewBackground{56, 60, 72, 255};

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
    editor::ThumbnailModelBounds result{};
    result.min = {box.min.x, box.min.y, box.min.z};
    result.max = {box.max.x, box.max.y, box.max.z};
    return result;
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

struct StaticModelPreviewRenderer::GpuState
{
    Model model{};
    RenderTexture2D target{};
    bool hasTarget = false;
};

StaticModelPreviewRenderer::StaticModelPreviewRenderer()
    : gpu(std::make_unique<GpuState>())
{
}

StaticModelPreviewRenderer::~StaticModelPreviewRenderer()
{
    Shutdown();
}

void StaticModelPreviewRenderer::ReleaseLoadedModel()
{
    if (hasModel && gpu)
    {
        ::UnloadModel(gpu->model);
        gpu->model = {};
    }
    hasModel = false;
    bounds = {};
}

void StaticModelPreviewRenderer::ReleaseTarget()
{
    if (gpu && gpu->hasTarget)
    {
        UnloadRenderTexture(gpu->target);
        gpu->target = {};
        gpu->hasTarget = false;
    }
    targetWidth = 0;
    targetHeight = 0;
}

void StaticModelPreviewRenderer::Shutdown()
{
    ReleaseLoadedModel();
    ReleaseTarget();
    loadedIdentity.clear();
    stamp = {};
    failed = false;
}

void StaticModelPreviewRenderer::Clear()
{
    ReleaseLoadedModel();
    loadedIdentity.clear();
    stamp = {};
    failed = false;
}

void StaticModelPreviewRenderer::AllowRetry()
{
    if (failed)
    {
        failed = false;
        stamp = {};
    }
}

bool StaticModelPreviewRenderer::LoadModelFromSource(const std::filesystem::path& sourcePath)
{
    const assets::StaticGlbValidation validation = assets::ValidateStaticGlbFile(sourcePath);
    if (validation.status != assets::StaticGlbStatus::Ok)
    {
        return false;
    }

    Model model = LoadModel(sourcePath.string().c_str());
    if (!ModelHasRenderableMesh(model))
    {
        ::UnloadModel(model);
        return false;
    }

    ReleaseLoadedModel();
    gpu->model = model;
    hasModel = true;
    bounds = editor::SanitizeStaticModelBounds(BoundsFromRaylib(GetModelBoundingBox(model)));
    return true;
}

void StaticModelPreviewRenderer::Sync(
    std::string_view canonicalIdentity,
    const std::filesystem::path& sourcePath)
{
    editor::ThumbnailSourceStamp sourceStamp{};
    const bool haveStamp = editor::ReadThumbnailSourceStamp(sourcePath, sourceStamp);
    const editor::PreviewLoadAction action = editor::ClassifyPreviewLoad(
        loadedIdentity,
        hasModel,
        failed,
        stamp,
        canonicalIdentity,
        haveStamp,
        sourceStamp);

    if (action == editor::PreviewLoadAction::Keep
        || action == editor::PreviewLoadAction::KeepFailed)
    {
        return;
    }
    if (action == editor::PreviewLoadAction::Clear)
    {
        Clear();
        return;
    }

    ReleaseLoadedModel();
    loadedIdentity = std::string(canonicalIdentity);
    failed = false;
    stamp = sourceStamp;
    if (!haveStamp || !LoadModelFromSource(sourcePath))
    {
        ReleaseLoadedModel();
        failed = true;
    }
}

bool StaticModelPreviewRenderer::Render(
    int width,
    int height,
    const editor::StaticModelPreviewOrbit& orbit)
{
    if (!hasModel || gpu == nullptr)
    {
        return false;
    }
    const editor::PreviewRenderSize size =
        editor::ResolvePreviewRenderSize(static_cast<float>(width), static_cast<float>(height));
    if (!size.valid)
    {
        return false;
    }
    if (!gpu->hasTarget
        || editor::PreviewRenderTargetNeedsResize(
            targetWidth, targetHeight, size.width, size.height))
    {
        ReleaseTarget();
        gpu->target = LoadRenderTexture(size.width, size.height);
        if (gpu->target.id == 0)
        {
            return false;
        }
        gpu->hasTarget = true;
        targetWidth = size.width;
        targetHeight = size.height;
    }

    const editor::ThumbnailCameraFrame frame = editor::MakeStaticModelCameraFrameFromOrbit(orbit);
    if (!std::isfinite(frame.position.x) || !std::isfinite(frame.target.x))
    {
        return false;
    }
    const Camera3D camera = CameraFromFrame(frame);
    BeginTextureMode(gpu->target);
    ClearBackground(kPreviewBackground);
    rlSetClipPlanes(static_cast<double>(frame.nearPlane), static_cast<double>(frame.farPlane));
    BeginMode3D(camera);
    DrawModel(gpu->model, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    EndMode3D();
    EndTextureMode();
    rlSetClipPlanes(RL_CULL_DISTANCE_NEAR, RL_CULL_DISTANCE_FAR);
    return gpu->target.texture.id != 0;
}

unsigned int StaticModelPreviewRenderer::TextureGpuId() const
{
    if (gpu == nullptr || !gpu->hasTarget)
    {
        return 0;
    }
    return gpu->target.texture.id;
}

bool StaticModelPreviewRenderer::HasModel() const
{
    return hasModel;
}

bool StaticModelPreviewRenderer::IsFailed() const
{
    return failed;
}

const std::string& StaticModelPreviewRenderer::LoadedIdentity() const
{
    return loadedIdentity;
}

editor::ThumbnailModelBounds StaticModelPreviewRenderer::Bounds() const
{
    return bounds;
}

editor::ThumbnailSourceStamp StaticModelPreviewRenderer::SourceStamp() const
{
    return stamp;
}
}
