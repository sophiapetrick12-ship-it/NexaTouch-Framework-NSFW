# NexaTouch Framework NSFW

An independent Skyrim Special Edition SKSE plugin project focused on safe, configurable NPC interaction systems.

## Target baseline
- Skyrim Special Edition runtime: **1.5.97**
- Required skeleton baseline: **XPMSSE**
- Native layer: C++ / SKSE using CommonLibSSE-NG
- Build environment: GitHub Actions (Windows + MSVC); no local compiler installation is required for CI builds

## Development status
**Early foundation only.** This repository currently establishes the project layout and automated build pipeline. It does not yet contain working hand animations, actor manipulation, an in-game menu, or finished interaction features.

## Safety principles
- Never write to actor, skeleton, or animation data using guessed offsets.
- Detect required nodes and capabilities before enabling an interaction.
- Fail closed: if a required node, actor state, or supported runtime cannot be verified, cancel the interaction safely.
- Do not overwrite XPMSSE, Faster HDT-SMP, CBPC, or their physics-controlled bones.
- Restore any temporary actor/camera state when an interaction ends or is interrupted.
- Keep Skyrim SE 1.5.97 as the first validation target. Other runtimes remain unsupported until separately tested.

## Planned milestones
1. Establish a clean SKSE plugin build for Skyrim SE 1.5.97.
2. Add configuration, logging, and runtime capability checks.
3. Design the interaction state machine and safe cancellation/restore path.
4. Investigate animation resource integration and XPMSSE node mapping.
5. Add the F1 interaction menu after the native foundation is stable.
6. Validate first-person and third-person behavior in-game before enabling features by default.

## Build
Pushes to `main` and pull requests run the Windows GitHub Actions workflow. Build artifacts are published only when compilation succeeds.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the initial technical boundaries.
