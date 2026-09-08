#pragma once

// Shared static-model bounds/framing/orbit math. Used by M48.1 thumbnails
// and M48.2 Model Preview. Not asset authority, not thumbnail cache, and
// not a generic camera framework.

#include "editor/EditorCamera.h"
#include "editor/StaticModelThumbnailCache.h"

#include <string_view>

namespace editor
{
inline constexpr float kStaticModelPreviewFieldOfViewY = kStaticModelThumbnailFieldOfViewY;
inline constexpr float kStaticModelPreviewBoundsPadding = kStaticModelThumbnailBoundsPadding;
inline constexpr float kStaticModelPreviewMinPitchDegrees = kEditorCameraMinPitchDegrees;
inline constexpr float kStaticModelPreviewMaxPitchDegrees = kEditorCameraMaxPitchDegrees;
inline constexpr float kStaticModelPreviewOrbitDegreesPerPixel = kEditorCameraLookDegreesPerPixel;
inline constexpr float kStaticModelPreviewMinRenderSize = 8.0f;

struct StaticModelPreviewOrbit
{
    core::Vec3 target{};
    float yawDegrees = 0.0f;
    float pitchDegrees = 0.0f;
    float distance = 1.0f;
    float fieldOfViewY = kStaticModelPreviewFieldOfViewY;
    float paddedRadius = 1.0f;
};

struct PreviewRenderSize
{
    int width = 0;
    int height = 0;
    bool valid = false;
};

enum class PreviewLoadAction
{
    Keep,
    KeepFailed,
    Load,
    Clear,
};

ThumbnailModelBounds SanitizeStaticModelBounds(ThumbnailModelBounds bounds);
core::Vec3 StaticModelBoundsCenter(const ThumbnailModelBounds& bounds);
float StaticModelPaddedRadius(const ThumbnailModelBounds& bounds);
float StaticModelDefaultFrameDistance(float paddedRadius);

ThumbnailCameraFrame MakeDefaultStaticModelCameraFrame(const ThumbnailModelBounds& bounds);

void ResetStaticModelPreviewOrbit(
    StaticModelPreviewOrbit& orbit,
    const ThumbnailModelBounds& bounds);
void ApplyStaticModelPreviewOrbit(
    StaticModelPreviewOrbit& orbit,
    float mouseDeltaX,
    float mouseDeltaY);
bool ApplyStaticModelPreviewDolly(StaticModelPreviewOrbit& orbit, float wheelDelta);
ThumbnailCameraFrame MakeStaticModelCameraFrameFromOrbit(const StaticModelPreviewOrbit& orbit);

PreviewLoadAction ClassifyPreviewLoad(
    std::string_view loadedIdentity,
    bool hasModel,
    bool failed,
    const ThumbnailSourceStamp& loadedStamp,
    std::string_view requestedIdentity,
    bool haveSourceStamp,
    const ThumbnailSourceStamp& sourceStamp);

PreviewRenderSize ResolvePreviewRenderSize(float availableWidth, float availableHeight);
bool PreviewRenderTargetNeedsResize(
    int currentWidth,
    int currentHeight,
    int desiredWidth,
    int desiredHeight);
}
