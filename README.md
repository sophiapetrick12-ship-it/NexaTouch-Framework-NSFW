# NexaTouch Framework NSFW

An independent Skyrim Special Edition SKSE plugin project focused on safe, configurable NPC interaction systems.

## Target baseline
- Skyrim Special Edition runtime: **1.5.97.0 only**
- Required skeleton baseline: **XPMSSE**
- Native layer: C++ / SKSE using CommonLibSSE-NG
- Build environment: GitHub Actions (Windows + MSVC)

## Current status
The SKSE foundation loads successfully in-game on the user's Skyrim SE 1.5.97 setup. The current development stage adds a real G-key input listener, crosshair target acquisition, actor/alive/combat/range checks, and a safe-handle interaction session toggle. It also includes a humanoid skeleton capability probe for core hand/head nodes and extended touch nodes. It does not yet move or freeze actors, draw a mouse-zone selector, perform touch animations, display the player hand, or provide reactions/audio. The skeleton capability probe does not by itself prove that the loaded NIF originated from XPMSSE.

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
