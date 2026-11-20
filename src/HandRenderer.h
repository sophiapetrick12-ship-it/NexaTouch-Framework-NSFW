#pragma once

#include <RE/Skyrim.h>

namespace NexaTouch
{
    // Clones the player's live 3D scene, keeps only the hand/forearm geometry,
    // and attaches that clone to the target's head node. Returns false if the
    // player's actual hand mesh cannot be identified safely.
    [[nodiscard]] bool AttachPlayerHandToTarget(RE::Actor& a_player, RE::Actor& a_target);
    void DetachPlayerHandFromTarget();
}
