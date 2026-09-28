#pragma once

#include "world/CharacterPlacement.h"
#include <cmath>

namespace gameplay
{
enum class CharacterPatrolLocomotionState { Idle, Move };

// Shared M112/M113 bounded horizontal patrol calculation, not an actor framework.
struct CharacterPatrolState
{
    world::CharacterPlacementSpec origin;
    core::Vec3 position{};
    core::Vec3 rotationDegrees{};
    core::Vec3 axis{1, 0, 0};
    core::Vec3 endpointMin{};
    core::Vec3 endpointMax{};
    CharacterPatrolLocomotionState locomotion = CharacterPatrolLocomotionState::Idle;
    int direction = 1;
    double phase = 0.0;
    const char* diagnostic = "Idle at authored origin";

    explicit CharacterPatrolState(const world::CharacterPlacementSpec& placement)
        : origin(placement), position(placement.position), rotationDegrees(placement.rotationDegrees)
    {
        constexpr double kRadians = 3.14159265358979323846 / 180.0;
        // Matches CharacterInstance's Rz * Ry * Rx model transform.
        const double y = std::remainder(double(origin.rotationDegrees.y), 360.0) * kRadians;
        const double z = std::remainder(double(origin.rotationDegrees.z), 360.0) * kRadians;
        double xAxis = std::cos(z) * std::cos(y);
        double zAxis = -std::sin(y);
        const double length = std::hypot(xAxis, zAxis);
        if (length > 1.0e-6) { xAxis /= length; zAxis /= length; }
        else { xAxis = std::cos(y); zAxis = -std::sin(y); }
        axis = {float(xAxis), 0, float(zAxis)};
        endpointMin = {origin.position.x - axis.x * origin.patrolDistance, origin.position.y,
            origin.position.z - axis.z * origin.patrolDistance};
        endpointMax = {origin.position.x + axis.x * origin.patrolDistance, origin.position.y,
            origin.position.z + axis.z * origin.patrolDistance};
        // Phase starts midway along the outbound leg: spawn is always origin.
        phase = origin.patrolDistance;
        Advance(0.0f);
    }

    void Advance(float deltaSeconds)
    {
        if (!origin.patrolEnabled) return;
        const double distance = origin.patrolDistance;
        const double period = 4.0 * distance;
        if (std::isfinite(deltaSeconds) && deltaSeconds > 0.0f)
            phase = std::fmod(phase + double(deltaSeconds) * origin.patrolSpeed, period);
        direction = phase < 2.0 * distance ? 1 : -1;
        const double offset = direction > 0 ? phase - distance : 3.0 * distance - phase;
        position = {origin.position.x + axis.x * float(offset), origin.position.y,
            origin.position.z + axis.z * float(offset)};
        // Model forward is +Z. Horizontal patrol uses a pure yaw; authored TRS
        // remains untouched, including pitch/roll, for disabled patrol and Save.
        rotationDegrees = {0, float(std::atan2(axis.x * direction, axis.z * direction)
            * (180.0 / 3.14159265358979323846)), 0};
        locomotion = CharacterPatrolLocomotionState::Move;
        diagnostic = "Horizontal patrol (no collision or root motion)";
    }
};
}
