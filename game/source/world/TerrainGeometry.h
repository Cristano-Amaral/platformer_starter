#pragma once

// Deterministic CPU heightfield tessellation shared by render, Jolt MeshShape,
// and editor picking. One cell = two triangles. Winding is CCW from +Y.

#include "core/Vec3.h"
#include "world/Terrain.h"

#include <cmath>
#include <cstddef>
#include <vector>

namespace world
{
struct TerrainTriangle
{
    int i0 = 0;
    int i1 = 0;
    int i2 = 0;
};

struct TerrainGeometry
{
    std::vector<core::Vec3> positions{};
    std::vector<core::Vec3> normals{};
    std::vector<TerrainTriangle> triangles{};
};

inline int TerrainVertexCount(int resolutionX, int resolutionZ)
{
    if (!TerrainResolutionIsValid(resolutionX) || !TerrainResolutionIsValid(resolutionZ))
    {
        return 0;
    }
    return resolutionX * resolutionZ;
}

inline int TerrainCellCount(int resolutionX, int resolutionZ)
{
    if (!TerrainResolutionIsValid(resolutionX) || !TerrainResolutionIsValid(resolutionZ))
    {
        return 0;
    }
    return (resolutionX - 1) * (resolutionZ - 1);
}

inline int TerrainTriangleCount(int resolutionX, int resolutionZ)
{
    return TerrainCellCount(resolutionX, resolutionZ) * 2;
}

inline int TerrainIndexCount(int resolutionX, int resolutionZ)
{
    return TerrainTriangleCount(resolutionX, resolutionZ) * 3;
}

inline core::Vec3 TerrainCross(core::Vec3 a, core::Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x};
}

inline core::Vec3 TerrainSub(core::Vec3 a, core::Vec3 b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline float TerrainDot(core::Vec3 a, core::Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline core::Vec3 TerrainNormalizeOrUp(core::Vec3 value)
{
    const float lengthSq = TerrainDot(value, value);
    if (!(lengthSq > 1.0e-20f) || !std::isfinite(lengthSq))
    {
        return {0.0f, 1.0f, 0.0f};
    }
    const float invLength = 1.0f / std::sqrt(lengthSq);
    return {value.x * invLength, value.y * invLength, value.z * invLength};
}

inline bool GenerateTerrainGeometry(const TerrainSpec& terrain, TerrainGeometry& out)
{
    out = {};
    if (!TerrainSpecIsValid(terrain))
    {
        return false;
    }

    const int vertexCount = TerrainVertexCount(terrain.resolutionX, terrain.resolutionZ);
    const int triangleCount = TerrainTriangleCount(terrain.resolutionX, terrain.resolutionZ);
    out.positions.resize(static_cast<std::size_t>(vertexCount));
    out.normals.assign(static_cast<std::size_t>(vertexCount), {0.0f, 0.0f, 0.0f});
    out.triangles.resize(static_cast<std::size_t>(triangleCount));

    for (int iz = 0; iz < terrain.resolutionZ; ++iz)
    {
        for (int ix = 0; ix < terrain.resolutionX; ++ix)
        {
            out.positions[static_cast<std::size_t>(TerrainHeightIndex(terrain, ix, iz))] =
                TerrainSamplePosition(terrain, ix, iz);
        }
    }

    int triangleIndex = 0;
    for (int iz = 0; iz < terrain.resolutionZ - 1; ++iz)
    {
        for (int ix = 0; ix < terrain.resolutionX - 1; ++ix)
        {
            const int i00 = TerrainHeightIndex(terrain, ix, iz);
            const int i10 = TerrainHeightIndex(terrain, ix + 1, iz);
            const int i01 = TerrainHeightIndex(terrain, ix, iz + 1);
            const int i11 = TerrainHeightIndex(terrain, ix + 1, iz + 1);

            // CCW from +Y: (0,0) -> (0,1) -> (1,0) and (1,0) -> (0,1) -> (1,1).
            out.triangles[static_cast<std::size_t>(triangleIndex)] = {i00, i01, i10};
            out.triangles[static_cast<std::size_t>(triangleIndex + 1)] = {i10, i01, i11};
            triangleIndex += 2;
        }
    }

    for (const TerrainTriangle& triangle : out.triangles)
    {
        const core::Vec3 a = out.positions[static_cast<std::size_t>(triangle.i0)];
        const core::Vec3 b = out.positions[static_cast<std::size_t>(triangle.i1)];
        const core::Vec3 c = out.positions[static_cast<std::size_t>(triangle.i2)];
        const core::Vec3 normal = TerrainCross(TerrainSub(b, a), TerrainSub(c, a));
        out.normals[static_cast<std::size_t>(triangle.i0)].x += normal.x;
        out.normals[static_cast<std::size_t>(triangle.i0)].y += normal.y;
        out.normals[static_cast<std::size_t>(triangle.i0)].z += normal.z;
        out.normals[static_cast<std::size_t>(triangle.i1)].x += normal.x;
        out.normals[static_cast<std::size_t>(triangle.i1)].y += normal.y;
        out.normals[static_cast<std::size_t>(triangle.i1)].z += normal.z;
        out.normals[static_cast<std::size_t>(triangle.i2)].x += normal.x;
        out.normals[static_cast<std::size_t>(triangle.i2)].y += normal.y;
        out.normals[static_cast<std::size_t>(triangle.i2)].z += normal.z;
    }

    for (core::Vec3& normal : out.normals)
    {
        normal = TerrainNormalizeOrUp(normal);
        if (!TerrainVecFinite(normal))
        {
            return false;
        }
    }
    for (const core::Vec3& position : out.positions)
    {
        if (!TerrainVecFinite(position))
        {
            return false;
        }
    }
    return true;
}

inline bool TerrainGeometryIsFinite(const TerrainGeometry& geometry)
{
    for (const core::Vec3& position : geometry.positions)
    {
        if (!TerrainVecFinite(position))
        {
            return false;
        }
    }
    for (const core::Vec3& normal : geometry.normals)
    {
        if (!TerrainVecFinite(normal))
        {
            return false;
        }
    }
    return true;
}

inline void TerrainWorldAabb(const TerrainSpec& terrain, core::Vec3& center, core::Vec3& size)
{
    center = {};
    size = {};
    TerrainGeometry geometry{};
    if (!GenerateTerrainGeometry(terrain, geometry) || geometry.positions.empty())
    {
        return;
    }

    core::Vec3 minimum = geometry.positions.front();
    core::Vec3 maximum = geometry.positions.front();
    for (const core::Vec3& position : geometry.positions)
    {
        minimum.x = position.x < minimum.x ? position.x : minimum.x;
        minimum.y = position.y < minimum.y ? position.y : minimum.y;
        minimum.z = position.z < minimum.z ? position.z : minimum.z;
        maximum.x = position.x > maximum.x ? position.x : maximum.x;
        maximum.y = position.y > maximum.y ? position.y : maximum.y;
        maximum.z = position.z > maximum.z ? position.z : maximum.z;
    }
    center = {
        (minimum.x + maximum.x) * 0.5f,
        (minimum.y + maximum.y) * 0.5f,
        (minimum.z + maximum.z) * 0.5f};
    size = {
        maximum.x - minimum.x,
        maximum.y - minimum.y,
        maximum.z - minimum.z};
    if (!(size.y > 0.0f))
    {
        size.y = 0.02f;
    }
}

inline bool IntersectRayTriangle(
    core::Vec3 origin,
    core::Vec3 direction,
    core::Vec3 a,
    core::Vec3 b,
    core::Vec3 c,
    float& outDistance)
{
    const core::Vec3 edge1 = TerrainSub(b, a);
    const core::Vec3 edge2 = TerrainSub(c, a);
    const core::Vec3 pvec = TerrainCross(direction, edge2);
    const float det = TerrainDot(edge1, pvec);
    if (!(std::fabs(det) > 1.0e-8f))
    {
        return false;
    }
    const float invDet = 1.0f / det;
    const core::Vec3 tvec = TerrainSub(origin, a);
    const float u = TerrainDot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }
    const core::Vec3 qvec = TerrainCross(tvec, edge1);
    const float v = TerrainDot(direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }
    const float t = TerrainDot(edge2, qvec) * invDet;
    if (!(t > 0.0f) || !std::isfinite(t))
    {
        return false;
    }
    outDistance = t;
    return true;
}

inline bool IntersectRayTerrain(
    const TerrainSpec& terrain,
    core::Vec3 origin,
    core::Vec3 direction,
    float& outDistance)
{
    TerrainGeometry geometry{};
    if (!GenerateTerrainGeometry(terrain, geometry))
    {
        return false;
    }

    bool hit = false;
    float best = 0.0f;
    for (const TerrainTriangle& triangle : geometry.triangles)
    {
        float distance = 0.0f;
        if (!IntersectRayTriangle(
                origin,
                direction,
                geometry.positions[static_cast<std::size_t>(triangle.i0)],
                geometry.positions[static_cast<std::size_t>(triangle.i1)],
                geometry.positions[static_cast<std::size_t>(triangle.i2)],
                distance))
        {
            continue;
        }
        if (!hit || distance < best)
        {
            hit = true;
            best = distance;
        }
    }
    if (hit)
    {
        outDistance = best;
    }
    return hit;
}
}
