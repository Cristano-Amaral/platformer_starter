#pragma once

#include "input/InputState.h"

namespace input
{
InputState Poll();
// RMB look: hide the cursor only while active. Never a process-wide trap.
void SetMouseLookActive(bool active);
}
