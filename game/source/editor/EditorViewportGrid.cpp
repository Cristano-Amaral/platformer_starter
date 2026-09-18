#include "editor/EditorViewportGrid.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

namespace editor
{
namespace
{
EditorViewportGridPreferences* gBoundViewportGridPreferences = nullptr;

std::string TrimCopy(std::string_view text)
{
    std::size_t begin = 0;
    while (begin < text.size()
        && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r'
            || text[begin] == '\n'))
    {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin
        && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'
            || text[end - 1] == '\n'))
    {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

bool ParseBoolToken(std::string_view text, bool& value)
{
    const std::string token = TrimCopy(text);
    if (token == "1" || token == "true" || token == "True" || token == "TRUE")
    {
        value = true;
        return true;
    }
    if (token == "0" || token == "false" || token == "False" || token == "FALSE")
    {
        value = false;
        return true;
    }
    return false;
}

bool IsViewportGridSectionHeader(std::string_view line)
{
    const std::string trimmed = TrimCopy(line);
    return trimmed == "[Platformer3D.ViewportGrid][Settings]";
}

bool IsIniSectionHeader(std::string_view line)
{
    const std::string trimmed = TrimCopy(line);
    return !trimmed.empty() && trimmed.front() == '[';
}

int FloorDivToInt(float value, float spacing)
{
    return static_cast<int>(std::floor((value / spacing) + 1.0e-4f));
}

int CeilDivToInt(float value, float spacing)
{
    return static_cast<int>(std::ceil((value / spacing) - 1.0e-4f));
}

void TryPushLine(EditorViewportGridLines& lines, const EditorViewportGridLine& line)
{
    if (lines.count < 0 || lines.count >= kEditorViewportGridMaxLines)
    {
        return;
    }
    lines.items[lines.count] = line;
    ++lines.count;
}

EditorViewportGridLineKind KindForConstantX(int minorIndex)
{
    if (minorIndex == 0)
    {
        return EditorViewportGridLineKind::AxisZ;
    }
    return ClassifyEditorViewportGridIndex(minorIndex);
}

EditorViewportGridLineKind KindForConstantZ(int minorIndex)
{
    if (minorIndex == 0)
    {
        return EditorViewportGridLineKind::AxisX;
    }
    return ClassifyEditorViewportGridIndex(minorIndex);
}
}

float EffectiveEditorViewportGridMinorSpacing(float translateIncrement)
{
    return SanitizeSnapIncrement(translateIncrement, kDefaultTranslateSnapIncrement);
}

int EditorViewportGridVisualStride(float minorSpacing, float halfExtent)
{
    const float spacing = EffectiveEditorViewportGridMinorSpacing(minorSpacing);
    float extent = halfExtent;
    if (!std::isfinite(extent) || !(extent > 0.0f))
    {
        extent = kEditorViewportGridMinHalfExtent;
    }

    int stride = 1;
    for (int safety = 0; safety < 12; ++safety)
    {
        const float visualSpacing = spacing * static_cast<float>(stride);
        const int estimated =
            static_cast<int>(std::floor((2.0f * extent) / visualSpacing)) + 1;
        if (estimated <= kEditorViewportGridMaxLinesPerAxis)
        {
            return stride;
        }
        stride *= kEditorViewportGridMajorCadence;
    }
    return stride;
}

EditorViewportGridLineKind ClassifyEditorViewportGridIndex(int minorIndex)
{
    if (minorIndex == 0)
    {
        return EditorViewportGridLineKind::Minor;
    }
    if (minorIndex % kEditorViewportGridMajorCadence == 0)
    {
        return EditorViewportGridLineKind::Major;
    }
    return EditorViewportGridLineKind::Minor;
}

bool EditorViewportGridIsWorldMultiple(float value, float spacing)
{
    const float sanitized = EffectiveEditorViewportGridMinorSpacing(spacing);
    if (!std::isfinite(value))
    {
        return false;
    }
    const float scaled = value / sanitized;
    if (!std::isfinite(scaled))
    {
        return false;
    }
    const float nearest = std::round(scaled);
    return std::fabs(value - nearest * sanitized) <= sanitized * 1.0e-3f + 1.0e-5f;
}

bool EditorViewportGridIncludesOriginAxis(
    const EditorViewportGridLines& lines,
    EditorViewportGridLineKind kind)
{
    const int count = lines.count < 0 ? 0 : lines.count;
    const int limited = count > kEditorViewportGridMaxLines ? kEditorViewportGridMaxLines : count;
    for (int index = 0; index < limited; ++index)
    {
        if (lines.items[index].kind == kind)
        {
            return true;
        }
    }
    return false;
}

EditorViewportGridGenerateRequest MakeEditorViewportGridGenerateRequest(
    float translateIncrement,
    core::Vec3 cameraPosition,
    core::Vec3 cameraTarget)
{
    EditorViewportGridGenerateRequest request{};
    request.translateIncrement = translateIncrement;
    request.focusX = cameraPosition.x;
    request.focusZ = cameraPosition.z;

    const core::Vec3 look{
        cameraTarget.x - cameraPosition.x,
        cameraTarget.y - cameraPosition.y,
        cameraTarget.z - cameraPosition.z};
    constexpr float kMaxFocusAbs = 4096.0f;
    if (std::fabs(look.y) > 1.0e-5f)
    {
        const float t = -cameraPosition.y / look.y;
        if (t > 0.0f)
        {
            const float x = cameraPosition.x + look.x * t;
            const float z = cameraPosition.z + look.z * t;
            if (std::isfinite(x) && std::isfinite(z) && std::fabs(x) <= kMaxFocusAbs
                && std::fabs(z) <= kMaxFocusAbs)
            {
                request.focusX = x;
                request.focusZ = z;
            }
        }
    }

    const float dx = cameraPosition.x - request.focusX;
    const float dy = cameraPosition.y;
    const float dz = cameraPosition.z - request.focusZ;
    float viewDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (!std::isfinite(viewDistance) || viewDistance < 1.0f)
    {
        viewDistance = 1.0f;
    }
    request.halfExtent = viewDistance * 0.85f;
    if (request.halfExtent < kEditorViewportGridMinHalfExtent)
    {
        request.halfExtent = kEditorViewportGridMinHalfExtent;
    }
    if (request.halfExtent > kEditorViewportGridMaxHalfExtent)
    {
        request.halfExtent = kEditorViewportGridMaxHalfExtent;
    }
    return request;
}

EditorViewportGridLines GenerateEditorViewportGridLines(
    const EditorViewportGridGenerateRequest& request)
{
    EditorViewportGridLines lines{};
    const float spacing = EffectiveEditorViewportGridMinorSpacing(request.translateIncrement);
    float halfExtent = request.halfExtent;
    if (!std::isfinite(halfExtent) || !(halfExtent > 0.0f))
    {
        halfExtent = kEditorViewportGridMinHalfExtent;
    }
    if (halfExtent > kEditorViewportGridMaxHalfExtent)
    {
        halfExtent = kEditorViewportGridMaxHalfExtent;
    }

    float focusX = request.focusX;
    float focusZ = request.focusZ;
    if (!std::isfinite(focusX))
    {
        focusX = 0.0f;
    }
    if (!std::isfinite(focusZ))
    {
        focusZ = 0.0f;
    }

    const int stride = EditorViewportGridVisualStride(spacing, halfExtent);
    const float visualSpacing = spacing * static_cast<float>(stride);
    const float xMin = focusX - halfExtent;
    const float xMax = focusX + halfExtent;
    const float zMin = focusZ - halfExtent;
    const float zMax = focusZ + halfExtent;

    const int visX0 = CeilDivToInt(xMin, visualSpacing);
    const int visX1 = FloorDivToInt(xMax, visualSpacing);
    const int visZ0 = CeilDivToInt(zMin, visualSpacing);
    const int visZ1 = FloorDivToInt(zMax, visualSpacing);

    for (int vis = visX0; vis <= visX1; ++vis)
    {
        const int minorIndex = vis * stride;
        const float x = static_cast<float>(minorIndex) * spacing;
        EditorViewportGridLine line{};
        line.start = {x, kEditorViewportGridY, zMin};
        line.end = {x, kEditorViewportGridY, zMax};
        line.kind = KindForConstantX(minorIndex);
        TryPushLine(lines, line);
    }
    for (int vis = visZ0; vis <= visZ1; ++vis)
    {
        const int minorIndex = vis * stride;
        const float z = static_cast<float>(minorIndex) * spacing;
        EditorViewportGridLine line{};
        line.start = {xMin, kEditorViewportGridY, z};
        line.end = {xMax, kEditorViewportGridY, z};
        line.kind = KindForConstantZ(minorIndex);
        TryPushLine(lines, line);
    }
    return lines;
}

bool ParseEditorViewportGridPreferenceLine(
    EditorViewportGridPreferences& preferences,
    std::string_view line)
{
    const std::string trimmed = TrimCopy(line);
    if (trimmed.empty() || trimmed.front() == ';' || trimmed.front() == '#')
    {
        return false;
    }

    const std::size_t equals = trimmed.find('=');
    if (equals == std::string::npos)
    {
        return false;
    }

    const std::string key = TrimCopy(trimmed.substr(0, equals));
    const std::string_view value = std::string_view(trimmed).substr(equals + 1);
    if (key == "Visible")
    {
        bool visible = preferences.visible;
        if (ParseBoolToken(value, visible))
        {
            preferences.visible = visible;
            return true;
        }
        return false;
    }
    return false;
}

EditorViewportGridPreferences ParseEditorViewportGridPreferencesFromLayoutText(
    std::string_view layoutText)
{
    EditorViewportGridPreferences preferences = MakeDefaultEditorViewportGridPreferences();
    std::istringstream stream{std::string(layoutText)};
    std::string line;
    bool inSection = false;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (IsViewportGridSectionHeader(line))
        {
            inSection = true;
            continue;
        }
        if (inSection && IsIniSectionHeader(line))
        {
            break;
        }
        if (inSection)
        {
            ParseEditorViewportGridPreferenceLine(preferences, line);
        }
    }
    return preferences;
}

std::string SerializeEditorViewportGridPreferencesSection(
    const EditorViewportGridPreferences& preferences)
{
    std::ostringstream out;
    out << '[' << kEditorViewportGridIniTypeName << "][" << kEditorViewportGridIniEntryName
        << "]\n";
    out << "Visible=" << (preferences.visible ? 1 : 0) << '\n';
    return out.str();
}

std::string MergeEditorViewportGridPreferencesIntoLayoutText(
    std::string_view layoutText,
    const EditorViewportGridPreferences& preferences)
{
    const std::string section = SerializeEditorViewportGridPreferencesSection(preferences);
    std::istringstream stream{std::string(layoutText)};
    std::ostringstream out;
    std::string line;
    bool inSection = false;
    bool written = false;
    bool needLeadingNewline = false;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (IsViewportGridSectionHeader(line))
        {
            inSection = true;
            if (!written)
            {
                if (needLeadingNewline)
                {
                    out << '\n';
                }
                out << section;
                written = true;
                needLeadingNewline = true;
            }
            continue;
        }
        if (inSection)
        {
            if (IsIniSectionHeader(line))
            {
                inSection = false;
            }
            else
            {
                continue;
            }
        }
        if (needLeadingNewline && !line.empty())
        {
            out << '\n';
        }
        out << line << '\n';
        needLeadingNewline = false;
    }
    if (!written)
    {
        std::string existing = out.str();
        if (!existing.empty() && existing.back() != '\n')
        {
            existing.push_back('\n');
        }
        if (!existing.empty() && (existing.size() < 2 || existing[existing.size() - 2] != '\n'))
        {
            existing.push_back('\n');
        }
        existing += section;
        return existing;
    }
    return out.str();
}

EditorViewportGridPreferences LoadEditorViewportGridPreferencesFromLayoutPath(
    const std::filesystem::path& path)
{
    if (path.empty())
    {
        return MakeDefaultEditorViewportGridPreferences();
    }

    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
    {
        return MakeDefaultEditorViewportGridPreferences();
    }

    std::ifstream in(path);
    if (!in)
    {
        return MakeDefaultEditorViewportGridPreferences();
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return ParseEditorViewportGridPreferencesFromLayoutText(buffer.str());
}

bool SaveEditorViewportGridPreferencesToLayoutPath(
    const std::filesystem::path& path,
    const EditorViewportGridPreferences& preferences)
{
    if (path.empty())
    {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        return false;
    }

    std::string existing;
    {
        std::ifstream in(path);
        if (in)
        {
            std::ostringstream buffer;
            buffer << in.rdbuf();
            existing = buffer.str();
        }
    }

    const std::string merged =
        MergeEditorViewportGridPreferencesIntoLayoutText(existing, preferences);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << merged;
    return static_cast<bool>(out);
}

void BindEditorViewportGridPreferences(EditorViewportGridPreferences* preferences)
{
    gBoundViewportGridPreferences = preferences;
}

EditorViewportGridPreferences* BoundEditorViewportGridPreferences()
{
    return gBoundViewportGridPreferences;
}
}
