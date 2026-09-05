# Canonical cooked runtime asset inventory.
# Shared by:
#   - CMake configure-time cooked-file checks
#   - Platformer3D POST_BUILD staging
#   - editor Stage Runtime Assets (cmake -P cmake/StageRuntimeAssets.cmake)
#
# Paths are relative to game/assets/cooked/ and to <exe>/assets/.
# Do not list cooker-only authoring inputs (for example
# textures/test_textured_basecolor.png) or cooked metadata (manifest.json).

set(PLATFORMER_RUNTIME_ASSETS
    textures/test_checker.png
    models/test_static.glb
    models/test_authored.glb
    models/test_textured.glb
    levels/level_01.level
)
