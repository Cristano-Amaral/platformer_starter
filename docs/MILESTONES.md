# Milestones

`docs/MILESTONES.md` is a compact index and compatibility entry point.
Canonical milestone definitions live **one file per milestone** under
`docs/milestones/`.

Do not load this index as a substitute for the active milestone file,
and do not load every historical milestone by default.

## Canonical directory

`docs/milestones/` (lowercase). Filename prefix is uppercase `MILESTONE_`.

## Filename convention

- Integer: Milestone 29 → [`docs/milestones/MILESTONE_29.md`](milestones/MILESTONE_29.md)
- Decimal: Milestone 58.4 → [`docs/milestones/MILESTONE_58_4.md`](milestones/MILESTONE_58_4.md)
- Leading zeros in historical headings (`Milestone 00`, `Milestone 01`) map to
  [`MILESTONE_0.md`](milestones/MILESTONE_0.md) and [`MILESTONE_1.md`](milestones/MILESTONE_1.md);
  the original heading text is preserved inside those files.

Do not create dotted filename alternatives such as `MILESTONE_58.4.md`.

## Current milestone

**Milestone 85.2 — Directional Light Gameplay Activation & Authoring Polish**

Canonical file: [`docs/milestones/MILESTONE_85_2.md`](milestones/MILESTONE_85_2.md)

Current architecture: [`docs/ARCHITECTURE.md`](ARCHITECTURE.md)

Milestone 73 is CLOSED.
Milestone 74 is CLOSED.
Milestone 75 is CLOSED.
Milestone 76 is CLOSED.
Milestone 77 is CLOSED.
Milestone 78 is CLOSED.
Milestone 79 is CLOSED.
Milestone 80 is CLOSED.
Milestone 81 is CLOSED.
Milestone 82 is CLOSED.
Milestone 83 is CLOSED.
Milestone 84 is CLOSED.
Milestone 85 is CLOSED.
Milestone 85.1 is CLOSED.
Do not start Milestone 86.

## Cursor loading contract

1. The user/prompt identifies the active milestone number or file.
2. Cursor reads `docs/milestones/MILESTONE_<N>.md`.
3. Cursor inspects current repository code, tests, and docs relevant to that milestone.
4. Other milestone files are read only when needed for a dependency, historical decision, or ambiguity.
5. Cursor does not automatically load every historical milestone.

Future milestone definitions are added directly as `docs/milestones/MILESTONE_<N>.md`.
This index then receives a small navigation/status entry.

## Source of truth

1. current repository code;
2. tests;
3. current architecture/docs;
4. active milestone file;
5. latest relevant checkpoint/current documentation;
6. older milestone files/history.

Historical milestone files do not override current implemented behavior.

## Index

| Milestone | File | Status |
| --- | --- | --- |
| 0 | [Milestone 0 — Repository/Foundation Setup](milestones/MILESTONE_0.md) | complete |
| 1 | [Milestone 1 — Window and Game Loop](milestones/MILESTONE_1.md) | complete |
| 2 | [Milestone 2 — 3D Greybox and Platformer Camera](milestones/MILESTONE_2.md) | complete |
| 3 | [Milestone 3 — Semantic Input and Player Movement](milestones/MILESTONE_3.md) | complete |
| 4 | [Milestone 4 — Vertical Motion Foundation](milestones/MILESTONE_4.md) | complete |
| 5 | [Milestone 5 — Static Platform Collision](milestones/MILESTONE_5.md) | complete |
| 6 | [Milestone 6 — Solid Static AABB Collision](milestones/MILESTONE_6.md) | complete |
| 7 | [Milestone 7 — Player Movement Feel](milestones/MILESTONE_7.md) | complete |
| 8 | [Milestone 8 — Platformer Camera Follow](milestones/MILESTONE_8.md) | complete |
| 9 | [Milestone 9 — Debug/Development Metrics](milestones/MILESTONE_9.md) | complete |
| 10 | [Milestone 10 — Experimental Jolt Physics Integration](milestones/MILESTONE_10.md) | complete |
| 11 | [Milestone 11 — Player CharacterVirtual](milestones/MILESTONE_11.md) | complete |
| 12 | [Milestone 12 — Physics Cleanup and Consolidation](milestones/MILESTONE_12.md) | complete |
| 13 | [Milestone 13 — Jolt Moving Platform](milestones/MILESTONE_13.md) | complete |
| 14 | [Milestone 14 — Slopes and CharacterVirtual Ground Handling](milestones/MILESTONE_14.md) | complete |
| 15 | [Milestone 15 — Asset Pipeline Foundation](milestones/MILESTONE_15.md) | complete |
| 16 | [Milestone 16 — First Static 3D Model Asset Pipeline](milestones/MILESTONE_16.md) | complete |
| 17 | [Milestone 17 — Blender Authored Asset Workflow](milestones/MILESTONE_17.md) | complete |
| 18 | [Milestone 18 — Material + Embedded Texture Asset Workflow](milestones/MILESTONE_18.md) | complete |
| 19 | [Milestone 19 — Asset Cooker Texture Optimization Foundation](milestones/MILESTONE_19.md) | complete |
| 20 | [Milestone 20 — Checkpoint + Fall/Respawn Loop](milestones/MILESTONE_20.md) | complete |
| 21 | [Milestone 21 — Level Goal + Completion Loop](milestones/MILESTONE_21.md) | complete |
| 22 | [Milestone 22 — Level Restart + Run-State Reset](milestones/MILESTONE_22.md) | complete |
| 23 | [Milestone 23 — Dynamic Body Interaction Safety](milestones/MILESTONE_23.md) | complete |
| 24 | [Milestone 24 — Extended Traversal + Multiple Checkpoints](milestones/MILESTONE_24.md) | complete |
| 25 | [Milestone 25 — Static Hazards + Hazard Respawn](milestones/MILESTONE_25.md) | complete |
| 26 | [Milestone 26 — Collectibles + Run Counter](milestones/MILESTONE_26.md) | complete |
| 27 | [Milestone 27 — Run Timer + Completion Time](milestones/MILESTONE_27.md) | complete |
| 28 | [Milestone 28 — Best Time (Session Record)](milestones/MILESTONE_28.md) | complete |
| 29 | [Milestone 29 — Persistent Best Time (Save File v1)](milestones/MILESTONE_29.md) | complete |
| 30 | [Milestone 30 — Level Data v1: Data-Driven Single Level](milestones/MILESTONE_30.md) | complete |
| 31 | [Milestone 31 — External Level File v1](milestones/MILESTONE_31.md) | complete |
| 32 | [Milestone 32 — Development Level Editor v1](milestones/MILESTONE_32.md) | complete |
| 33 | [Milestone 33 — Visual Level Editor v2: Viewport Navigation, Hierarchy & World Selection](milestones/MILESTONE_33.md) | complete |
| 34 | [Milestone 34 — Visual Level Editor v3: Transform Gizmo + Persistent Editor Layout](milestones/MILESTONE_34.md) | complete |
| 35 | [Milestone 35 — Visual Level Editor v4: Scale/Resize Gizmo + Orientation Widget + Precision Navigation](milestones/MILESTONE_35.md) | complete |
| 36 | [Milestone 36 — Editor Menu Bar & Workspace Controls](milestones/MILESTONE_36.md) | complete |
| 37 | [Milestone 37 — Editor Tool Runner: Asset Cooker & Build Integration](milestones/MILESTONE_37.md) | complete |
| 38 | [Milestone 38 — Runtime Asset Staging & Cook-and-Stage Workflow](milestones/MILESTONE_38.md) | complete |
| 39 | [Milestone 39 — Development Runtime Level Reload](milestones/MILESTONE_39.md) | complete |
| 40 | [Milestone 40 — Cook, Stage & Reload Workflow](milestones/MILESTONE_40.md) | complete |
| 41 | [Milestone 41 — Authored Object Add / Delete / Duplicate](milestones/MILESTONE_41.md) | complete |
| 42 | [Milestone 42 — Object Palette & Placement Workflow](milestones/MILESTONE_42.md) | complete |
| 43 | [Milestone 43 — Editor Quick Toolbar](milestones/MILESTONE_43.md) | complete |
| 44 | [Milestone 44 — Legacy Prototype Scene Cleanup](milestones/MILESTONE_44.md) | complete |
| 45 | [Milestone 45 — Authored Dynamic Physics Objects](milestones/MILESTONE_45.md) | complete |
| 46 | [Milestone 46 — Dynamic Box Runtime Recovery](milestones/MILESTONE_46.md) | complete |
| 47 | [Milestone 47 — Static GLB Asset Import & Registry](milestones/MILESTONE_47.md) | complete |
| 48 | [Milestone 48 — Content Browser / Asset Browser v1](milestones/MILESTONE_48.md) | complete |
| 48.1 | [Milestone 48.1 — Content Browser Thumbnails](milestones/MILESTONE_48_1.md) | complete |
| 48.2 | [Milestone 48.2 — Interactive Static Model Preview](milestones/MILESTONE_48_2.md) | complete |
| 49 | [Milestone 49 — Authored Static Props](milestones/MILESTONE_49.md) | complete |
| 50 | [Milestone 50 — Static Prop Placement Workflow](milestones/MILESTONE_50.md) | complete |
| 51 | [Milestone 51 — Dynamic Box Grab / Carry](milestones/MILESTONE_51.md) | complete |
| 52 | [Milestone 52 — Pressure Plate / Dynamic Box Trigger](milestones/MILESTONE_52.md) | complete |
| 53 | [Milestone 53 — Authored Door & Pressure Plate Link](milestones/MILESTONE_53.md) | complete |
| 54 | [Milestone 54 — Player Inventory System v1](milestones/MILESTONE_54.md) | complete |
| 55 | [Milestone 55 — World Item Pickup & Inventory Integration](milestones/MILESTONE_55.md) | complete |
| 56 | [Milestone 56 — Player Inventory UI v1](milestones/MILESTONE_56.md) | complete |
| 57 | [Milestone 57 — Key Item & Locked Door Interaction](milestones/MILESTONE_57.md) | complete |
| 57.1 | [Milestone 57.1 — Specific Door Item Requirement & Pressure Plate Modes](milestones/MILESTONE_57_1.md) | complete |
| 58 | [Milestone 58 — Item Pickup Visual Transform & Asset Presentation](milestones/MILESTONE_58.md) | complete |
| 58.1 | [Milestone 58.1 — Editor Rotate Gizmo v1](milestones/MILESTONE_58_1.md) | complete |
| 58.2 | [Milestone 58.2 — Model-backed Selection & Transform Preview](milestones/MILESTONE_58_2.md) | complete |
| 58.3 | [Milestone 58.3 — Item Pickup Gameplay Target Highlight](milestones/MILESTONE_58_3.md) | complete |
| 58.4 | [Milestone 58.4 — Item Pickup Target Highlight Intensity](milestones/MILESTONE_58_4.md) | complete |
| 59 | [Milestone 59 — Item Pickup Presentation Polish](milestones/MILESTONE_59.md) | implemented, awaiting manual acceptance |
| 60 | [Milestone 60 — Split Milestone Documentation](milestones/MILESTONE_60.md) | implemented, awaiting manual acceptance |
| 61 | [Milestone 61 — Item Pickup Collection Feedback](milestones/MILESTONE_61.md) | complete |
| 62 | [Milestone 62 — Item Pickup Collection HUD Notification](milestones/MILESTONE_62.md) | implemented, awaiting manual acceptance |
| 63 | [Milestone 63 — Level Goal / Exit](milestones/MILESTONE_63.md) | complete |
| 64 | [Milestone 64 — Multiple Levels & Level Transition](milestones/MILESTONE_64.md) | complete |
| 64.1 | [Milestone 64.1 — Editor Level Browser & Transition UX](milestones/MILESTONE_64_1.md) | complete |
| 65 | [Milestone 65 — Game Flow & Run Completion](milestones/MILESTONE_65.md) | complete |
| 66 | [Milestone 66 — Main Menu & Play Flow](milestones/MILESTONE_66.md) | complete |
| 67 | [Milestone 67 — Gameplay HUD & Objective Presentation](milestones/MILESTONE_67.md) | complete |
| 68 | [Milestone 68 — Pause Menu & Runtime Navigation](milestones/MILESTONE_68.md) | complete |
| 69 | [Milestone 69 — Player Health & Damage](milestones/MILESTONE_69.md) | complete |
| 70 | [Milestone 70 — Death, Damage Feedback & Respawn](milestones/MILESTONE_70.md) | complete |
| 71 | [Milestone 71 — Gameplay Audio Foundation](milestones/MILESTONE_71.md) | complete |
| 72 | [Milestone 72 — Player Movement Audio Feedback](milestones/MILESTONE_72.md) | complete |
| 73 | [Milestone 73 — World Interaction Audio Feedback](milestones/MILESTONE_73.md) | complete |
| 74 | [Milestone 74 — Gameplay / Level Polish Pass](milestones/MILESTONE_74.md) | complete |
| 75 | [Milestone 75 — UI Audio & Menu Feedback](milestones/MILESTONE_75.md) | complete |
| 76 | [Milestone 76 — Transform Snapping & Authoring Productivity](milestones/MILESTONE_76.md) | complete |
| 77 | [Milestone 77 — Viewport Grid & Spatial Authoring Guides](milestones/MILESTONE_77.md) | complete |
| 78 | [Milestone 78 — Multi-Selection & Group Translate](milestones/MILESTONE_78.md) | complete |
| 79 | [Milestone 79 — Multi-Selection Lifecycle Operations](milestones/MILESTONE_79.md) | complete |
| 80 | [Milestone 80 — Multi-Selection Group Rotate](milestones/MILESTONE_80.md) | complete |
| 81 | [Milestone 81 — Authoring Groups Foundation](milestones/MILESTONE_81.md) | complete |
| 82 | [Milestone 82 — Hierarchy & Group Workflow Polish](milestones/MILESTONE_82.md) | complete |
| 83 | [Milestone 83 — 3D Player Character Foundation](milestones/MILESTONE_83.md) | complete |
| 84 | [Milestone 84 — Materials & Textures Foundation](milestones/MILESTONE_84.md) | complete |
| 85 | [Milestone 85 — Lighting & Shadows Foundation](milestones/MILESTONE_85.md) | complete |
| 85.1 | [Milestone 85.1 — Environment & Directional Light Authoring](milestones/MILESTONE_85_1.md) | complete |
| 85.2 | [Milestone 85.2 — Directional Light Gameplay Activation & Authoring Polish](milestones/MILESTONE_85_2.md) | implemented, awaiting manual acceptance |

## Later milestones

Animation, enemies, collectibles, level editor, audio, save system,
profiling/optimization, Raspberry Pi validation, Android port, iOS
feasibility/backend work.
