# Initial architecture and safety boundaries

## Components
- **SKSE native plugin:** runtime detection, logging, configuration, and later verified engine integration.
- **Papyrus/UI layer:** planned for quest-backed menu and user-facing interaction controls; not implemented yet.
- **Animation assets:** planned as separately authored/validated behavior and animation resources. C++ and Papyrus alone do not create a complete animated hand interaction.
- **Skeleton compatibility:** XPMSSE is the mandatory baseline. CBBE/3BA, BHUNP, CBPC, and Faster HDT-SMP compatibility must be validated independently.

## Runtime policy
The first target is Skyrim SE 1.5.97 only. Do not advertise AE/VR support or load behavior on another runtime until it has a dedicated compatibility check.

## Interaction lifecycle (planned)
1. Validate player/NPC references and actor states.
2. Confirm required skeleton nodes and animation resources exist.
3. Check that combat, death, dialogue, scene, and other interruption conditions permit interaction.
4. Acquire only the minimum temporary control needed.
5. Run the authored animation and synchronized reactions.
6. Restore all acquired state on normal completion, cancellation, unload, or interruption.

Any failed validation must abort before actor/skeleton writes. Unknown node layouts and unknown offsets are not acceptable fallbacks.

## Physics ownership
The plugin must not take ownership of physics-controlled bones or replace XPMSSE/CBPC/Faster HDT-SMP behavior. Node discovery and animation integration must be designed to coexist with those systems, not to overwrite them.

## Camera policy
Camera adjustment is optional and must not force a perspective switch. If a supported camera integration cannot be established safely, leave the camera untouched.

## Current implementation status
This document describes constraints and intended architecture only. It is not evidence that the listed features have been implemented.
