#pragma once
#include <RE/Skyrim.h>
namespace NexaTouch {
    [[nodiscard]] bool AttachPlayerHandToTarget(RE::Actor& a_player, RE::Actor& a_target);
    void UpdatePetHeadHand(float a_horizontal, float a_vertical, float a_stroke) noexcept;
    void DetachPlayerHandFromTarget();
}
