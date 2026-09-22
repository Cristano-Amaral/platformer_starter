# Milestone 93 --- Terrain Vegetation Foundation

**Status:** IMPLEMENTED — awaiting manual acceptance\
**Branch:** `milestone/93-terrain-vegetation-foundation`

## 1. Objective

Introduce the first authored vegetation workflow for Terrain so level
authors can select a static `.glb` model and paint/remove vegetation
instances over Terrain regions.

M93 establishes the durable foundation for later foliage/vegetation
improvements without introducing fauna, gameplay AI, generalized entity
systems, or unrelated rendering architecture.

The milestone must preserve the existing authored-state lifecycle:

`workingCopy → Apply → active → Save`

Runtime physics/transient state must not silently become authored
vegetation data.

## 2. User-facing result

The editor gains a Terrain vegetation authoring mode in which the user
can choose a vegetation model from the existing static-model/content
catalog, configure basic placement parameters, paint and erase
vegetation over Terrain, preview pending `workingCopy` edits, Apply/Save
them, reload the level with identical authored vegetation, and Cook &
Stage the required runtime assets for Development/Release.

Vegetation must follow the Terrain surface rather than being authored as
arbitrary free-floating Static Props.

## 3. Scope

### 3.1 Vegetation palette

Add an authored Terrain vegetation palette. Each entry references a
static model through the repository's existing static-model
identity/catalog conventions; do not create a parallel asset identity
system.

Each palette entry supports, at minimum: - model identity; - density; -
minimum uniform scale; - maximum uniform scale; - random yaw
enabled/disabled; - align-to-terrain-normal enabled/disabled.

Use bounded, validated values consistent with repository conventions.
The editor supports adding/removing entries and selecting the active
entry. Removing an entry must safely remove/remap its placements and
never leave dangling references.

### 3.2 Terrain vegetation brush

Add a vegetation paint mode associated with Terrain with brush radius,
Paint, Erase, and active vegetation palette entry.

Painting places vegetation only where the brush intersects Terrain.
Erasing removes authored vegetation within the brush influence. Brush
behavior and randomized results must be deterministic for the same
authored data/operation inputs.

Avoid producing new placements continuously while the mouse is
stationary every frame. Follow the spacing/stroke principles already
established by Terrain sculpt/material painting where applicable.

### 3.3 Placement behavior

Generated authored instances derive world position from the Terrain
surface. X/Z identify Terrain placement; Y is derived from the current
Terrain surface. Yaw and uniform scale follow palette settings; optional
terrain-normal alignment follows its setting.

If Terrain heights are sculpted after vegetation exists, vegetation must
continue to sit on the current Terrain surface rather than preserving
stale world-space Y.

M93 does not require vegetation collision or gameplay interaction.

### 3.4 Determinism

Randomized yaw/scale/distribution must be reproducible after reload. Use
a deterministic authored seed or another deterministic representation
suitable for the current architecture. Do not introduce GUIDs.

### 3.5 Rendering

Vegetation is rendered as repeated instances of existing static `.glb`
models. Reuse existing model resources/catalog/loading infrastructure
and avoid a heavyweight asset load per placement.

Use batching/instancing where supported by the current renderer and
repository architecture. If the raylib/OpenGL path imposes a practical
constraint, document it and implement the most scalable representation
fitting the current renderer without replacing it.

Vegetation participates correctly in the existing world-lighting path to
the extent supported by reused static-model rendering. Do not add a new
PBR system.

### 3.6 Editor preview and lifecycle

Follow existing editor authority: - brush edits modify `workingCopy`; -
pending vegetation is visible as editor preview; - Apply
validates/promotes to `active`; - Save persists authored state; -
abandoning/reloading unapplied edits follows existing editor behavior; -
vegetation edits mark the level dirty consistently.

### 3.7 Level Format v1

Keep `PLATFORMER_LEVEL 1`.

Extend Level Format v1 with compact textual records for vegetation
palette definitions and deterministic placement data, with round-trip
serialization and validation while preserving 64 KiB, 256 lines, and 512
characters/line.

Choose a compact format deliberately. A naive one-line-per-instance
representation is not acceptable if normal painted vegetation becomes
impractical under Level Format v1 limits.

Existing levels without vegetation remain valid and unchanged in
behavior. Save emits deterministic/canonical ordering.

### 3.8 Cook/stage and dependencies

Vegetation model dependencies must be discovered by the existing
cook/stage pipeline. A `.glb` referenced only by Terrain vegetation must
still be included in required staged runtime assets.

Development may use only fallbacks already allowed by repository policy.
Release must work from staged runtime assets according to the current
Release contract.

### 3.9 Content Browser integration

Reuse `StaticModelCatalog` and existing Content Browser model
identities. Do not build a second model browser. Provide a practical
model selector, reusing catalog/thumbnail UI where reasonable without
redesigning Content Browser.

Extend the physical asset delete/reference guard so a model referenced
by vegetation is treated as in use.

## 4. Data and architecture requirements

Prefer vegetation-specific authored data owned by Terrain/level
authoring rather than converting painted vegetation into ordinary Static
Props.

Clearly separate palette configuration, authored placement/distribution
data, derived world transforms, and GPU/render resources.

Do not serialize GPU handles/runtime model objects. Do not make Terrain
geometry resolution the vegetation placement resolution. Do not couple
vegetation data to M92 weight-map resolution without a demonstrated
architectural reason.

## 5. Validation

Reject malformed/unsafe vegetation data using existing patterns.
Validate model identities, palette references, density bounds, scale
ranges, brush parameters, placement data bounds, serialized
sizes/counts, and Level Format hard limits.

Malformed vegetation records must fail safely and consistently with
current Level Format behavior.

## 6. Automated regression coverage

Cover at least: 1. levels without vegetation remain compatible; 2.
palette parse/write round trip; 3. deterministic Save output; 4.
deterministic placement/randomization after reload; 5. Paint adds
vegetation only on Terrain; 6. Erase removes vegetation in the affected
region; 7. vegetation follows Terrain after sculpting; 8. scale remains
within min/max; 9. random-yaw disabled behavior; 10. normal-alignment
enabled/disabled behavior; 11. palette removal leaves no dangling
references; 12. vegetation-only model dependency is discovered by
cook/stage; 13. asset delete guard recognizes vegetation references; 14.
enough instances to exercise the scalable rendering/data path; 15. Level
Format hard limits remain preserved; 16. automated tests do not modify
`level_01.level` or `level_02.level`.

Use existing tests/frameworks where appropriate.

## 7. Required validation

Follow `AGENTS.md` and `DEVELOPMENT_WORKFLOW.md`, including:

``` text
cmake --preset windows-vs2022
cmake --build --preset windows-debug
cmake --build --preset windows-development
cmake --build --preset windows-release

python tools/test_milestone_docs.py
python tools/test_agent_instructions.py
python tools/test_stage_runtime_assets.py
python tools/test_cook_level_v1.py
python tools/test_cook_runtime_png.py
python tools/test_import_static_glb.py
python tools/test_stage_world_shaders.py
```

Run all relevant affected C++ regression executables.

Before reporting:

``` text
git diff --check
git diff -- game/assets/source/levels/level_01.level
git diff -- game/assets/source/levels/level_02.level
git diff --stat
```

Canonical level diffs must be empty unless the user explicitly amends
the milestone definition.

## 8. Manual acceptance targets

Demonstrate: - add/select a vegetation `.glb`; - paint a visible Terrain
region; - density variation behaves sensibly; - randomized scale stays
within bounds; - randomized yaw can be enabled/disabled; - erase removes
vegetation; - vegetation follows sculpted Terrain; - Apply/Save/reload
and application restart preserve the result; - Cook & Stage plus the
appropriate Release build renders vegetation correctly in Release; - a
model used only by vegetation is protected by the delete guard; -
existing M92 Terrain materials/painting remain correct.

Automated green status does not replace manual acceptance.

## 9. Explicit non-goals

M93 does not include grass-blade/procedural-grass shaders, wind
animation, seasonal variation, a generalized LOD authoring system,
billboard/impostor generation, an occlusion-culling framework,
fauna/NPCs/AI, gameplay interaction, destructible vegetation,
harvesting, vegetation physics/collision, navmesh/pathfinding, Player
Path/traversal, PBR overhaul, triplanar Terrain materials, generalized
prefab/entity/GUID systems, undo/redo, or Content Browser redesign.

These belong to separate future milestones.

## 10. Completion rule

The coding agent implements M93 on the milestone branch, validates it,
reports exact changes/tests/results/known limitations, and **STOPS**.

The coding agent must not commit, push, merge, close M93, or start M94.

M93 becomes CLOSED only after automated validation, ChatGPT review, user
manual acceptance and explicit approval, Git closure per
`DEVELOPMENT_WORKFLOW.md`, and confirmation that `main` is clean and
synchronized with `origin/main`.
