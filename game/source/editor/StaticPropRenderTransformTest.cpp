// Milestone 49 Correction 2: Static Prop transform/render-contract regressions.
//
// The renderer composes a prop as rlTranslate * rlRotateZ * rlRotateY *
// rlRotateX * rlScale and then draws the shared model at the local origin, so
// the authored spec is the only per-instance state. These tests pin that
// contract at the math boundary: raw model dimensions survive Scale (1,1,1),
// instances sharing one identity stay independent, repeated evaluation never
// accumulates, and picking agrees with the render transform. They also pin the
// staged-runtime diagnostic policy (no canonical-source fallback).

#include "editor/StaticPropTransform.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
int gFailures = 0;

void Expect(bool condition, const char* what)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++gFailures;
    }
}

bool NearlyEqual(float a, float b, float tolerance = 1.0e-4f)
{
    return std::fabs(a - b) <= tolerance;
}

bool Vec3Near(core::Vec3 a, core::Vec3 b, float tolerance = 1.0e-4f)
{
    return NearlyEqual(a.x, b.x, tolerance) && NearlyEqual(a.y, b.y, tolerance)
        && NearlyEqual(a.z, b.z, tolerance);
}

bool Vec3Identical(core::Vec3 a, core::Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

// Raw GLB bounds measured from the staged runtime models used during the
// Correction 2 reproduction. They are fixtures for the "no normalization"
// contract, not an assertion about any particular asset on disk.
struct RawModelBounds
{
    const char* identity;
    core::Vec3 localMin;
    core::Vec3 localMax;
};

constexpr RawModelBounds kBarrel{
    "models/Barrel by HFJAKI92 - wrYrHLVtxg.glb",
    {-1.7175f, -0.6327f, -1.7372f},
    {1.7090f, 2.3821f, 1.7421f}};
constexpr RawModelBounds kChest{
    "models/Chest by Quaternius - O72u4Drp8k.glb",
    {-0.590267f, -0.001326f, -0.390449f},
    {0.589050f, 0.897170f, 0.433321f}};
constexpr RawModelBounds kTestStatic{
    "models/test_static.glb", {-0.55f, -0.6f, -0.55f}, {0.55f, 0.6f, 0.55f}};
constexpr RawModelBounds kTestAuthored{
    "models/test_authored.glb", {-1.0f, 0.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
constexpr RawModelBounds kTestTextured{
    "models/test_textured.glb", {-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};

world::StaticPropSpec MakeProp(
    const char* identity,
    core::Vec3 position,
    core::Vec3 rotationDegrees,
    core::Vec3 scale)
{
    world::StaticPropSpec prop{};
    prop.modelIdentity = identity;
    prop.position = position;
    prop.rotationDegrees = rotationDegrees;
    prop.scale = scale;
    return prop;
}
}

int main()
{
    // Scale (1,1,1) preserves raw GLB dimensions. Import/Cook/Stage must never
    // normalize a model into a nominal unit size.
    {
        const RawModelBounds models[] = {kBarrel, kTestStatic, kTestAuthored, kTestTextured};
        for (const RawModelBounds& model : models)
        {
            const world::StaticPropSpec prop = MakeProp(
                model.identity, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
            core::Vec3 center{};
            core::Vec3 size{};
            editor::StaticPropWorldAabb(prop, model.localMin, model.localMax, center, size);
            const core::Vec3 raw{
                model.localMax.x - model.localMin.x,
                model.localMax.y - model.localMin.y,
                model.localMax.z - model.localMin.z};
            Expect(Vec3Near(size, raw), "Scale (1,1,1) preserves raw model dimensions");
            const core::Vec3 rawCenter{
                (model.localMin.x + model.localMax.x) * 0.5f,
                (model.localMin.y + model.localMax.y) * 0.5f,
                (model.localMin.z + model.localMax.z) * 0.5f};
            Expect(Vec3Near(center, rawCenter), "Scale (1,1,1) preserves raw model center");
        }
    }

    // Authored Scale is a visual multiplier on the model, never a primitive
    // Resize: it stretches the same raw geometry and leaves position alone.
    {
        const world::StaticPropSpec unit = MakeProp(
            kTestTextured.identity, {3.0f, 2.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        const world::StaticPropSpec scaled = MakeProp(
            kTestTextured.identity, {3.0f, 2.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {2.0f, 0.5f, 3.0f});
        core::Vec3 unitCenter{};
        core::Vec3 unitSize{};
        core::Vec3 scaledCenter{};
        core::Vec3 scaledSize{};
        editor::StaticPropWorldAabb(
            unit, kTestTextured.localMin, kTestTextured.localMax, unitCenter, unitSize);
        editor::StaticPropWorldAabb(
            scaled, kTestTextured.localMin, kTestTextured.localMax, scaledCenter, scaledSize);
        Expect(
            NearlyEqual(scaledSize.x, unitSize.x * 2.0f)
                && NearlyEqual(scaledSize.y, unitSize.y * 0.5f)
                && NearlyEqual(scaledSize.z, unitSize.z * 3.0f),
            "authored Scale multiplies the model bounds per axis");
        Expect(
            Vec3Near(scaledCenter, unitCenter),
            "authored Scale about an origin-centered model keeps the world center");
        Expect(scaled.position.x == 3.0f, "authored Scale does not move authored position");
    }

    // Renderer composition order: translate, then Euler Z*Y*X, then scale.
    {
        const world::StaticPropSpec prop = MakeProp(
            kTestStatic.identity, {5.0f, 1.0f, -2.0f}, {10.0f, 20.0f, 30.0f}, {2.0f, 3.0f, 4.0f});
        const core::Vec3 local{0.5f, -0.25f, 0.75f};
        const core::Vec3 expected = prop.position
            + editor::RotateEulerXYZ(
                  editor::ScaleAxes(local, prop.scale), prop.rotationDegrees);
        Expect(
            Vec3Near(editor::StaticPropWorldFromLocal(prop, local), expected),
            "world transform is position + RotZ*RotY*RotX * (scale * local)");
        const core::Vec3 roundTrip = editor::InverseScaleAxes(
            editor::InverseRotateEulerXYZ(
                editor::Sub(expected, prop.position), prop.rotationDegrees),
            prop.scale);
        Expect(Vec3Near(roundTrip, local), "inverse transform recovers the local point");
    }

    // Two instances of one identity are independent. The shared model resource
    // is keyed by identity only; nothing per-instance may ride along with it.
    {
        world::StaticPropSpec first = MakeProp(
            kBarrel.identity, {-4.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.25f, 0.25f, 0.25f});
        world::StaticPropSpec second = MakeProp(
            kBarrel.identity, {4.0f, 1.0f, 0.0f}, {0.0f, 45.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
        const world::StaticPropSpec firstBefore = first;
        const world::StaticPropSpec secondBefore = second;
        core::Vec3 firstCenter{};
        core::Vec3 firstSize{};
        core::Vec3 secondCenter{};
        core::Vec3 secondSize{};
        editor::StaticPropWorldAabb(first, kBarrel.localMin, kBarrel.localMax, firstCenter, firstSize);
        editor::StaticPropWorldAabb(
            second, kBarrel.localMin, kBarrel.localMax, secondCenter, secondSize);
        Expect(
            !Vec3Near(firstSize, secondSize) && !Vec3Near(firstCenter, secondCenter),
            "two instances of one identity keep independent transforms");
        Expect(
            NearlyEqual(secondSize.y / firstSize.y, 8.0f, 1.0e-3f),
            "each instance uses only its own Scale");
        Expect(
            Vec3Identical(first.position, firstBefore.position)
                && Vec3Identical(first.scale, firstBefore.scale)
                && Vec3Identical(first.rotationDegrees, firstBefore.rotationDegrees)
                && Vec3Identical(second.scale, secondBefore.scale),
            "evaluating one instance does not mutate any authored spec");
    }

    // Different identities cannot contaminate one another: the transform is a
    // pure function of the spec, so swapping the identity string alone cannot
    // change geometry, and equal specs under different identities agree.
    {
        const world::StaticPropSpec barrel = MakeProp(
            kBarrel.identity, {1.0f, 2.0f, 3.0f}, {5.0f, 15.0f, 25.0f}, {1.5f, 1.5f, 1.5f});
        const world::StaticPropSpec textured = MakeProp(
            kTestTextured.identity, {1.0f, 2.0f, 3.0f}, {5.0f, 15.0f, 25.0f}, {1.5f, 1.5f, 1.5f});
        const core::Vec3 local{0.3f, -0.4f, 0.5f};
        Expect(
            Vec3Identical(
                editor::StaticPropWorldFromLocal(barrel, local),
                editor::StaticPropWorldFromLocal(textured, local)),
            "identity string never participates in the render transform");
        core::Vec3 barrelCenter{};
        core::Vec3 barrelSize{};
        core::Vec3 texturedCenter{};
        core::Vec3 texturedSize{};
        editor::StaticPropWorldAabb(
            barrel, kBarrel.localMin, kBarrel.localMax, barrelCenter, barrelSize);
        editor::StaticPropWorldAabb(
            textured, kTestTextured.localMin, kTestTextured.localMax, texturedCenter, texturedSize);
        Expect(
            !Vec3Near(barrelSize, texturedSize),
            "different identities keep their own raw model bounds");
    }

    // No frame-to-frame accumulation: the draw path reads the spec and pushes
    // and pops its own matrix, so repeated evaluation must be bit-identical.
    {
        world::StaticPropSpec prop = MakeProp(
            kBarrel.identity, {2.0f, 3.5f, 0.0f}, {0.0f, 30.0f, 0.0f}, {0.5f, 0.5f, 0.5f});
        core::Vec3 firstCenter{};
        core::Vec3 firstSize{};
        editor::StaticPropWorldAabb(prop, kBarrel.localMin, kBarrel.localMax, firstCenter, firstSize);
        for (int frame = 0; frame < 600; ++frame)
        {
            core::Vec3 center{};
            core::Vec3 size{};
            editor::StaticPropWorldAabb(prop, kBarrel.localMin, kBarrel.localMax, center, size);
            if (!Vec3Identical(center, firstCenter) || !Vec3Identical(size, firstSize))
            {
                Expect(false, "repeated frames must not drift the rendered transform");
                break;
            }
        }
        Expect(
            prop.scale.x == 0.5f && prop.rotationDegrees.y == 30.0f,
            "600 frames leave authored Scale and Rotation untouched");
    }

    // Picking agrees with the render transform, including scale and rotation.
    {
        const world::StaticPropSpec prop = MakeProp(
            kTestTextured.identity, {6.0f, 2.0f, 0.0f}, {0.0f, 45.0f, 0.0f}, {2.0f, 2.0f, 2.0f});
        const editor::Ray3 hitRay{{6.0f, 2.0f, 20.0f}, {0.0f, 0.0f, -1.0f}};
        const editor::RayHit hit = editor::IntersectRayStaticProp(
            hitRay, prop, kTestTextured.localMin, kTestTextured.localMax);
        Expect(hit.hit, "ray at the authored position hits the scaled prop");
        // Yawed 2x cube of raw half-extent 1: the near face along -Z sits at
        // 2 * sqrt(2) from the center.
        Expect(
            NearlyEqual(hit.distance, 20.0f - (2.0f * 1.41421356f), 1.0e-2f),
            "pick distance matches the scaled and rotated render bounds");
        const editor::Ray3 missRay{{6.0f, 12.0f, 20.0f}, {0.0f, 0.0f, -1.0f}};
        Expect(
            !editor::IntersectRayStaticProp(
                 missRay, prop, kTestTextured.localMin, kTestTextured.localMax)
                 .hit,
            "ray outside the prop bounds misses");
        world::StaticPropSpec invalid = prop;
        invalid.scale = {0.0f, 1.0f, 1.0f};
        Expect(
            !editor::IntersectRayStaticProp(
                 hitRay, invalid, kTestTextured.localMin, kTestTextured.localMax)
                 .hit,
            "invalid authored scale is not pickable");
    }

    // Staged runtime assets are the only scene-render authority. The
    // diagnostic explains the placeholder cube and names Cook & Stage; it must
    // not suggest a canonical-source fallback.
    {
        Expect(
            editor::ClassifyStaticPropAsset(kBarrel.identity, true)
                == editor::StaticPropAssetState::Ok,
            "staged model present classifies as Ok");
        Expect(
            editor::StaticPropAssetStateMessage(editor::StaticPropAssetState::Ok) == nullptr,
            "Ok has no diagnostic line");
        Expect(
            editor::ClassifyStaticPropAsset(kBarrel.identity, false)
                == editor::StaticPropAssetState::MissingStagedRuntimeModel,
            "valid identity without a staged file reports the staged gap");
        Expect(
            editor::ClassifyStaticPropAsset("barrel.glb", true)
                == editor::StaticPropAssetState::InvalidIdentity,
            "non-canonical identity is reported before the staged check");
        Expect(
            editor::ClassifyStaticPropAsset("", true)
                == editor::StaticPropAssetState::InvalidIdentity,
            "empty identity is invalid");
        const char* missing =
            editor::StaticPropAssetStateMessage(
                editor::StaticPropAssetState::MissingStagedRuntimeModel);
        Expect(missing != nullptr, "missing staged model has an actionable message");
        const std::string missingText = missing != nullptr ? missing : "";
        Expect(
            missingText.find("Cook & Stage") != std::string::npos,
            "missing staged message names the Cook & Stage workflow");
        Expect(
            missingText.find("staged runtime assets") != std::string::npos,
            "missing staged message names the staged runtime authority");
        Expect(
            missingText.find("source") == std::string::npos,
            "missing staged message never offers a canonical-source fallback");
    }

    // Correction 3: Chest vs Barrel loaded AABBs (raylib GetModelBoundingBox after
    // baking node transforms). Scale (1,1,1) keeps those dimensions. The gameplay
    // chase camera is not inside a default-add Chest, and the look ray to spawn
    // does not immediately hit that Chest.
    {
        constexpr core::Vec3 kGameplayCameraPos{2.0f, 4.3f, 12.0f};
        constexpr core::Vec3 kGameplayCameraTarget{0.0f, 0.8f, 0.0f};
        constexpr core::Vec3 kDefaultAdd{0.42f, 1.54f, 0.0f};
        const editor::Ray3 gameplayLook{
            kGameplayCameraPos,
            editor::NormalizeOr(
                editor::Sub(kGameplayCameraTarget, kGameplayCameraPos),
                {0.0f, 0.0f, -1.0f})};

        const world::StaticPropSpec chest = MakeProp(
            kChest.identity,
            kDefaultAdd,
            {0.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 1.0f});
        core::Vec3 chestCenter{};
        core::Vec3 chestSize{};
        editor::StaticPropWorldAabb(chest, kChest.localMin, kChest.localMax, chestCenter, chestSize);
        Expect(chestSize.x < 1.3f && chestSize.y < 1.1f && chestSize.z < 1.0f,
            "Chest loaded size at Scale (1,1,1) is about one world unit, not 100x");
        Expect(chestSize.x > 0.5f && chestSize.y > 0.4f,
            "Chest loaded size is not the raw centimetre accessor");
        Expect(
            !editor::StaticPropContainsWorldPoint(
                chest, kChest.localMin, kChest.localMax, kGameplayCameraPos),
            "default-add Chest does not contain the gameplay camera");
        Expect(
            !editor::StaticPropContainsWorldPoint(
                chest, kChest.localMin, kChest.localMax, kGameplayCameraTarget),
            "default-add Chest does not contain the gameplay camera target");
        const editor::RayHit chestLook = editor::IntersectRayStaticProp(
            gameplayLook, chest, kChest.localMin, kChest.localMax);
        Expect(!chestLook.hit, "gameplay look ray does not immediately hit default-add Chest");

        world::StaticPropSpec atCamera = chest;
        atCamera.position = kGameplayCameraPos;
        Expect(
            editor::StaticPropContainsWorldPoint(
                atCamera, kChest.localMin, kChest.localMax, kGameplayCameraPos),
            "a Chest placed on the camera does contain the camera point");

        const world::StaticPropSpec barrel = MakeProp(
            kBarrel.identity,
            kDefaultAdd,
            {0.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 1.0f});
        core::Vec3 barrelCenter{};
        core::Vec3 barrelSize{};
        editor::StaticPropWorldAabb(
            barrel, kBarrel.localMin, kBarrel.localMax, barrelCenter, barrelSize);
        Expect(barrelSize.x > chestSize.x && barrelSize.y > chestSize.y,
            "Barrel loaded size is larger than Chest");
        Expect(
            !editor::StaticPropContainsWorldPoint(
                barrel, kBarrel.localMin, kBarrel.localMax, kGameplayCameraPos),
            "default-add Barrel does not contain the gameplay camera");
        const editor::RayHit barrelLook = editor::IntersectRayStaticProp(
            gameplayLook, barrel, kBarrel.localMin, kBarrel.localMax);
        Expect(
            !barrelLook.hit || barrelLook.distance > 8.0f,
            "default-add Barrel does not sit on the gameplay near plane");
    }

    if (gFailures != 0)
    {
        std::fprintf(stderr, "%d Static Prop render transform test(s) failed.\n", gFailures);
        return 1;
    }

    std::printf("Static Prop render transform tests passed.\n");
    return 0;
}
