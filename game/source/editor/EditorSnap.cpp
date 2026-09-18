#include "editor/EditorSnap.h"

#include "world/StaticProp.h"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

namespace editor
{
namespace
{
EditorSnapPreferences* gBoundSnapPreferences = nullptr;

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

bool StartsWith(std::string_view text, std::string_view prefix)
{
    return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
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

bool ParseFloatToken(std::string_view text, float& value)
{
    const std::string token = TrimCopy(text);
    if (token.empty())
    {
        return false;
    }
    try
    {
        std::size_t consumed = 0;
        const float parsed = std::stof(token, &consumed);
        if (consumed != token.size() || !std::isfinite(parsed))
        {
            return false;
        }
        value = parsed;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool IsSnapSectionHeader(std::string_view line)
{
    const std::string trimmed = TrimCopy(line);
    return trimmed == "[Platformer3D.Snap][Settings]";
}

bool IsIniSectionHeader(std::string_view line)
{
    const std::string trimmed = TrimCopy(line);
    return !trimmed.empty() && trimmed.front() == '[';
}
}

float SanitizeSnapIncrement(float value, float fallback)
{
    if (!std::isfinite(fallback) || !(fallback > 0.0f))
    {
        fallback = kMinSnapIncrement;
    }
    if (!std::isfinite(value) || !(value > 0.0f))
    {
        return fallback;
    }
    if (value < kMinSnapIncrement)
    {
        return kMinSnapIncrement;
    }
    if (value > kMaxSnapIncrement)
    {
        return kMaxSnapIncrement;
    }
    return value;
}

void SanitizeEditorSnapPreferences(EditorSnapPreferences& preferences)
{
    preferences.translateIncrement = SanitizeSnapIncrement(
        preferences.translateIncrement, kDefaultTranslateSnapIncrement);
    preferences.resizeIncrement =
        SanitizeSnapIncrement(preferences.resizeIncrement, kDefaultResizeSnapIncrement);
    preferences.scaleIncrement =
        SanitizeSnapIncrement(preferences.scaleIncrement, kDefaultScaleSnapIncrement);
    preferences.rotateIncrementDegrees = SanitizeSnapIncrement(
        preferences.rotateIncrementDegrees, kDefaultRotateSnapIncrementDegrees);
}

float EditorSnapDefaultIncrement(EditorTransformMode mode)
{
    switch (mode)
    {
    case EditorTransformMode::Resize:
        return kDefaultResizeSnapIncrement;
    case EditorTransformMode::Scale:
        return kDefaultScaleSnapIncrement;
    case EditorTransformMode::Rotate:
        return kDefaultRotateSnapIncrementDegrees;
    case EditorTransformMode::Translate:
        break;
    }
    return kDefaultTranslateSnapIncrement;
}

float EditorSnapIncrementForMode(
    const EditorSnapPreferences& preferences,
    EditorTransformMode mode)
{
    switch (mode)
    {
    case EditorTransformMode::Resize:
        return SanitizeSnapIncrement(
            preferences.resizeIncrement, kDefaultResizeSnapIncrement);
    case EditorTransformMode::Scale:
        return SanitizeSnapIncrement(preferences.scaleIncrement, kDefaultScaleSnapIncrement);
    case EditorTransformMode::Rotate:
        return SanitizeSnapIncrement(
            preferences.rotateIncrementDegrees, kDefaultRotateSnapIncrementDegrees);
    case EditorTransformMode::Translate:
        break;
    }
    return SanitizeSnapIncrement(
        preferences.translateIncrement, kDefaultTranslateSnapIncrement);
}

float* EditorSnapIncrementPointer(
    EditorSnapPreferences& preferences,
    EditorTransformMode mode)
{
    switch (mode)
    {
    case EditorTransformMode::Resize:
        return &preferences.resizeIncrement;
    case EditorTransformMode::Scale:
        return &preferences.scaleIncrement;
    case EditorTransformMode::Rotate:
        return &preferences.rotateIncrementDegrees;
    case EditorTransformMode::Translate:
        break;
    }
    return &preferences.translateIncrement;
}

const char* EditorSnapIncrementLabel(EditorTransformMode mode)
{
    switch (mode)
    {
    case EditorTransformMode::Resize:
        return "Resize Inc";
    case EditorTransformMode::Scale:
        return "Scale Inc";
    case EditorTransformMode::Rotate:
        return "Rotate Inc";
    case EditorTransformMode::Translate:
        break;
    }
    return "Translate Inc";
}

float QuantizeToIncrement(float value, float increment)
{
    if (!std::isfinite(value) || !std::isfinite(increment) || !(increment > 0.0f))
    {
        return value;
    }

    const float scaled = value / increment;
    if (!std::isfinite(scaled))
    {
        return value;
    }

    // std::round: ties (exactly 0.5) go away from zero.
    const float quantized = std::round(scaled) * increment;
    if (!std::isfinite(quantized))
    {
        return value;
    }
    return quantized;
}

core::Vec3 QuantizeVec3Axis(core::Vec3 value, EditorAxis axis, float increment)
{
    switch (axis)
    {
    case EditorAxis::X:
        value.x = QuantizeToIncrement(value.x, increment);
        break;
    case EditorAxis::Y:
        value.y = QuantizeToIncrement(value.y, increment);
        break;
    case EditorAxis::Z:
        value.z = QuantizeToIncrement(value.z, increment);
        break;
    case EditorAxis::None:
        break;
    }
    return value;
}

core::Vec3 ApplyAuthoredTransformSnap(
    core::Vec3 intended,
    EditorTransformMode mode,
    EditorAxis axis,
    bool snapEnabled,
    float increment)
{
    if (snapEnabled)
    {
        intended = QuantizeVec3Axis(intended, axis, increment);
    }

    switch (mode)
    {
    case EditorTransformMode::Resize:
        return ClampAuthoredBoxSize(intended);
    case EditorTransformMode::Scale:
        return world::ClampStaticPropScale(intended);
    case EditorTransformMode::Translate:
    case EditorTransformMode::Rotate:
        break;
    }
    return intended;
}

bool ParseEditorSnapPreferenceLine(EditorSnapPreferences& preferences, std::string_view line)
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
    if (key == "Enabled")
    {
        bool enabled = preferences.enabled;
        if (ParseBoolToken(value, enabled))
        {
            preferences.enabled = enabled;
            return true;
        }
        return false;
    }

    float parsed = 0.0f;
    if (!ParseFloatToken(value, parsed))
    {
        return false;
    }
    if (key == "Translate")
    {
        preferences.translateIncrement = parsed;
        return true;
    }
    if (key == "Resize")
    {
        preferences.resizeIncrement = parsed;
        return true;
    }
    if (key == "Scale")
    {
        preferences.scaleIncrement = parsed;
        return true;
    }
    if (key == "Rotate")
    {
        preferences.rotateIncrementDegrees = parsed;
        return true;
    }
    return false;
}

EditorSnapPreferences ParseEditorSnapPreferencesFromLayoutText(std::string_view layoutText)
{
    EditorSnapPreferences preferences = MakeDefaultEditorSnapPreferences();
    std::istringstream stream{std::string(layoutText)};
    std::string line;
    bool inSection = false;
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (IsSnapSectionHeader(line))
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
            ParseEditorSnapPreferenceLine(preferences, line);
        }
    }
    SanitizeEditorSnapPreferences(preferences);
    return preferences;
}

std::string SerializeEditorSnapPreferencesSection(const EditorSnapPreferences& preferences)
{
    EditorSnapPreferences sanitized = preferences;
    SanitizeEditorSnapPreferences(sanitized);

    std::ostringstream out;
    out << '[' << kEditorSnapIniTypeName << "][" << kEditorSnapIniEntryName << "]\n";
    out << "Enabled=" << (sanitized.enabled ? 1 : 0) << '\n';
    out << "Translate=" << sanitized.translateIncrement << '\n';
    out << "Resize=" << sanitized.resizeIncrement << '\n';
    out << "Scale=" << sanitized.scaleIncrement << '\n';
    out << "Rotate=" << sanitized.rotateIncrementDegrees << '\n';
    return out.str();
}

std::string MergeEditorSnapPreferencesIntoLayoutText(
    std::string_view layoutText,
    const EditorSnapPreferences& preferences)
{
    const std::string section = SerializeEditorSnapPreferencesSection(preferences);
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
        if (IsSnapSectionHeader(line))
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

EditorSnapPreferences LoadEditorSnapPreferencesFromLayoutPath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return MakeDefaultEditorSnapPreferences();
    }

    std::error_code error;
    if (!std::filesystem::exists(path, error) || error)
    {
        return MakeDefaultEditorSnapPreferences();
    }

    std::ifstream in(path);
    if (!in)
    {
        return MakeDefaultEditorSnapPreferences();
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();
    return ParseEditorSnapPreferencesFromLayoutText(buffer.str());
}

bool SaveEditorSnapPreferencesToLayoutPath(
    const std::filesystem::path& path,
    const EditorSnapPreferences& preferences)
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

    const std::string merged = MergeEditorSnapPreferencesIntoLayoutText(existing, preferences);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << merged;
    return static_cast<bool>(out);
}

void BindEditorSnapPreferences(EditorSnapPreferences* preferences)
{
    gBoundSnapPreferences = preferences;
}

EditorSnapPreferences* BoundEditorSnapPreferences()
{
    return gBoundSnapPreferences;
}
}
