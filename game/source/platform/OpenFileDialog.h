#pragma once

// Narrow Development file-selection boundary for M47 static GLB import.
// Generic editor code must not include OS headers. Windows implementation:
// platform/WindowsOpenFileDialog.cpp. Other hosts/configs:
// platform/OpenFileDialogStub.cpp.
//
// This is not a filesystem browser, Content Browser, or generic dialog
// framework. FOS_NOCHANGEDIR (Windows) keeps the process CWD untouched.

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace platform
{
enum class OpenFileDialogStatus
{
    Succeeded,
    Cancelled,
    Unavailable,
    Error,
};

struct OpenFileDialogFilter
{
    std::string label;
    std::string pattern;
};

struct OpenFileDialogRequest
{
    std::string title;
    std::vector<OpenFileDialogFilter> filters;
};

struct OpenFileDialogResult
{
    OpenFileDialogStatus status = OpenFileDialogStatus::Unavailable;
    std::filesystem::path path;
    std::string message;
};

OpenFileDialogResult OpenSingleFileDialog(const OpenFileDialogRequest& request);
}
