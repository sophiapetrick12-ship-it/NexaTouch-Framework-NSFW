# Initial architecture and safety boundaries

## Components
- **SKSE native plugin:** runtime validation, logging, configuration, and later verified engine integration.
- **Papyrus/UI layer:** planned for a quest-backed F1 menu and user-facing interaction controls; not implemented yet.
- **Animation assets:** must be authored and validated as behavior and animation resources. C++ and Papyrus alone do not create a complete animated hand interaction.
- **Skeleton compatibility:** XPMSSE is the mandatory baseline. CBBE/3BA, BHUNP, CBPC, and Faster HDT-SMP compatibility must be validated independently.

## Current implemented foundation
- Loads through SKSE and writes a dedicated NexaTouch log.
- Accepts only Skyrim SE runtime **1.5.97.0**. Other runtimes fail closed.
- Loads or creates `Data/SKSE/Plugins/NexaTouchFramework.ini`.
- Parses boolean feature settings and logs the effective configuration.
- Does not modify actor, skeleton, animation, physics, or camera state.

## Interaction lifecycle (planned)
1. Validate player/NPC references and actor states.
2. Confirm required XPMSSE nodes and animation resources exist.
3. Check that combat, death, dialogue, scene, and other interruption conditions permit interaction.
4. Acquire only the minimum temporary control needed.
5. Run authored animation and synchronized reactions.
6. Restore all acquired state on normal completion, cancellation, unload, or interruption.

Any failed validation must abort before actor/skeleton writes. Unknown node layouts and unknown offsets are not acceptable fallbacks.

## Physics ownership
The plugin must not take ownership of physics-controlled bones or replace XPMSSE/CBPC/Faster HDT-SMP behavior. Node discovery and animation integration must coexist with those systems, not overwrite them.

## Camera policy
Camera adjustment is optional and must not force a perspective switch. If a supported camera integration cannot be established safely, leave the camera untouched.

## Compatibility policy
Skyrim SE 1.5.97.0 is the only enabled runtime. CBBE/3BA, BHUNP, CBPC, Faster HDT-SMP, Improved Camera, and SmoothCam support are targets for separate validation, not blanket compatibility claims.

## Mandatory animal support (Awww-style)
Animal interactions are a first-class requirement, not a later optional add-on. The framework must support both humanoid NPCs and supported animal actors, following the same user-facing interaction approach as Awww where applicable.

- Build a separate capability/profile layer for humanoid and animal actors; never assume an animal uses a humanoid skeleton, nodes, animation graph, or behavior.
- Animal support must be designed into target selection, eligibility checks, interaction state, cancellation, and cleanup from the beginning.
- Each supported animal race/species requires verified skeleton/node discovery and matching animation/behavior resources before its interactions can be enabled.
- If an animal's rig or animation set is unknown or unsupported, skip that actor safely. Do not force humanoid animations onto it and do not write guessed node offsets.
- Maintain a compatibility matrix by animal race/species and skeleton/mod setup. Add support incrementally and test each profile independently.
- Animal interactions should feel native to the framework and follow the same quality/safety standards as NPC interactions; they must not be silently omitted from the final scope.
