#include "ui/debug/DebugUi.h"

namespace ui
{
void DebugUi::Initialize()
{
    backend.Initialize();
    panelVisible = true;
}

void DebugUi::Shutdown()
{
    backend.Shutdown();
}

void DebugUi::Draw(const DebugMetricsSnapshot& snapshot)
{
    if (backend.ConsumeTogglePressed())
    {
        panelVisible = !panelVisible;
    }

    backend.BeginFrame();
    if (panelVisible)
    {
        DrawDebugMetrics(snapshot);
    }
    backend.EndFrame();
}
}
