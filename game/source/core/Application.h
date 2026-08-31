#pragma once

#include "platform/Window.h"

namespace core
{
class Application
{
public:
    int Run();

private:
    void Initialize();
    void Shutdown();

    platform::Window window;
    bool initialized = false;
};
}
