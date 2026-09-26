#include <raylib.h>

#include <cstring>
#include <iostream>

namespace
{
bool HasAnimation(const ModelAnimation* animations, int animationCount, const char* name)
{
    for (int index = 0; index < animationCount; ++index)
    {
        if (std::strcmp(animations[index].name, name) == 0)
            return true;
    }
    return false;
}
}

int main()
{
    int animationCount = 0;
    ModelAnimation* animations = LoadModelAnimations(PLATFORMER_PLAYER_GLB, &animationCount);

    const bool valid = animations != nullptr
        && animationCount == 3
        && HasAnimation(animations, animationCount, "Idle")
        && HasAnimation(animations, animationCount, "Move")
        && HasAnimation(animations, animationCount, "Jump");

    if (animations != nullptr)
        UnloadModelAnimations(animations, animationCount);

    if (!valid)
    {
        std::cerr << "Canonical Player GLB did not load its Idle/Move/Jump clips through raylib.\n";
        return 1;
    }

    animationCount = 0;
    animations = LoadModelAnimations(PLATFORMER_REUSABLE_ANIMATION_GLB, &animationCount);
    const bool reusableValid = animations != nullptr && animationCount == 3
        && HasAnimation(animations, animationCount, "Idle")
        && HasAnimation(animations, animationCount, "Move")
        && HasAnimation(animations, animationCount, "Jump");
    if (animations != nullptr) UnloadModelAnimations(animations, animationCount);
    if (!reusableValid)
    {
        std::cerr << "Canonical reusable animation GLB failed the production raylib loader.\n";
        return 1;
    }

    std::cout << "PlayerAnimationAssetTest passed\n";
    return 0;
}
