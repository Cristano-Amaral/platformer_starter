#include "platform/Time.h"

#include "raylib.h"

namespace platform
{
float DeltaSeconds()
{
    return GetFrameTime();
}
}
