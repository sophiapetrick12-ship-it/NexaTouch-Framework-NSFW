#pragma once

#include <RE/Skyrim.h>

namespace NexaTouch
{
    // Attaches a clone of the player's right-hand scene graph to the target's
    // head node. This is an experimental renderer entry point; callers must
    // validate the actor 3D and supported skeleton before invoking it.
    [[nodiscard]] bool AttachPlayerHandToTarget(RE::Actor& a_player, RE::Actor& a_target);

    // Detaches any hand scene graph previously attached by this renderer.
    void DetachPlayerHandFromTarget();
}
