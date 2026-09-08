#include "editor/StaticModelFraming.h"

#include "editor/EditorMath.h"

#include <algorithm>
#include <cmath>

namespace editor
{
namespace
{
constexpr float kMinExtent = 1.0e-4f;
constexpr float kMinRadius = 0.05f;
constexpr core::Vec3 kDefaultViewDirection{1.0f, 0.85f, 1.0f};

core::Vec3 OrbitOffset(float yawDegrees, float pitchDegrees)
{
    const float yaw = yawDegrees * kDegreesToRadians;
    const float pitch = pitchDegrees * kDegreesToRadians;
    const float cosinePitch = std::cos(pitch);
    return NormalizeOr(
        {cosinePitch * std::sin(yaw), std::sin(pitch), cosinePitch * std::cos(yaw)},
        {0.0f, 0.0f, 1.0f});
}

void ClampPreviewOrbit(StaticModelPreviewOrbit& orbit)
{
    orbit.pitchDegrees = std::clamp(
        orbit.pitchDegrees,
        kStaticModelPreviewMinPitchDegrees,
        kStaticModelPreviewMaxPitchDegrees);
    if (!(orbit.fieldOfViewY > 0.0f) || orbit.fieldOfViewY >= 180.0f)
    {
        orbit.fieldOfViewY = kStaticModelPreviewFieldOfViewY;
    }
    const float minDistance = std::max(orbit.paddedRadius * 0.2f, kMinRadius);
    const float maxDistance = std::max(orbit.paddedRadius * 24.0f, minDistance + 0.1f);
    if (!std::isfinite(orbit.distance))
    {
        orbit.distance = StaticModelDefaultFrameDistance(orbit.paddedRadius);
    }
    orbit.distance = std::clamp(orbit.distance, minDistance, maxDistance);
    if (!std::isfinite(orbit.target.x) || !std::isfinite(orbit.target.y)
        || !std::isfinite(orbit.target.z))
    {
        orbit.target = {};
    }
}

void ApplyNearFar(ThumbnailCameraFrame& frame, float distance, float paddedRadius)
{
    frame.nearPlane = distance - paddedRadius * 1.25f;
    if (!(frame.nearPlane > 0.01f) || !std::isfinite(frame.nearPlane))
    {
        frame.nearPlane = 0.05f;
    }
    frame.farPlane = distance + paddedRadius * 3.0f;
    if (!(frame.farPlane > frame.nearPlane + 0.1f) || !std::isfinite(frame.farPlane))
    {
        frame.farPlane = frame.nearPlane + paddedRadius * 8.0f + 10.0f;
    }
}
}

ThumbnailModelBounds SanitizeStaticModelBounds(ThumbnailModelBounds bounds)
{
    if (!(bounds.max.x >= bounds.min.x) || !(bounds.max.y >= bounds.min.y)
        || !(bounds.max.z >= bounds.min.z) || !std::isfinite(bounds.min.x)
        || !std::isfinite(bounds.min.y) || !std::isfinite(bounds.min.z)
        || !std::isfinite(bounds.max.x) || !std::isfinite(bounds.max.y)
        || !std::isfinite(bounds.max.z))
    {
        return {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    }

    core::Vec3 extent{
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z};
    if (extent.x < kMinExtent && extent.y < kMinExtent && extent.z < kMinExtent)
    {
        return {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    }
    return bounds;
}

core::Vec3 StaticModelBoundsCenter(const ThumbnailModelBounds& bounds)
{
    const ThumbnailModelBounds sanitized = SanitizeStaticModelBounds(bounds);
    return {
        (sanitized.min.x + sanitized.max.x) * 0.5f,
        (sanitized.min.y + sanitized.max.y) * 0.5f,
        (sanitized.min.z + sanitized.max.z) * 0.5f};
}

float StaticModelPaddedRadius(const ThumbnailModelBounds& bounds)
{
    const ThumbnailModelBounds sanitized = SanitizeStaticModelBounds(bounds);
    core::Vec3 extent{
        sanitized.max.x - sanitized.min.x,
        sanitized.max.y - sanitized.min.y,
        sanitized.max.z - sanitized.min.z};
    if (extent.x < kMinExtent)
    {
        extent.x = kMinExtent;
    }
    if (extent.y < kMinExtent)
    {
        extent.y = kMinExtent;
    }
    if (extent.z < kMinExtent)
    {
        extent.z = kMinExtent;
    }
    const float radius = Length(Scale(extent, 0.5f));
    return (radius > kMinRadius ? radius : kMinRadius) * kStaticModelPreviewBoundsPadding;
}

float StaticModelDefaultFrameDistance(float paddedRadius)
{
    const float safeRadius = paddedRadius > kMinRadius ? paddedRadius : kMinRadius;
    const float halfFov = kStaticModelPreviewFieldOfViewY * 0.5f * kDegreesToRadians;
    const float tangent = std::tan(halfFov);
    float distance = safeRadius / (tangent > 1.0e-4f ? tangent : 1.0e-4f);
    if (!std::isfinite(distance) || distance < safeRadius * 1.5f)
    {
        distance = safeRadius * 2.5f;
    }
    return distance;
}

ThumbnailCameraFrame MakeDefaultStaticModelCameraFrame(const ThumbnailModelBounds& bounds)
{
    const ThumbnailModelBounds sanitized = SanitizeStaticModelBounds(bounds);
    const core::Vec3 center = StaticModelBoundsCenter(sanitized);
    const float paddedRadius = StaticModelPaddedRadius(sanitized);
    const float distance = StaticModelDefaultFrameDistance(paddedRadius);
    const core::Vec3 direction = NormalizeOr(kDefaultViewDirection, {0.0f, 0.0f, 1.0f});
    ThumbnailCameraFrame frame{};
    frame.target = center;
    frame.position = center + Scale(direction, distance);
    frame.up = {0.0f, 1.0f, 0.0f};
    frame.fieldOfViewY = kStaticModelPreviewFieldOfViewY;
    ApplyNearFar(frame, distance, paddedRadius);
    return frame;
}

void ResetStaticModelPreviewOrbit(
    StaticModelPreviewOrbit& orbit,
    const ThumbnailModelBounds& bounds)
{
    const ThumbnailModelBounds sanitized = SanitizeStaticModelBounds(bounds);
    const core::Vec3 direction = NormalizeOr(kDefaultViewDirection, {0.0f, 0.0f, 1.0f});
    orbit.target = StaticModelBoundsCenter(sanitized);
    orbit.paddedRadius = StaticModelPaddedRadius(sanitized);
    orbit.distance = StaticModelDefaultFrameDistance(orbit.paddedRadius);
    orbit.fieldOfViewY = kStaticModelPreviewFieldOfViewY;
    orbit.pitchDegrees =
        std::asin(std::clamp(direction.y, -1.0f, 1.0f)) * kRadiansToDegrees;
    orbit.yawDegrees = std::atan2(direction.x, direction.z) * kRadiansToDegrees;
    ClampPreviewOrbit(orbit);
}

void ApplyStaticModelPreviewOrbit(
    StaticModelPreviewOrbit& orbit,
    float mouseDeltaX,
    float mouseDeltaY)
{
    if (!std::isfinite(mouseDeltaX) || !std::isfinite(mouseDeltaY))
    {
        return;
    }
    orbit.yawDegrees += mouseDeltaX * kStaticModelPreviewOrbitDegreesPerPixel;
    orbit.pitchDegrees -= mouseDeltaY * kStaticModelPreviewOrbitDegreesPerPixel;
    ClampPreviewOrbit(orbit);
}

bool ApplyStaticModelPreviewDolly(StaticModelPreviewOrbit& orbit, float wheelDelta)
{
    if (wheelDelta == 0.0f || !std::isfinite(wheelDelta))
    {
        return false;
    }
    const float next = orbit.distance * std::pow(0.9f, wheelDelta);
    if (!std::isfinite(next))
    {
        return false;
    }
    orbit.distance = next;
    ClampPreviewOrbit(orbit);
    return true;
}

ThumbnailCameraFrame MakeStaticModelCameraFrameFromOrbit(const StaticModelPreviewOrbit& orbit)
{
    StaticModelPreviewOrbit clamped = orbit;
    ClampPreviewOrbit(clamped);
    const core::Vec3 offset = OrbitOffset(clamped.yawDegrees, clamped.pitchDegrees);
    ThumbnailCameraFrame frame{};
    frame.target = clamped.target;
    frame.position = clamped.target + Scale(offset, clamped.distance);
    frame.up = {0.0f, 1.0f, 0.0f};
    frame.fieldOfViewY = clamped.fieldOfViewY;
    ApplyNearFar(frame, clamped.distance, clamped.paddedRadius);
    return frame;
}

PreviewLoadAction ClassifyPreviewLoad(
    std::string_view loadedIdentity,
    bool hasModel,
    bool failed,
    const ThumbnailSourceStamp& loadedStamp,
    std::string_view requestedIdentity,
    bool haveSourceStamp,
    const ThumbnailSourceStamp& sourceStamp)
{
    if (requestedIdentity.empty())
    {
        return PreviewLoadAction::Clear;
    }
    if (loadedIdentity != requestedIdentity)
    {
        return PreviewLoadAction::Load;
    }
    if (hasModel && haveSourceStamp && SourceStampsEqual(loadedStamp, sourceStamp))
    {
        return PreviewLoadAction::Keep;
    }
    if (failed && haveSourceStamp && SourceStampsEqual(loadedStamp, sourceStamp))
    {
        return PreviewLoadAction::KeepFailed;
    }
    if (failed && !haveSourceStamp)
    {
        return PreviewLoadAction::KeepFailed;
    }
    return PreviewLoadAction::Load;
}

PreviewRenderSize ResolvePreviewRenderSize(float availableWidth, float availableHeight)
{
    PreviewRenderSize size{};
    if (!(availableWidth >= kStaticModelPreviewMinRenderSize)
        || !(availableHeight >= kStaticModelPreviewMinRenderSize)
        || !std::isfinite(availableWidth) || !std::isfinite(availableHeight))
    {
        return size;
    }
    size.width = static_cast<int>(availableWidth);
    size.height = static_cast<int>(availableHeight);
    if (size.width < 1 || size.height < 1)
    {
        return {};
    }
    size.valid = true;
    return size;
}

bool PreviewRenderTargetNeedsResize(
    int currentWidth,
    int currentHeight,
    int desiredWidth,
    int desiredHeight)
{
    return currentWidth != desiredWidth || currentHeight != desiredHeight;
}
}
