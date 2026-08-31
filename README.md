# Platformer3D

Starter repository for a C++ 3D platformer developed with Cursor.

## Windows bootstrap
Requirements:
- Cursor
- Visual Studio 2022 with Desktop development with C++
- CMake 3.25+
- Git
- Python 3 (needed in a later milestone)

Configure:
```powershell
cmake --preset windows-vs2022
```

Build Development:
```powershell
cmake --build --preset windows-development
```

Run:
```powershell
.\build\windows-vs2022\bin\Development\Platformer3D.exe
```

## Cursor
Open the repository root in Cursor. The agent will pick up `AGENTS.md`, `.cursor/rules/*.mdc`, and `.cursor/skills/*/SKILL.md`.

Start with:
> Read AGENTS.md and docs/MILESTONES.md. Execute Milestone 00 only. Inspect the repository, configure and build the Development preset, fix only foundation/build issues, and report whether all acceptance criteria are satisfied. Do not start Milestone 01.
