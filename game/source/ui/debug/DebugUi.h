#pragma once

#include "ui/debug/DebugMetrics.h"
#include "ui/debug/DebugUiBackend.h"

namespace ui
{
class DebugUi
{
public:
    void Initialize();
    void Shutdown();
    void Draw(const DebugMetricsSnapshot& snapshot);

private:
    DebugUiBackend backend;
    bool panelVisible = true;
};
}
