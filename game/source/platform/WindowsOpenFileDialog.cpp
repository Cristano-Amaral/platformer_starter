#include "platform/OpenFileDialog.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>

#include <string>
#include <vector>

namespace platform
{
namespace
{
std::wstring Utf8ToWide(std::string_view utf8)
{
    if (utf8.empty())
    {
        return {};
    }
    const int needed = MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0)
    {
        return {};
    }
    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(
        CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), needed);
    return wide;
}

OpenFileDialogResult MakeResult(
    OpenFileDialogStatus status,
    std::string message,
    std::filesystem::path path = {})
{
    OpenFileDialogResult result{};
    result.status = status;
    result.message = std::move(message);
    result.path = std::move(path);
    return result;
}
}

OpenFileDialogResult OpenSingleFileDialog(const OpenFileDialogRequest& request)
{
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool initializedHere = SUCCEEDED(init);
    if (FAILED(init) && init != RPC_E_CHANGED_MODE)
    {
        return MakeResult(OpenFileDialogStatus::Error, "COM could not be initialized for the file dialog");
    }

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || dialog == nullptr)
    {
        if (initializedHere)
        {
            CoUninitialize();
        }
        return MakeResult(OpenFileDialogStatus::Error, "Windows file dialog could not be created");
    }

    FILEOPENDIALOGOPTIONS options = 0;
    if (SUCCEEDED(dialog->GetOptions(&options)))
    {
        options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR;
        dialog->SetOptions(options);
    }

    const std::wstring title = Utf8ToWide(request.title.empty() ? "Open" : request.title);
    if (!title.empty())
    {
        dialog->SetTitle(title.c_str());
    }

    std::vector<std::wstring> labels;
    std::vector<std::wstring> patterns;
    std::vector<COMDLG_FILTERSPEC> specs;
    labels.reserve(request.filters.size());
    patterns.reserve(request.filters.size());
    for (const OpenFileDialogFilter& filter : request.filters)
    {
        labels.push_back(Utf8ToWide(filter.label.empty() ? "Files" : filter.label));
        patterns.push_back(Utf8ToWide(filter.pattern.empty() ? "*.*" : filter.pattern));
    }
    specs.reserve(labels.size());
    for (std::size_t i = 0; i < labels.size(); ++i)
    {
        COMDLG_FILTERSPEC spec{};
        spec.pszName = labels[i].c_str();
        spec.pszSpec = patterns[i].c_str();
        specs.push_back(spec);
    }
    if (!specs.empty())
    {
        dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
        dialog->SetFileTypeIndex(1);
    }

    hr = dialog->Show(nullptr);
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        dialog->Release();
        if (initializedHere)
        {
            CoUninitialize();
        }
        return MakeResult(OpenFileDialogStatus::Cancelled, "file selection cancelled");
    }
    if (FAILED(hr))
    {
        dialog->Release();
        if (initializedHere)
        {
            CoUninitialize();
        }
        return MakeResult(OpenFileDialogStatus::Error, "file dialog failed");
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    dialog->Release();
    if (FAILED(hr) || item == nullptr)
    {
        if (initializedHere)
        {
            CoUninitialize();
        }
        return MakeResult(OpenFileDialogStatus::Error, "file dialog returned no path");
    }

    PWSTR filePath = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
    item->Release();
    if (FAILED(hr) || filePath == nullptr)
    {
        if (initializedHere)
        {
            CoUninitialize();
        }
        return MakeResult(OpenFileDialogStatus::Error, "selected path could not be resolved");
    }

    std::filesystem::path selected = std::filesystem::path(filePath).lexically_normal();
    CoTaskMemFree(filePath);
    if (initializedHere)
    {
        CoUninitialize();
    }
    if (selected.empty() || !selected.is_absolute())
    {
        return MakeResult(OpenFileDialogStatus::Error, "selected path is not absolute");
    }
    return MakeResult(OpenFileDialogStatus::Succeeded, "file selected", std::move(selected));
}
}
