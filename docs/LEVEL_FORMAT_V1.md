# PLATFORMER_LEVEL v1

Human-readable project-owned level format. No JSON/YAML/TOML/XML. No third-party
serializer. This document is the writer contract for a future Development editor.

**Milestone 31 Phase B:** this file is the live authored source for Level 01.
Application loads the staged cooked copy once at Initialize. There is no
compiled `CreateLevel01Definition()` / `Level01.cpp` fallback. Phase C manual
validation is still required before M31 is complete.

## Identities

| Role | Path |
|---|---|
| Source (authored) | `game/assets/source/levels/level_01.level` |
| Cooked | `game/assets/cooked/levels/level_01.level` |
| Staged / runtime | `<executable directory>/assets/levels/level_01.level` |

Logical runtime id: `levels/level_01.level` via `platform::RuntimeAssetPath`.
The executable never reads `assets/source`.

## Header

First non-blank line:

```
PLATFORMER_LEVEL 1
```

Magic is `PLATFORMER_LEVEL`. Version token must be exactly `1` for v1.
Any other unsigned integer version is `UnsupportedVersion`.
Any other header is `Invalid`.

## Records

Keywords are case-sensitive ASCII. Tokens are separated by spaces or tabs.
Blank lines are ignored. Leading/trailing spaces/tabs on a line are ignored.
`CRLF` and `LF` are accepted (`CR` is stripped). UTF-8 without BOM.
No comments in v1.

After the header, records may appear in any order. Encounter order of repeated
records (`platform`, `slope`, `checkpoint`, `hazard`, `collectible`) is the
array order in `LevelDefinition`. Singleton records must appear exactly once.
Unknown keywords and trailing unrecognized content are `Invalid`.

### Required singletons

```
id <token>
spawn <x> <y> <z>
kill_plane <y>
ground <cx> <cy> <cz> <sx> <sy> <sz>
support_index_cp1 <int>
support_index_cp2 <int>
support_index_goal <int>
moving_platform <sx> <sy> <sz> <centerY> <centerZ> <pathMinX> <pathMaxX> <speed> <startX>
goal <cx> <cy> <cz> <sx> <sy> <sz>
dynamic_box <cx> <cy> <cz> <sx> <sy> <sz> <mass>
camera <ox> <oy> <oz> <fovY>
```

`id` is `[A-Za-z_][A-Za-z0-9_]*`. The file must contain the authored identity
(do not infer it from the filename). M31 bootstrap will require `level_01`.

### Required repeated records (fixed M30 Level 01 counts)

```
platform <cx> <cy> <cz> <sx> <sy> <sz>     # exactly 6
slope <cx> <cy> <cz> <sx> <sy> <sz> <rotZ> # exactly 2
checkpoint <cx> <cy> <cz> <sx> <sy> <sz> <rx> <ry> <rz>  # exactly 2
hazard <cx> <cy> <cz> <sx> <sy> <sz>       # exactly 2
collectible <cx> <cy> <cz> <sx> <sy> <sz>  # exactly 3
```

`support_index_*` are 0-based indices into the `platform` array (M30
`checkpoint1PlatformIndex` / `checkpoint2PlatformIndex` / `goalPlatformIndex`).
They are authored validation metadata, not gameplay runtime state.

## Numbers

Locale-independent `std::from_chars`. Whole token must parse. Reject overflow,
NaN, Inf, leftover suffix (`1.0f`), and empty tokens. Integers for version and
support indices.

Positive sizes: each component finite and `> 0`.
Moving platform: size positive, path min `<` path max, speed `> 0`, startX
inside `[pathMinX, pathMaxX]`.
Dynamic box mass finite and `> 0`.
Camera FOV finite, `> 0` and `< 180` (same range as M30).

## Not in the file

Runtime: active checkpoint, respawn position, death count, collected flags,
completion, TIME, BEST, moving-platform pose/direction, cyan-box pose, BodyIDs,
camera smoothed target.

Player/controller policy: accel/decel/speed/gravity/jump/coyote/buffer,
CharacterVirtual max slope and shape, `kPlayerVisualSize`, inner-body settings.

Camera follow policy: dead zone X/Y, follow sharpness.

## Cooker

Kind `level_v1`: UTF-8 + header `PLATFORMER_LEVEL 1`, then **byte-for-byte copy**.
No whitespace canonicalization. Incremental skip uses `sourceSha256` like other
copy assets. Full grammar validation is C++ `ParseLevelText` only.

## Parser API

`world::ParseLevelText` / `world::LoadLevelFile` in `world/LevelFile.h`.
Statuses: `Loaded`, `Missing`, `Invalid`, `UnsupportedVersion`, `Error`.
`Missing` is a missing file at load time, not empty text (empty text is
`Invalid`).
`LevelDefinition.id` is an owning `std::string`; the parsed definition remains
valid after the source text is destroyed.
