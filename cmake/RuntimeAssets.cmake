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
    models/player.glb
    models/test_static.glb
    models/test_authored.glb
    models/test_textured.glb
    levels/level_01.level
    levels/level_02.level
    sounds/item_pickup_collect.wav
    sounds/player_damage.wav
    sounds/player_death.wav
    sounds/player_respawn.wav
    sounds/player_footstep.wav
    sounds/player_jump.wav
    sounds/player_land.wav
    sounds/checkpoint_activate.wav
    sounds/pressure_plate_activate.wav
    sounds/pressure_plate_deactivate.wav
    sounds/door_unlock.wav
    sounds/level_goal_complete.wav
    sounds/collectible_collect.wav
    sounds/ui_navigate.wav
    sounds/ui_confirm.wav
    sounds/pause_open.wav
    sounds/pause_close.wav
    sounds/inventory_open.wav
    sounds/inventory_close.wav
)
