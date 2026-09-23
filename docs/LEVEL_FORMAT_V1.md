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
| Source (authored) | `game/assets/source/levels/<id>.level` (canonical `level_01`, `level_02`; extra valid files are discovered) |
| Cooked | `game/assets/cooked/levels/<id>.level` |
| Staged / runtime | `<executable directory>/assets/levels/<id>.level` |

Logical runtime ids: `levels/<id>.level` via `platform::RuntimeAssetPath`.
The runtime never reads `assets/source`. Destination-bearing Level Goals resolve only through that staged runtime convention. There is no source-tree fallback. Development Levels UI discovers authored `source/levels/*.level` files for Open/New/Next Level selection only. Extra valid source levels cook as `level_v1` and extra cooked levels stage without a per-level CMake inventory edit; required inventory remains `level_01` and `level_02`. A deleted extra authored Level is removed from cooked and staged `levels/*.level` on the next cook/stage; other asset categories are not scanned.

The Development editor writes the source row for the **current runtime level identity** (`levels/<id>.level`) and never the staged runtime
copy. It resolves that path through `editor::AuthoringLevelSourcePath`, whose
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
records (`platform`, `slope`, `checkpoint`, `hazard`, `collectible`,
`level_goal`, `dynamic_box`, `pressure_plate`, `door`, `item_pickup`, `static_prop`, `authoring_group`) is the array order in `LevelDefinition`. Required singleton records must
appear exactly once. Optional singleton records (`environment`, `directional_light`, `terrain` plus its `terrain_row` samples) may be omitted and must not be duplicated. Unknown keywords and trailing unrecognized content are
`Invalid`. Optional `authoring_group` records persist Development Authoring
Groups; they are authored organizational metadata, not gameplay objects.

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
camera <ox> <oy> <oz> <fovY>
```

`id` is `[A-Za-z_][A-Za-z0-9_]*`. The file must contain the authored identity
(do not infer it from the filename). Application Initialize requires `level_01`.
After a successful M64 transition the active identity is the destination
(`level_02`). The parser and writer accept any valid identifier. Destination
tokens on `level_goal` reuse the same identity grammar; they are not paths.
The player is not an authored Level object. There is no `player_model`
record, player mesh property, or per-Level presentation block. Milestone 83
loads staged `models/player.glb` as a runtime presentation follower.

### Optional lighting singletons (Milestone 85.1)

```
environment <r> <g> <b> <intensity>
directional_light <enabled> <dx> <dy> <dz> <r> <g> <b> <intensity> <shadowsEnabled>
```

These author the single M85 lighting environment. They are not a light list,
Component, Scene node, or gameplay activation target. There is no
`ambient_light`, `shadow_settings`, or `linkedLightIndex` record. Milestone
85.3 adds optional repeatable `point_light` / `spot_light` records below;
they are not Directional Light and are not gameplay targets.

Omitted records resolve to M85-compatible defaults: ambient color `{1,1,1}`
intensity `0.34`; directional enabled `1`, ray `normalize(-0.42,-1,-0.38)`,
color `{1,0.96,0.88}`, intensity `0.88`, shadows enabled `1`. Canonical
`level_01` / `level_02` stay valid without these lines.

`enabled` and `shadowsEnabled` are exact `0`/`1` bools (same grammar as
Pressure Plate flags). Colors are finite in `[0,1]`. Intensities are finite
and non-negative (`ambient <= 2`, directional `<= 4`). Direction is the
**ray-travel** vector (from the sun toward surfaces), finite and non-zero;
the active representation is normalized. Duplicate records are `Invalid`.

`Enabled = 0` disables directional illumination and directional shadows;
ambient remains. `Shadows Enabled = 0` with `Enabled = 1` keeps directional
lighting without the shadow pass. Intensity `0` is not a substitute for
`enabled`. M85.2 Pressure Plate `controlsDirectionalLight` may gate
effective enablement at runtime; it is not a generic event/receiver
system.

The writer always emits both records after `camera`.

### Optional local lights (Milestone 85.3)

```
point_light <px> <py> <pz> <r> <g> <b> <intensity> <range> <enabled>
spot_light <px> <py> <pz> <dx> <dy> <dz> <r> <g> <b> <intensity> <range> <innerConeDegrees> <outerConeDegrees> <enabled>
```

Repeatable authored Point and Spot Lights. Zero or more of each. Encounter
order is `LevelDefinition` container order. Old Levels without these records
remain valid and keep M85–M85.2 global lighting semantics. Canonical
`level_01` / `level_02` stay valid without these lines. The format version
stays `1`.

`enabled` is an exact `0`/`1` bool. Colors are finite in `[0,1]`. Intensity
is finite in `[0, 4]` (same ceiling as Directional). Range is finite in
`[0.1, 64]`. Spot direction is finite, non-zero, and stored normalized
(illumination axis / ray travel). Cone angles are degrees:
`0 <= inner`, `outer <= 89`, and `outer - inner >= 0.5`. Token counts are
strict (10 / 15). Malformed records are `Invalid`.

Defaults for newly created editor lights: color `{1, 0.95, 0.85}`, intensity
`1.5`, range `8`, enabled `1`, Spot direction `{0, -1, 0}`, inner `20`,
outer `35`.

Local lights are presentation lighting. M85.4 Pressure Plates may target
specific Point/Spot Lights by typed index. They are not Directional Light
and do not cast shadows. Pressure Plate `controlsDirectionalLight` still
names only the singleton Directional Light.

The writer emits `point_light` records then `spot_light` records after
`directional_light` (and after optional `terrain` / `terrain_row` /
`terrain_material` / `terrain_layer` / `terrain_weights` / `terrain_weight` /
`terrain_veg` / `terrain_veg_entry` / `terrain_veg_row` / `terrain_veg_style`
`terrain_cover` / `terrain_cover_entry` / `terrain_cover_data`
when present; legacy `terrain_paint` is read but not written), omitted when the collections are empty.

### Optional Terrain singleton (Milestone 86 / 88)

```
terrain <enabled> <originX> <originY> <originZ> <sizeX> <sizeZ> <resolutionX> <resolutionZ>
terrain_row <rowIndex> <h0> <h1> ... <hN>
terrain_material <textureIdentity|-> <tiling>
terrain_layer <layerIndex> <textureIdentity> <tiling>
terrain_paint <rowIndex> <q00> <q01> <q02> <q03> ... <qN0> <qN1> <qN2> <qN3>
terrain_weights <resolutionX> <resolutionZ>
terrain_weight <mapIndex> <rowIndex> <hex>
terrain_veg <resolutionX> <resolutionZ> <seed>
terrain_veg_entry <index> <density> <minScale> <maxScale> <yaw 0|1> <align 0|1> <modelIdentity...>
terrain_veg_row <rowIndex> <hex>
terrain_veg_style <hex>
```

At most one Terrain per Level. Omitted records mean the Level has no Terrain;
old Levels remain valid. The format version stays `1`. Terrain is a regular
XZ heightfield, not a repeatable prop category, GUID, or tile set.

`enabled` is an exact `0`/`1` bool. Origin is finite with each component in
`[-1024, 1024]`. Horizontal `sizeX` / `sizeZ` are finite in `[0.12, 256]`.
Resolution is an integer in `[2, 17]` on each axis. Heights are finite in
`[-256, 256]`. Sample count must be exactly `resolutionX * resolutionZ`.

Origin convention: `sample(0,0)` is the min-X / min-Z corner.

```
worldX = origin.x + ix * sizeX / (resolutionX - 1)
worldY = origin.y + heights[iz * resolutionX + ix]
worldZ = origin.z + iz * sizeZ / (resolutionZ - 1)
```

Heights are relative to Origin Y. World Y is up.

There is exactly one `terrain` header. Each `terrain_row` names a unique
`rowIndex` in `[0, resolutionZ)`. Row `rowIndex` contains exactly
`resolutionX` height samples in +X order. Duplicate headers, duplicate rows,
missing rows, extra samples, non-finite values, and out-of-range dimensions
are `Invalid`. `terrain_row` without a preceding `terrain` header is
`Invalid`.

Default editor Terrain (Add Terrain): enabled `1`, origin `{-8, 0.25, -4}`,
size `16 x 8`, resolution `9 x 5` (45 zero heights, 2-unit spacing). Canonical
`level_01` / `level_02` stay valid without these lines.

The writer emits `terrain` then `terrain_row 0..resolutionZ-1` after
`directional_light` and before `point_light`, omitted when `hasTerrain` is
false.

Milestone 87 sculpts those same height samples. Brush mode, operation, radius,
and strength are Development Editor tool state and are **not** Level Format
fields. Existing M86 Terrain files remain valid. The format version stays `1`.

Milestone 88 adds one optional base surface material. `terrain_material` is
omitted when the assignment is empty and tiling is the default `0.25`. When
present it is a singleton after the Terrain header: `textureIdentity` is the
portable M84 runtime PNG identity `textures/<file>.png`, or `-` for no
assignment. Tiling is finite in `[0.01, 16]` (repeats per world unit). Duplicate
`terrain_material`, missing Terrain header, absolute paths, non-PNG identities,
and non-finite/out-of-bounds tiling are `Invalid`. Old M86/M87 Terrain files
without this record remain valid and use empty identity plus default tiling
(solid fallback). The writer emits `terrain_material` after `terrain_row` when
the assignment or tiling is non-default. The format version stays `1`.

Milestone 90 adds optional extra Terrain material layers and per-sample painted
weights. Layer 0 remains `terrain_material` (M88). Extra layers are compact
`terrain_layer` records with `layerIndex` in `{1,2,3}` matching the next
assigned extra (`1` first, then `2`, then `3`). Each extra identity is
`textures/<file>.png` (never `-`, never a duplicate of layer 0 or another extra).
Tiling is finite in `[0.01, 16]`. Duplicate indices, gaps, missing Terrain
header, absolute paths, and invalid tiling are `Invalid`. Old M88 files without
these records remain valid: one base material, default weights.

Painted weights are independent of heights and tied to the same XZ sample rows.
`terrain_paint` is omitted when every sample is the implicit default (layer 0
weight 1, others 0). When any `terrain_paint` row is present, exactly
`resolutionZ` unique rows are required, each with `resolutionX` groups of four
integers. Each group is layer 0..3 quantized to `[0, 255]` and **must sum to
255**. Parser reconstructs floats as `q / 255` and renormalizes. Non-integer
tokens, out-of-range values, sums other than 255, duplicate rows, missing rows,
and extra tokens are `Invalid`. The writer emits `terrain_layer` after
`terrain_material` (when extras exist) and `terrain_paint` after that (when
weights are non-default). The format version stays `1`.

Milestone 92 keeps format version `1` and reads those M88/M90 records.
New authored output uses dedicated weight maps instead of `terrain_paint`:

```
terrain_weights <resolutionX> <resolutionZ>
terrain_weight <mapIndex> <rowIndex> <hex>
```

`terrain_layer` indices are contiguous starting at `1` and may exceed `3`,
up to the practical palette cap of 16 layers (15 extras). `terrain_weights`
is optional and singleton. Resolution on each axis is an integer in
`[2, 32]`. The default grid is `32 32` and is independent of Terrain
`resolutionX` / `resolutionZ`. The header is omitted when the resolution is
the default and every texel is implicit layer 0. It is written alone when
only the resolution is non-default.

`terrain_weight` rows are omitted when weights are the implicit default.
When any row is present, every packed map required by the palette is
required: map count is `ceil(layerCount / 4)`, and each map has exactly
`resolutionZ` unique rows. `mapIndex` is `[0, mapCount)`. `hex` is lowercase
and contains exactly `resolutionX * 8` digits. Each texel is eight digits,
`RRGGBBAA`, for layers `mapIndex*4+0..3`. Across every map, the channels
for assigned layers sum to 255 and channels at or above the layer count are
`00`. The parser reconstructs floats as `q/255` and renormalizes. Uppercase
hex, the wrong length, a sum other than 255, a non-zero unused channel,
duplicate rows, missing rows, and mixing `terrain_paint` with
`terrain_weights` / `terrain_weight` are `Invalid`. A `terrain_layer` after
weight records is `Invalid`.

Loading old `terrain_paint` bilinearly resamples those four per-geometry
weights onto the default 32×32 grid. Corners match the old corner samples.
Saving then emits `terrain_weight` and does not emit `terrain_paint`.
Unpainted M88 Terrain stays 100% layer 0 and omits weight records.

A fully painted 16-layer 32×32 map is 4×32 = 128 lines of hex (each line
under 512 characters). The 64 KiB / 256-line guards are unchanged; a Level
that cannot fit the weight rows is `Invalid` rather than a reason to raise
those guards. The writer emits `terrain_weights` then `terrain_weight` in
map-major, row-major order after `terrain_layer`.

Milestone 93 keeps format version `1`. Vegetation is omitted entirely when the
palette is empty. It is not a Static Prop and it does not store instance
transforms:

```
terrain_veg <resolutionX> <resolutionZ> <seed>
terrain_veg_entry <index> <density> <minScale> <maxScale> <yaw 0|1> <align 0|1> <modelIdentity...>
terrain_veg_row <rowIndex> <hex>
terrain_veg_style <hex>
```

`terrain_veg` is an optional singleton and requires `terrain`. Resolution on
each axis is an integer in `[4, 24]`. The default grid is `16 16` and is
independent of both the heightfield and the weight-map resolution. `seed` is
a `uint32`. At most 8 `terrain_veg_entry` records, indices contiguous from 0.
Density is in `[0.05, 8]`. Scales are in `[0.05, 8]` with `minScale <= maxScale`.
`yaw` and `align` are exact `0`/`1`. `modelIdentity` is the remainder of the
line and must be `models/<file>.glb`. Palette `density`, `minScale`,
`maxScale`, `yaw`, and `align` are Next-Paint values. Changing them does not
rewrite vegetation already painted.

`terrain_veg_row` hex starts with two lowercase digits per cell. That byte is
a bitmask: bit `i` set means palette entry `i` occupies that cell. Each set
bit is followed, in ascending entry index, by two more lowercase hex digits:
an 8-bit density quantum. Quantum 0 is density 0.05 and quantum 255 is
density 8, linearly. `00` with no density digits is empty. Empty occupancy
rows are omitted.

Paint also captures per occupied entry: Min Scale, Max Scale, Random Yaw, and
Align to Terrain Normal. Those four values are not appended to
`terrain_veg_row`. A full 24-cell row with eight occupied entries is already
432 occupancy/density hex digits (about 451 characters with the prefix), so
extra scale/yaw/align bytes would exceed 512. They are a concatenated stream
of 12-bit triples, 3 lowercase hex digits per occupied slot, in raster order
`(cellIndex, entryIndex)` matching the occupancy bits. Each triple packs
5-bit min scale, 5-bit max scale, Random Yaw, and Align Normal:

- scale range `[0.05, 8]`, 32 levels (`quantum 0..31`);
- step `7.95 / 31 ≈ 0.2565`;
- maximum quantization error `≈ 0.128`;
- bit 10 is Random Yaw, bit 11 is Align Normal.

The writer splits the stream into `terrain_veg_style` records of at most 492
hex digits (`164` triples) so each line stays under 512 characters
(`terrain_veg_style ` is 19 characters). Occupied-slot worst case is
`24×24×8 = 4608` triples → 29 style records. Together with 24 occupancy rows,
8 entries, and the vegetation header that is 62 vegetation lines. A maximum
16-layer weight map plus a 24×24 vegetation grid plus canonical authored
objects stays within 256 lines and 64 KiB. Derived Y, yaw, scale, and the
current Terrain normal are not written. Milestone 94 does not change these
records. Its derived XZ placement can differ from the Milestone 93 sampler;
Save still does not write generated transforms. Milestone 95 does not change
these records either. Thumbnail, picker, brush-hover, and other editor-only
Vegetation UI state is not Level data.

The first M93 writer used one nibble per cell, where `N` selected entry
`N - 1`. Correction 1 used exactly `resolutionX * 2` hex digits and no
density bytes. Correction 2 used occupancy plus density bytes and no style
records. All three still load. Missing spatial density or style is filled
from the palette entry that previously generated those instances. Save
writes occupancy/density rows plus style records only. A second header, a
row without the header, a bit above the palette, a density byte on an empty
entry, a style stream whose length does not match occupancy, or vegetation
without Terrain is `Invalid`. The writer emits `terrain_veg`, then entries,
then occupied rows, then style records after `terrain_weight`.

Milestone 96 keeps format version `1`. Ground cover is omitted entirely when
the palette is empty. It is not vegetation occupancy and it does not store
instance transforms:

```
terrain_cover <resolutionX> <resolutionZ> <seed>
terrain_cover_entry <index> <density> <minWidth> <maxWidth> <minHeight> <maxHeight> <textureIdentity>
terrain_cover_data <hex>
```

`terrain_cover` is an optional singleton and requires `terrain`. Resolution on
each axis is an integer in `[4, 8]`. The default grid is `8 8` and is
independent of the heightfield, weight-map, and vegetation resolutions.
`seed` is a `uint32`. At most 4 `terrain_cover_entry` records, indices
contiguous from 0. Density is in `[0.5, 48]`. Width is in `[0.05, 1.5]` and
height is in `[0.05, 2]` with min <= max. `textureIdentity` is the remainder
of the line and must be `textures/<file>.png`. Palette density/width/height
are Next-Paint values. Changing them does not rewrite ground cover already
painted. Duplicate texture identities are legal as separate slots. The
Development Ground Cover picker further requires useful PNG cutout alpha
(decoded 8-bit alpha below 0.5, or RGB `tRNS`); Level load still accepts
any valid runtime PNG identity.

`terrain_cover_data` is a concatenated lowercase hex stream of occupied
cells in raster order. Each cell starts with one occupancy nibble (bit `i`
set means palette entry `i` occupies that cell). Each set bit is followed,
in ascending entry index, by six hex digits: an 8-bit density quantum and a
16-bit packed min/max width/height (4 bits each). Empty occupancy is stored
as nibble `0` with no extra digits. A fully empty grid omits data records.
The writer splits the stream into `terrain_cover_data` records of at most
492 hex digits so each line stays under 512 characters
(`terrain_cover_data ` is 19 characters). An 8×8 grid with all four entries
occupied is 1600 digits → 4 data records, plus 1 header and 4 entries = 9
lines. Absolute worst-case Terrain (max heightfield + 16-layer weights +
max 8-entry vegetation) has only a few spare lines; a typical Level plus
full ground cover stays inside 256 lines / 64 KiB / 512 characters. A Level
that cannot fit is `Invalid`. Derived transforms are never written.

A second header, an entry/data record without the header, a bit above the
palette, a data stream whose length does not match occupancy, or ground
cover without Terrain is `Invalid`. The writer emits `terrain_cover`, then
entries, then data records after vegetation.

### Required repeated records

Encounter order of repeated records is the container order in `LevelDefinition`. Canonical Level 01 uses 6 / 2 / 2 / 2 / 3 / 0 / 0 / 0 / 0 / 0 / 0 (platforms / slopes / checkpoints / hazards / collectibles / Level Goals / Dynamic Boxes / Pressure Plates / Doors / Item Pickups / Static Props) and FOV 40. v1 does **not** require those instance counts. Checkpoint / hazard / collectible / Level Goal / Dynamic Box / Pressure Plate / Door / Item Pickup / Static Prop counts are 0 or more; the parser does **not** impose a small design cap. Shared defensive guards remain `kMaxLevelFileBytes` (64 KiB) and `kMaxLevelLines` (256). Platform count plus Dynamic Box count plus Door count is limited by the shared authored-body leftover (`kMaxAuthoredPhysicsBodies` = 59 = `kPhysicsMaxBodies` 65 minus 5 fixed bodies minus 1 reserved Terrain slot). Pressure Plates, Item Pickups, Static Props, and Level Goals are not Jolt bodies and do **not** consume that leftover. Optional Terrain uses the reserved static-body slot and also does not reduce the leftover of 59. Slopes remain exactly 2. A file may list zero `platform` records syntactically; semantic validation then fails because a valid/saveable level requires **at least one** platform (`kMinElevatedPlatformCount = 1`) so the three `support_index_*` values can be in range. Support indices must be 0-based and in range of the parsed platform list. M41 Add/Duplicate append platforms (existing indices stay valid). Platform delete remaps `R > D` to `R - 1` and rejects deleting a platform that any `support_index_*` still names. M45 Add/Duplicate/Delete Dynamic Boxes have no support-index remapping. M49 Add/Duplicate/Delete Static Props have no physics remapping. M52 Add/Duplicate/Delete Pressure Plates have no physics remapping. M53 Add/Duplicate append Doors (existing plate Door indices stay valid). Door delete clears links that named D and remaps later plate links `R > D` to `R - 1`. Invalid Door links fail validation; they are never silently retargeted. M55 Add/Duplicate/Delete Item Pickups have no physics remapping and never serialize runtime collected state. M63 Add/Duplicate/Delete Level Goals have no physics remapping and never serialize runtime completion.

```
platform <cx> <cy> <cz> <sx> <sy> <sz>
slope <cx> <cy> <cz> <sx> <sy> <sz> <rotZ> # exactly 2
checkpoint <cx> <cy> <cz> <sx> <sy> <sz> <rx> <ry> <rz>
hazard <cx> <cy> <cz> <sx> <sy> <sz>
collectible <cx> <cy> <cz> <sx> <sy> <sz>
level_goal <cx> <cy> <cz> <sx> <sy> <sz> [<nextLevelId>]
dynamic_box <cx> <cy> <cz> <sx> <sy> <sz> <massKg>
pressure_plate <cx> <cy> <cz> <sx> <sy> <sz> [<doorIndex> [<activateByDynamicBox> <activateByPlayer> <visibleInGameplay> [<controlsDirectionalLight> [lights <count> <kind> <index> ...]]]]
door <cx> <cy> <cz> <sx> <sy> <sz> <openDistance> [<requiredItem>]
item_pickup <px> <py> <pz> <quantity> <itemId> [visual <ox> <oy> <oz> <rx> <ry> <rz> <sx> <sy> <sz>] [bounds <0|1>] [highlight <intensity>] [gold <amount>] [idle <0|1> <bobAmplitude> <bobSpeed> <spinSpeedDegrees>] [<modelIdentity...>]
static_prop <px> <py> <pz> <rx> <ry> <rz> <sx> <sy> <sz> <identity...>
authoring_group <name> <kind> <index> <kind> <index> ...
```

`support_index_*` are 0-based indices into the `platform` array (M30
`checkpoint1PlatformIndex` / `checkpoint2PlatformIndex` / `goalPlatformIndex`).
They are authored validation metadata, not gameplay runtime state.

## Numbers

Locale-independent `std::from_chars`. Whole token must parse. Reject overflow,
NaN, Inf, leftover suffix (`1.0f`), and empty tokens. Integers for version and
support indices.

Positive sizes: each component finite and `> 0`. Dynamic Box, Pressure Plate, Door, and Level Goal extents must also
be `>= kMinDynamicBoxExtent` / `kMinPressurePlateExtent` / `kMinDoorExtent` / `kMinLevelGoalExtent` (0.12), matching the editor authored-box minimum
so Resize/parse share one floor. Level Goals are not Jolt bodies.
Moving platform: size positive, path min `<` path max, speed `> 0`, startX
inside `[pathMinX, pathMaxX]`.
Dynamic Box mass is kilograms: finite, `> 0`, and `<= kMaxDynamicBoxMassKg`
(10000). Reject 0, negative, NaN, ±Inf, and values above that safety maximum.
Do not store inverse mass. Zero, one, or many `dynamic_box` records are valid.
A historical one-record file parses as a one-element collection. Canonical
Level 01 intentionally has **zero** Dynamic Boxes (M45 removed the legacy
probe line; this is not the historical EOL artifact). Save serializes authored
`center`, never the live Jolt pose.

`level_goal` is a repeatable authored axis-aligned completion volume, not a
Jolt body. Position is world center. Size is extents; each axis finite and
`>= kMinLevelGoalExtent` (0.12). Default Add size is `2, 1.6, 1.8`. Zero,
one, or many records are valid. Any one active goal may complete the level.
Canonical Level 01 has **one** destination-bearing Level Goal (`nextLevelId = level_02`). Canonical Level 02 has **one** terminal Level Goal. Neither file uses the historical required singleton `goal` keyword.
Runtime `LevelCompletionState` is **not** a Level Format field and is never
serialized. The historical required singleton `goal` keyword is rejected.
Presentation: Development Editor draws the authored AABB as a translucent
volume; Gameplay and Release draw the two-post marker only. The invisible
AABB remains the completion region.

An optional 8th token is the destination identity (`nextLevelId`):
- omitted or empty → terminal goal (`RUN COMPLETE` / Enter Play Again from staged `level_01`, R Restart Current Level, Esc Main Menu);
- a valid `id` token such as `level_02` → destination-bearing goal (`LEVEL COMPLETE` / Enter Continue after the 1.75 s hold).

The writer emits the token only when non-empty. Unsafe identities (absolute
paths, traversal, slashes, dots, extensions) are rejected. Runtime resolution
uses staged `levels/<id>.level` only. A missing or malformed destination fails
atomically: the current level stays loaded and completed, and the failed
attempt is not retried every frame.

`pressure_plate` is an authored axis-aligned trigger volume, not a Jolt body.
Position is world center. Size is extents; each axis finite and
`>= kMinPressurePlateExtent` (0.12). Default Add size is `2, 0.2, 2`. Zero,
one, or many records are valid. Canonical Level 01 has **zero** Pressure
Plates. Runtime Active/Inactive is derived from overlap and is **not** a
Level Format field.

A 7-token `pressure_plate` is a no-link plate (`linkedDoorIndex = -1`) with
legacy M52/M53 modes: `activateByDynamicBox = true`, `activateByPlayer =
false`, `visibleInGameplay = true`, and `controlsDirectionalLight = false`.
An 8-token record adds the authored Door
index into `LevelDefinition.doors` (0-based). `-1` is explicit no-link.
Omitted mode flags keep those legacy defaults. An 11-token record adds three
strict `0`/`1` flags: Dynamic Box activation, Player activation, gameplay
visibility. A 12-token record adds `controlsDirectionalLight` (`0`/`1`) so
the plate may activate the singleton Level Directional Light. The field is
independent of `linkedDoorIndex`; one plate may control its Door and the
Directional Light together. Invalid flag tokens are rejected. The writer
always emits 12 positional tokens. An optional `lights <count> <kind>
<index> ...` suffix names zero or more Point/Spot targets (`kind` is exact
`point` or `spot`; indices are 0-based into those collections). The writer
emits the suffix only when the target list is non-empty. Duplicate identical
`{kind,index}` pairs, unknown kinds, non-integer indices, the wrong token
count, `lights 0`, and out-of-range indices are rejected. Old 7 / 8 / 11 /
12-token records remain valid and mean an empty target list. Both activation
sources may be true (OR) or both false (never Active). `visibleInGameplay = 0`
suppresses the Gameplay fill; Editor authoring still draws/selects the plate.
The Door index is **not** a BodyID, pointer, or GUID. Out-of-range and
non-integer links fail validation; they do not silently retarget another Door.
Cardinality is one Pressure Plate → zero or one Door, plus an optional
singleton Directional Light flag, plus zero or more typed local-light
targets. Multiple plates may name the same Door. Multiple plates may
control the same Directional Light (OR). Multiple plates may target the same
Point or Spot Light (OR). One plate cannot name multiple Doors. Local-light
target kinds are separate index namespaces; deleting a Point remaps only
Point targets.

`door` is a repeatable authored solid. Position is the **closed** world center.
Size is extents; each axis finite and `>= kMinDoorExtent` (0.12). Open
distance is finite and in `[kMinDoorOpenDistance, kMaxDoorOpenDistance]`
(`[0.12, 20]`). Default Add size is `1.2, 3.0, 2.4` and default open distance
is `3.2`. Opening direction is fixed **+Y**. An optional 9th token is the
Inventory requirement:

- omitted or `0` → no required item;
- `1` → required item `key` (M57 compatibility);
- a valid M54 `itemId` (`key`, `card`, `red_key`, …) → that item.

`redKey` is rejected because `gameplay::IsValidItemId` requires lowercase.
The writer emits `0` when empty and the canonical `itemId` when set (so a
parsed `1` is saved as `key`). Do not author a Pickup index, lock id, or
channel. Zero, one, or many records are valid. Canonical Level 01 has
**zero** Doors. Runtime open fraction, desiredOpen, lock unlocked flags,
obstruction, and live kinematic pose are **not** Level Format fields. Save
writes the authored closed pose and required item token only.

`item_pickup` is a repeatable authored world acquisition volume. Position is
the world **gameplay** pickup location (interaction, facing, LOS, placement,
Duplicate +1 X). `quantity` is a positive integer in the M54 Inventory range
(`1..kMaxItemQuantity`). `itemId` uses the production M54 `IsValidItemId`
rule (not a second parser-local grammar). An optional `visual` segment holds
per-instance presentation: offset, Euler XYZ degrees, and visual scale.
Omitted `visual` (legacy M55 records) means offset/rotation `0,0,0` and scale
`1,1,1`. Visual scale is finite and `> 0` on every axis — the same rule as
Static Prop scale, including the Scale-gizmo floor `kMinStaticPropScale` 0.01.
Visual fields never move the gameplay pickup point. An optional `bounds <0|1>`
marker is presentation only: `1` (default) draws the Gameplay target
interaction-bounds wire when this pickup is the current M55 target; `0`
suppresses that wire without changing targeting, HUD, or collection. The
marker sits after `visual` (or after `itemId` on lines that omit `visual`) and
before `modelIdentity`, so identities that contain spaces are not truncated.
Omitted `bounds` (legacy M55/M58 records) means `showInteractionBounds = true`.
The boolean token accepts exact `0` or `1` only. An optional `highlight <intensity>`
marker is presentation only: a finite float in `[0, 1]` that scales the Gameplay
target-highlight pass opacity (`round(255 * intensity)`). Omitted `highlight`
(legacy M55/M58/M58.3 records) means `targetHighlightIntensity = 0.70`.
Out-of-range, NaN, Inf, and malformed floats are rejected; they are not clamped.
The marker sits after `bounds` (or after `visual` / `itemId` when those earlier
optional markers are omitted) and before `gold` / `idle` / `modelIdentity`.
An optional `gold <amount>` marker is presentation only: a finite float in
`[0, 1]` that pushes the extra target-highlight pass toward the existing gold
`RGB(255, 220, 72)` (0 = white / original albedo, 1 = full gold). It is **not**
a second alpha. Omitted `gold` (legacy M55–M58.4 records) means
`targetHighlightGoldAmount = 0.70`. An optional `idle <0|1> <bobAmplitude>
<bobSpeed> <spinSpeedDegrees>` marker is presentation only. The bool is exact
`0`/`1`. Amplitude is finite `[0, 2]`, bob speed finite `[0, 10]` (cycles per
second), spin speed finite `[-720, 720]` degrees per second. Omitted `idle`
means disabled (`0 0.15 1 45`). Runtime bob/spin phase is **not** a Level Format
field. Optional trailing tokens after `idle` (or after `gold` / `highlight` /
`bounds` / `visual` / `itemId` on legacy lines) are a canonical Static
Model identity (`models/<file>.glb`), reassembled with spaces like
`static_prop`. The writer always emits the `visual` marker, nine floats, the
`bounds` marker, `0`/`1`, the `highlight` marker and intensity, the `gold`
marker and amount, the `idle` marker and four values, then the identity when
non-empty. Omitted identity means the primitive
fallback visual; a GLB is **not** required to author a valid pickup. Zero,
one, or many records are valid. Canonical Level 01 has **zero** Item Pickups.
Runtime available/collected flags and idle animation phase are **not** Level
Format fields. Save writes authored gameplay position, quantity, itemId, visual
transform, interaction-bounds visibility, target-highlight intensity, gold
amount, idle presentation settings, and optional model identity only.

`static_prop` is a visual authored instance, not a physics body. Identity is
the canonical project-relative Static Model Asset path (`models/<file>.glb`),
always last on the line so remaining tokens join with spaces if a filename
ever contains spaces (current `TryParseStaticModelIdentity` still rejects
unsafe names). Position is world XYZ. Rotation is Euler XYZ degrees (Rx then
Ry then Rz, matching renderer `rlRotate`). Scale is visual model scale, finite
and `> 0` on every axis — not primitive `size`. Authored Scale `(1,1,1)` keeps
the size produced by the runtime loader after glTF node transforms are baked;
the engine does not normalize imported models to a unit cube. Parse/Apply validate grammar,
finite transforms, positive scale, and identity safety. They do **not** require
the GLB to exist on disk. Missing staged runtime files draw a fallback cube.
Zero, one, or many `static_prop` records are valid. Canonical Level 01 has
**zero** Static Props.

`authoring_group` is optional authored organizational metadata for the
Development editor. It is **not** a gameplay entity, Prefab, transform parent,
or GUID. Zero, one, or many records are valid. Omitted records (legacy files)
mean an empty `LevelDefinition.authoringGroups` collection. Gameplay, physics,
and Release ignore the collection.

```
authoring_group <name> <kind> <index> <kind> <index> ...
```

`name` uses the same identifier grammar as `id`: `[A-Za-z_][A-Za-z0-9_]*`.
Names are unique within a Level. Default created names are `Group_01`,
`Group_02`, … Copied groups use `<name>_Copy`, then `<name>_Copy_2`.
Each group has at least two members. `<kind>` is a v1 object keyword
(`platform`, `static_prop`, `item_pickup`, `point_light`, `spot_light`, …).
`<index>` is a 0-based unsigned index into that authored collection. The first
member is the preferred PRIMARY when the editor reconstructs an M78
multi-selection. One authored object may belong to at most one group.
Overlapping membership, a one-member group, an unknown kind, or an
out-of-range index is `Invalid`. `environment`, `directional_light`, and
`terrain` are **not** valid member kinds. The writer emits groups after `static_prop` and
before `camera`.

Camera FOV finite, `> 0` and `< 180` (same range as M30).

## Not in the file

Runtime: active checkpoint, respawn position, death count, collected flags,
completion, TIME, BEST, moving-platform pose/direction, BodyIDs,
camera smoothed target, Dynamic Box runtime pose/velocity, Dynamic Box
kill-plane recovery, Pressure Plate Active/Inactive, overlapping Dynamic Box
index, overlap count, Door open fraction, desiredOpen, obstruction, and live
Door kinematic pose. Recovery uses the existing authored `kill_plane` and does
not add a Dynamic Box field. Pressure Plate activation and Door desired-open
are recomputed from current runtime overlap and are never written.

Player/controller policy: accel/decel/speed/gravity/jump/coyote/buffer,
CharacterVirtual max slope and shape, `kPlayerVisualSize`, inner-body settings.

Camera follow policy: dead zone X/Y, follow sharpness.

`LevelFileTest` asserts this by whitelist: every keyword the writer emits must
be one of the 26 v1 keywords, so no runtime state can appear in output.

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
platform            variable, elevatedPlatforms index order
support_index_cp1
support_index_cp2
support_index_goal
slope               x2, slopes index order
moving_platform
checkpoint          variable, checkpoints index order
hazard              variable, hazards index order
collectible         variable, collectibles index order
level_goal          variable, levelGoals index order
dynamic_box         variable, dynamicBoxes index order
pressure_plate      variable, pressurePlates index order
door                variable, doors index order
item_pickup         variable, itemPickups index order
static_prop         variable, staticProps index order
authoring_group     variable, authoringGroups index order
camera
environment
directional_light
terrain             omitted when absent
terrain_row         resolutionZ rows, row index order
terrain_material    omitted when layer 0 is the solid fallback at default tiling
terrain_layer       extra layers, index order
terrain_weights     omitted when the weight grid is the implicit default
terrain_weight      non-empty packed rows, map-major then row-major
terrain_veg         omitted when the vegetation palette is empty
terrain_veg_entry   palette index order
terrain_veg_row     occupied rows only, row index order
terrain_veg_style   occupied-slot style stream, 492 hex digits per record
terrain_cover       omitted when the ground-cover palette is empty
terrain_cover_entry palette index order
terrain_cover_data  occupied-slot density/style stream, 492 hex digits per record
point_light         variable, pointLights index order
spot_light          variable, spotLights index order
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
