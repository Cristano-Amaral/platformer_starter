#include "platform/OpenFileDialog.h"

namespace platform
{
OpenFileDialogResult OpenSingleFileDialog(const OpenFileDialogRequest&)
{
    OpenFileDialogResult result{};
    result.status = OpenFileDialogStatus::Unavailable;
    result.message = "file selection is unavailable in this configuration";
    return result;
}
}
