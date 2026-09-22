#include "world/LevelWriter.h"
#include "world/TerrainVegetation.h"

#include "assets/RuntimePng.h"
#include "platform/FileReplace.h"
#include "world/LevelFile.h"

#include <array>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <system_error>

namespace world
{
namespace
{
// std::to_chars shortest round-trip: locale-independent, and from_chars
// recovers the exact same float. Non-finite values never reach here because
// IsWritableLevelDefinition rejects them first.
void AppendFloat(std::string& out, float value)
{
    std::array<char, 64> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{})
    {
        out += '0';
        return;
    }
    out.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

void AppendInt(std::string& out, int value)
{
    std::array<char, 32> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (result.ec != std::errc{})
    {
        out += '0';
        return;
    }
    out.append(buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data()));
}

void AppendVec3(std::string& out, core::Vec3 value)
{
    AppendFloat(out, value.x);
    out += ' ';
    AppendFloat(out, value.y);
    out += ' ';
    AppendFloat(out, value.z);
}

void AppendBoxRecord(std::string& out, std::string_view keyword, const Box& box)
{
    out.append(keyword);
    out += ' ';
    AppendVec3(out, box.center);
    out += ' ';
    AppendVec3(out, box.size);
    out += '\n';
}

void AppendCenterSizeRecord(
    std::string& out,
    std::string_view keyword,
    core::Vec3 center,
    core::Vec3 size)
{
    out.append(keyword);
    out += ' ';
    AppendVec3(out, center);
    out += ' ';
    AppendVec3(out, size);
    out += '\n';
}

void AppendIntRecord(std::string& out, std::string_view keyword, int value)
{
    out.append(keyword);
    out += ' ';
    AppendInt(out, value);
    out += '\n';
}

WriteLevelFileResult MakeWriteResult(WriteLevelFileStatus status, const char* error)
{
    WriteLevelFileResult result{};
    result.status = status;
    result.error = error;
    return result;
}

void BestEffortRemove(const std::filesystem::path& path)
{
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}
}

bool IsWritableLevelDefinition(const LevelDefinition& level)
{
    return IsValidLevelIdToken(level.id) && LevelDefinitionHasRequiredAuthoredContent(level);
}

std::string SerializeLevelText(const LevelDefinition& level)
{
    if (!IsWritableLevelDefinition(level))
    {
        return {};
    }

    // Canonical v1 record order. Documented in docs/LEVEL_FORMAT_V1.md.
    // The parser accepts any order; the writer emits exactly one.
    std::string out;
    out.reserve(1024);

    out.append(kLevelFileMagic);
    out += ' ';
    AppendInt(out, kLevelFileVersion);
    out += '\n';

    out += "id ";
    out.append(level.id);
    out += '\n';

    out += "spawn ";
    AppendVec3(out, level.initialSpawnVisualCenter);
    out += '\n';

    out += "kill_plane ";
    AppendFloat(out, level.killPlaneY);
    out += '\n';

    AppendBoxRecord(out, "ground", level.ground);

    for (const Box& platform : level.elevatedPlatforms)
    {
        AppendBoxRecord(out, "platform", platform);
    }

    AppendIntRecord(out, "support_index_cp1", level.checkpoint1PlatformIndex);
    AppendIntRecord(out, "support_index_cp2", level.checkpoint2PlatformIndex);
    AppendIntRecord(out, "support_index_goal", level.goalPlatformIndex);

    for (const SlopeSpec& slope : level.slopes)
    {
        out += "slope ";
        AppendVec3(out, slope.center);
        out += ' ';
        AppendVec3(out, slope.size);
        out += ' ';
        AppendFloat(out, slope.rotationZDegrees);
        out += '\n';
    }

    out += "moving_platform ";
    AppendVec3(out, level.movingPlatform.size);
    out += ' ';
    AppendFloat(out, level.movingPlatform.centerY);
    out += ' ';
    AppendFloat(out, level.movingPlatform.centerZ);
    out += ' ';
    AppendFloat(out, level.movingPlatform.pathMinX);
    out += ' ';
    AppendFloat(out, level.movingPlatform.pathMaxX);
    out += ' ';
    AppendFloat(out, level.movingPlatform.speed);
    out += ' ';
    AppendFloat(out, level.movingPlatform.startX);
    out += '\n';

    for (const CheckpointSpec& checkpoint : level.checkpoints)
    {
        out += "checkpoint ";
        AppendVec3(out, checkpoint.center);
        out += ' ';
        AppendVec3(out, checkpoint.size);
        out += ' ';
        AppendVec3(out, checkpoint.respawnPosition);
        out += '\n';
    }

    for (const HazardSpec& hazard : level.hazards)
    {
        AppendCenterSizeRecord(out, "hazard", hazard.center, hazard.size);
    }

    for (const CollectibleSpec& collectible : level.collectibles)
    {
        AppendCenterSizeRecord(out, "collectible", collectible.center, collectible.size);
    }

    for (const LevelGoalSpec& goal : level.levelGoals)
    {
        out += "level_goal ";
        AppendVec3(out, goal.center);
        out += ' ';
        AppendVec3(out, goal.size);
        if (!goal.nextLevelId.empty())
        {
            out += ' ';
            out += goal.nextLevelId;
        }
        out += '\n';
    }

    for (const DynamicBoxSpec& box : level.dynamicBoxes)
    {
        out += "dynamic_box ";
        AppendVec3(out, box.center);
        out += ' ';
        AppendVec3(out, box.size);
        out += ' ';
        AppendFloat(out, box.massKg);
        out += '\n';
    }

    for (const PressurePlateSpec& plate : level.pressurePlates)
    {
        out += "pressure_plate ";
        AppendVec3(out, plate.center);
        out += ' ';
        AppendVec3(out, plate.size);
        out += ' ';
        AppendInt(out, plate.linkedDoorIndex);
        out += ' ';
        AppendInt(out, plate.activateByDynamicBox ? 1 : 0);
        out += ' ';
        AppendInt(out, plate.activateByPlayer ? 1 : 0);
        out += ' ';
        AppendInt(out, plate.visibleInGameplay ? 1 : 0);
        out += ' ';
        AppendInt(out, plate.controlsDirectionalLight ? 1 : 0);
        if (!plate.controlledLocalLights.empty())
        {
            out += ' ';
            out += kPressurePlateLocalLightsMarker;
            out += ' ';
            AppendInt(out, static_cast<int>(plate.controlledLocalLights.size()));
            for (const LocalLightTarget& target : plate.controlledLocalLights)
            {
                out += ' ';
                out += LocalLightKindToken(target.kind);
                out += ' ';
                AppendInt(out, target.index);
            }
        }
        out += '\n';
    }

    for (const DoorSpec& door : level.doors)
    {
        out += "door ";
        AppendVec3(out, door.center);
        out += ' ';
        AppendVec3(out, door.size);
        out += ' ';
        AppendFloat(out, door.openDistance);
        out += ' ';
        if (door.requiredItemId.empty())
        {
            AppendInt(out, 0);
        }
        else
        {
            out += door.requiredItemId;
        }
        out += '\n';
    }

    for (const ItemPickupSpec& pickup : level.itemPickups)
    {
        out += "item_pickup ";
        AppendVec3(out, pickup.position);
        out += ' ';
        AppendInt(out, pickup.quantity);
        out += ' ';
        out.append(pickup.itemId);
        out += ' ';
        out.append(kItemPickupVisualKeyword);
        out += ' ';
        AppendVec3(out, pickup.visualOffset);
        out += ' ';
        AppendVec3(out, pickup.visualRotationDegrees);
        out += ' ';
        AppendVec3(out, pickup.visualScale);
        out += ' ';
        out.append(kItemPickupBoundsKeyword);
        out += ' ';
        AppendInt(out, pickup.showInteractionBounds ? 1 : 0);
        out += ' ';
        out.append(kItemPickupHighlightKeyword);
        out += ' ';
        AppendFloat(out, pickup.targetHighlightIntensity);
        out += ' ';
        out.append(kItemPickupGoldKeyword);
        out += ' ';
        AppendFloat(out, pickup.targetHighlightGoldAmount);
        out += ' ';
        out.append(kItemPickupIdleKeyword);
        out += ' ';
        AppendInt(out, pickup.idleAnimationEnabled ? 1 : 0);
        out += ' ';
        AppendFloat(out, pickup.idleBobAmplitude);
        out += ' ';
        AppendFloat(out, pickup.idleBobSpeed);
        out += ' ';
        AppendFloat(out, pickup.idleSpinSpeedDegrees);
        if (!pickup.modelIdentity.empty())
        {
            out += ' ';
            out.append(pickup.modelIdentity);
        }
        out += '\n';
    }

    for (const StaticPropSpec& prop : level.staticProps)
    {
        out += "static_prop ";
        AppendVec3(out, prop.position);
        out += ' ';
        AppendVec3(out, prop.rotationDegrees);
        out += ' ';
        AppendVec3(out, prop.scale);
        out += ' ';
        out.append(prop.modelIdentity);
        out += '\n';
    }

    for (const AuthoringGroup& group : level.authoringGroups)
    {
        out.append(kAuthoringGroupRecordKeyword);
        out += ' ';
        out.append(group.name);
        for (const AuthoringGroupMember& member : group.members)
        {
            out += ' ';
            out.append(AuthoringGroupMemberKindKeyword(member.kind));
            out += ' ';
            AppendInt(out, static_cast<int>(member.index));
        }
        out += '\n';
    }

    out += "camera ";
    AppendVec3(out, level.camera.offset);
    out += ' ';
    AppendFloat(out, level.camera.fieldOfViewY);
    out += '\n';

    out += "environment ";
    AppendVec3(out, level.environment.ambientColor);
    out += ' ';
    AppendFloat(out, level.environment.ambientIntensity);
    out += '\n';

    out += "directional_light ";
    AppendInt(out, level.environment.directionalEnabled ? 1 : 0);
    out += ' ';
    AppendVec3(out, level.environment.directionalRayDirection);
    out += ' ';
    AppendVec3(out, level.environment.directionalColor);
    out += ' ';
    AppendFloat(out, level.environment.directionalIntensity);
    out += ' ';
    AppendInt(out, level.environment.directionalShadowsEnabled ? 1 : 0);
    out += '\n';

    if (level.hasTerrain)
    {
        out += "terrain ";
        AppendInt(out, level.terrain.enabled ? 1 : 0);
        out += ' ';
        AppendVec3(out, level.terrain.origin);
        out += ' ';
        AppendFloat(out, level.terrain.sizeX);
        out += ' ';
        AppendFloat(out, level.terrain.sizeZ);
        out += ' ';
        AppendInt(out, level.terrain.resolutionX);
        out += ' ';
        AppendInt(out, level.terrain.resolutionZ);
        out += '\n';
        for (int row = 0; row < level.terrain.resolutionZ; ++row)
        {
            out += "terrain_row ";
            AppendInt(out, row);
            for (int column = 0; column < level.terrain.resolutionX; ++column)
            {
                out += ' ';
                AppendFloat(
                    out,
                    level.terrain.heights[static_cast<std::size_t>(
                        TerrainHeightIndex(level.terrain, column, row))]);
            }
            out += '\n';
        }
        if (TerrainMaterialRecordShouldWrite(level.terrain))
        {
            out += "terrain_material ";
            if (TerrainTextureIdentityIsNone(level.terrain.textureIdentity))
            {
                out += assets::kRuntimePngNoneToken;
            }
            else
            {
                out += level.terrain.textureIdentity;
            }
            out += ' ';
            AppendFloat(out, level.terrain.textureTiling);
            out += '\n';
        }
        for (std::size_t extra = 0; extra < level.terrain.extraLayers.size(); ++extra)
        {
            const TerrainMaterialLayer& layer = level.terrain.extraLayers[extra];
            out += "terrain_layer ";
            AppendInt(out, static_cast<int>(extra) + 1);
            out += ' ';
            out += layer.textureIdentity;
            out += ' ';
            AppendFloat(out, layer.textureTiling);
            out += '\n';
        }
        if (TerrainWeightHeaderShouldWrite(level.terrain))
        {
            out += "terrain_weights ";
            AppendInt(out, level.terrain.weightResolutionX);
            out += ' ';
            AppendInt(out, level.terrain.weightResolutionZ);
            out += '\n';
        }
        if (TerrainPaintRecordsShouldWrite(level.terrain))
        {
            const int layerCount = TerrainMaterialLayerCount(level.terrain);
            const int maps = TerrainPackedWeightMapCount(level.terrain);
            for (int map = 0; map < maps; ++map)
            {
                for (int row = 0; row < level.terrain.weightResolutionZ; ++row)
                {
                    out += "terrain_weight ";
                    AppendInt(out, map);
                    out += ' ';
                    AppendInt(out, row);
                    out += ' ';
                    for (int column = 0; column < level.terrain.weightResolutionX; ++column)
                    {
                        const int texelIndex = TerrainWeightTexelIndex(level.terrain, column, row);
                        float weights[kMaxTerrainMaterialLayers];
                        ReadTerrainTexelWeights(level.terrain, texelIndex, weights);
                        int quantized[kMaxTerrainMaterialLayers]{};
                        QuantizeTerrainTexelWeights(weights, layerCount, quantized);
                        AppendTerrainWeightMapHex(quantized, map, out);
                    }
                    out += '\n';
                }
            }
        }
        if (TerrainVegetationShouldWrite(level.terrain))
        {
            out += "terrain_veg ";
            AppendInt(out, level.terrain.vegetationResolutionX);
            out += ' ';
            AppendInt(out, level.terrain.vegetationResolutionZ);
            out += ' ';
            out += std::to_string(level.terrain.vegetationSeed);
            out += '\n';
            for (std::size_t index = 0; index < level.terrain.vegetationEntries.size(); ++index)
            {
                const TerrainVegetationEntry& entry = level.terrain.vegetationEntries[index];
                out += "terrain_veg_entry ";
                AppendInt(out, static_cast<int>(index));
                out += ' ';
                AppendFloat(out, entry.density);
                out += ' ';
                AppendFloat(out, entry.minScale);
                out += ' ';
                AppendFloat(out, entry.maxScale);
                out += ' ';
                AppendInt(out, entry.randomYaw ? 1 : 0);
                out += ' ';
                AppendInt(out, entry.alignToNormal ? 1 : 0);
                out += ' ';
                out += entry.modelIdentity;
                out += '\n';
            }
            for (int row = 0; row < level.terrain.vegetationResolutionZ; ++row)
            {
                if (!TerrainVegetationRowIsOccupied(level.terrain, row))
                {
                    continue;
                }
                out += "terrain_veg_row ";
                AppendInt(out, row);
                out += ' ';
                for (int column = 0; column < level.terrain.vegetationResolutionX; ++column)
                {
                    const int cellIndex = TerrainVegetationCellIndex(level.terrain, column, row);
                    const unsigned char cell =
                        level.terrain.vegetationCells[static_cast<std::size_t>(cellIndex)];
                    out.push_back(TerrainVegetationHexDigit((static_cast<int>(cell) >> 4) & 0xF));
                    out.push_back(TerrainVegetationHexDigit(static_cast<int>(cell) & 0xF));
                    for (int entryIndex = 0; entryIndex < kMaxTerrainVegetationEntries; ++entryIndex)
                    {
                        if (!TerrainVegetationCellHasEntry(cell, entryIndex))
                        {
                            continue;
                        }
                        const int slot = TerrainVegetationDensitySlot(cellIndex, entryIndex);
                        const unsigned char quantum =
                            slot >= 0
                                && slot < static_cast<int>(level.terrain.vegetationDensityQuanta.size())
                            ? level.terrain.vegetationDensityQuanta[static_cast<std::size_t>(slot)]
                            : 0;
                        out.push_back(TerrainVegetationHexDigit((static_cast<int>(quantum) >> 4) & 0xF));
                        out.push_back(TerrainVegetationHexDigit(static_cast<int>(quantum) & 0xF));
                    }
                }
                out += '\n';
            }
            std::string styleHex;
            const int cells = level.terrain.vegetationResolutionX * level.terrain.vegetationResolutionZ;
            for (int cellIndex = 0; cellIndex < cells; ++cellIndex)
            {
                const unsigned char cell =
                    level.terrain.vegetationCells[static_cast<std::size_t>(cellIndex)];
                for (int entryIndex = 0; entryIndex < kMaxTerrainVegetationEntries; ++entryIndex)
                {
                    if (!TerrainVegetationCellHasEntry(cell, entryIndex))
                    {
                        continue;
                    }
                    const int slot = TerrainVegetationDensitySlot(cellIndex, entryIndex);
                    const std::uint16_t packed =
                        slot >= 0 && slot < static_cast<int>(level.terrain.vegetationPaintParams.size())
                        ? level.terrain.vegetationPaintParams[static_cast<std::size_t>(slot)]
                        : 0;
                    const int value = static_cast<int>(packed) & 0xFFF;
                    styleHex.push_back(TerrainVegetationHexDigit((value >> 8) & 0xF));
                    styleHex.push_back(TerrainVegetationHexDigit((value >> 4) & 0xF));
                    styleHex.push_back(TerrainVegetationHexDigit(value & 0xF));
                }
            }
            std::size_t styleCursor = 0;
            while (styleCursor < styleHex.size())
            {
                std::size_t take = styleHex.size() - styleCursor;
                if (take > static_cast<std::size_t>(kTerrainVegetationStyleHexPerRecord))
                {
                    take = static_cast<std::size_t>(kTerrainVegetationStyleHexPerRecord);
                }
                out += "terrain_veg_style ";
                out.append(styleHex, styleCursor, take);
                out += '\n';
                styleCursor += take;
            }
        }
    }

    for (const PointLightSpec& light : level.pointLights)
    {
        out += "point_light ";
        AppendVec3(out, light.position);
        out += ' ';
        AppendVec3(out, light.color);
        out += ' ';
        AppendFloat(out, light.intensity);
        out += ' ';
        AppendFloat(out, light.range);
        out += ' ';
        AppendInt(out, light.enabled ? 1 : 0);
        out += '\n';
    }

    for (const SpotLightSpec& light : level.spotLights)
    {
        out += "spot_light ";
        AppendVec3(out, light.position);
        out += ' ';
        AppendVec3(out, CanonicalSpotLightDirection(light.direction));
        out += ' ';
        AppendVec3(out, light.color);
        out += ' ';
        AppendFloat(out, light.intensity);
        out += ' ';
        AppendFloat(out, light.range);
        out += ' ';
        AppendFloat(out, light.innerConeDegrees);
        out += ' ';
        AppendFloat(out, light.outerConeDegrees);
        out += ' ';
        AppendInt(out, light.enabled ? 1 : 0);
        out += '\n';
    }

    return out;
}

std::filesystem::path LevelFileTemporaryPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return {};
    }

    std::filesystem::path temporary = path;
    temporary += std::string(kLevelFileTemporarySuffix);
    return temporary;
}

WriteLevelFileResult SaveLevelFile(
    const std::filesystem::path& path,
    const LevelDefinition& level)
{
    if (!IsWritableLevelDefinition(level))
    {
        return MakeWriteResult(WriteLevelFileStatus::Invalid, "semantic validation failed");
    }
    // Absolute-only: the authoring target is always supplied explicitly, so a
    // relative path can never resolve against the process CWD.
    if (path.empty() || !path.is_absolute())
    {
        return MakeWriteResult(WriteLevelFileStatus::Error, "path must be absolute");
    }

    const std::filesystem::path temporaryPath = LevelFileTemporaryPath(path);
    if (temporaryPath.empty())
    {
        return MakeWriteResult(WriteLevelFileStatus::Error, "temporary path unavailable");
    }

    const std::string text = SerializeLevelText(level);
    if (text.empty())
    {
        return MakeWriteResult(WriteLevelFileStatus::Invalid, "serialization produced no text");
    }

    {
        std::ofstream stream(temporaryPath, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!stream)
        {
            return MakeWriteResult(WriteLevelFileStatus::Error, "temporary open failed");
        }

        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.flush();
        const bool writeOk = static_cast<bool>(stream);
        stream.close();
        if (!writeOk || stream.fail())
        {
            BestEffortRemove(temporaryPath);
            return MakeWriteResult(WriteLevelFileStatus::Error, "temporary write failed");
        }
    }

    if (!platform::ReplaceFileWithTemporary(temporaryPath, path))
    {
        BestEffortRemove(temporaryPath);
        return MakeWriteResult(WriteLevelFileStatus::Error, "replace failed");
    }

    return MakeWriteResult(WriteLevelFileStatus::Saved, "");
}
}
