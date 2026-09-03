# PLATFORMER_LEVEL v1

Human-readable project-owned level format. No JSON/YAML/TOML/XML. No third-party
serializer.

**Milestone 31 (complete):** this file is the live authored source for Level 01.
Application loads the staged cooked copy once at Initialize. There is no
compiled `CreateLevel01Definition()` / `Level01.cpp` fallback.

**Milestone 32 Phase B:** the Development editor writes this format live. `Save
Level Source` serializes the **active/applied** `LevelDefinition`, never
unapplied editor edits, and only the source row below. Canonical formatting
applies on the first editor save, so `25.60` becomes `25.6`; the parsed value is
identical.

**Milestone 32 Phase A:** v1 is now read *and* written by the game. There is no
version 2, no editor-only format, no intermediate JSON, and no hidden binary
file. The writer emits exactly this contract.

## Identities

| Role | Path |
|---|---|
| Source (authored) | `game/assets/source/levels/level_01.level` |
| Cooked | `game/assets/cooked/levels/level_01.level` |
| Staged / runtime | `<executable directory>/assets/levels/level_01.level` |

Logical runtime id: `levels/level_01.level` via `platform::RuntimeAssetPath`.
The runtime never reads `assets/source`.

The Development editor writes only the source row and never the staged runtime
copy. It resolves that path through `editor::AuthoringLevel01SourcePath`, whose
root is injected by CMake for the Development configuration only. Saving the
source does not update the cooked or staged copies; the normal
`python tools/cook_assets.py` + `cmake --build` steps do that.

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
(do not infer it from the filename). Application requires `level_01`; the
parser and writer accept any valid identifier.

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

`LevelFileTest` asserts this by whitelist: every keyword the writer emits must
be one of the 17 v1 keywords, so no runtime state can appear in output.

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
`world::IsValidLevelIdToken` is the shared `id` grammar rule, so the writer
cannot emit an identifier the parser would reject.

## Writer API

`world/LevelWriter.h` (M32 Phase A):

```
bool                 world::IsWritableLevelDefinition(const LevelDefinition&);
std::string          world::SerializeLevelText(const LevelDefinition&);
WriteLevelFileResult world::SaveLevelFile(const std::filesystem::path&, const LevelDefinition&);
std::filesystem::path world::LevelFileTemporaryPath(const std::filesystem::path&);
```

`WriteLevelFileStatus` is `Saved` / `Invalid` / `Error` plus a short error
string. It is not a generic engine-wide result type.

`IsWritableLevelDefinition` is `IsValidLevelIdToken(level.id)` plus
`LevelDefinitionHasRequiredAuthoredContent`. `SerializeLevelText` returns an
empty string when that gate fails, so an invalid definition can never reach a
file. `SaveLevelFile` requires an **absolute** path: the writer never resolves
against the process current working directory.

Safe write sequence: validate, serialize, write the sibling temp
`<target>.tmp`, flush and close, then promote through
`platform::ReplaceFileWithTemporary` (the M29 boundary). The temp is removed on
any failure, and the writer itself contains no Win32 calls.

## Canonical writer output order

The parser accepts records in any order; the writer emits exactly one order.
Serializing the same `LevelDefinition` twice yields byte-identical output.
Source whitespace, blank lines, and original record order are **not**
preserved — the editor owns semantic data, not a text AST.

```
PLATFORMER_LEVEL 1
id
spawn
kill_plane
ground
platform            x6, elevatedPlatforms index order
support_index_cp1
support_index_cp2
support_index_goal
slope               x2, slopes index order
moving_platform
checkpoint          x2, checkpoints index order
hazard              x2, hazards index order
collectible         x3, collectibles index order
goal
dynamic_box
camera
```

One record per line, single-space separated, `\n` line endings, trailing
newline after the last record, no BOM, no comments, no blank lines.

## Writer numeric policy

`std::to_chars` shortest round-trip form for every `float`, and the integer
overload for the version and `support_index_*`. That is locale-independent by
construction and `std::from_chars` recovers the exact same `float`, so no
precision is lost. `std::numeric_limits<float>::max_digits10` (9) would also
round-trip but emits longer, noisier text.

NaN and Inf are never serialized: validation rejects them before any text is
produced.
