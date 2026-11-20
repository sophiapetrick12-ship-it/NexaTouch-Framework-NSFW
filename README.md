# NexaTouch Framework NSFW

An independent Skyrim Special Edition SKSE plugin project focused on safe, configurable NPC interaction systems.

## Target baseline
- Skyrim Special Edition runtime: **1.5.97.0 only**
- Required skeleton baseline: **XPMSSE**
- Native layer: C++ / SKSE using CommonLibSSE-NG
- Build environment: GitHub Actions (Windows + MSVC)

## Current status
NexaTouch registers a **Settings** page inside the existing **SKSE Menu Framework** control panel. It does not create a separate F1 or MessageBox menu. The page exposes the target-selection enable toggle, a configurable interaction key (default G), and verbose logging. Settings persist to `Data/SKSE/Plugins/NexaTouchFramework.ini`. The framework's own configured hotkey opens the control panel.

Keyboard input is registered after SKSE sends `kInputLoaded`; the settings page is registered after `kPostLoad` when the framework is available. The current interaction stage performs crosshair target acquisition with actor/alive/combat/range checks and safe-handle session control. It does not move or freeze actors, draw a mouse-zone selector, perform touch animations, display the player hand, or provide NPC reactions/physics.

## Required interaction scope
- Humanoid NPC interactions and Awww-style animal interactions are both mandatory parts of the project scope.
- Animal support is designed into the framework from the start, using species-specific verified skeleton/animation profiles rather than reusing humanoid assumptions.

## Safety principles
- Never write to actor, skeleton, or animation data using guessed offsets.
- Detect required nodes and capabilities before enabling an interaction.
- Fail closed if a required node, actor state, supported runtime, or animation resource cannot be verified.
- Do not overwrite XPMSSE, Faster HDT-SMP, CBPC, or physics-controlled bones.
- Restore any temporary actor/camera state when an interaction ends or is interrupted.
- Keep Skyrim SE 1.5.97.0 as the first validation target.

## Build
Pushes to `main` and pull requests run the Windows GitHub Actions workflow. Successful builds publish a MO2-ready package containing the plugin and default INI.
