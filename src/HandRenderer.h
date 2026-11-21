#pragma once
#include <RE/Skyrim.h>
namespace NexaTouch {
    [[nodiscard]] bool IsPetHeadContact(RE::Actor& a_target, float a_mouseX, float a_mouseY, float a_viewportWidth, float a_viewportHeight, RE::NiPoint3& a_localContact) noexcept;
    [[nodiscard]] bool AttachPlayerHandToTarget(RE::Actor& a_player, RE::Actor& a_target);
    void UpdatePetHeadHand(float a_horizontal, float a_vertical, float a_stroke, float a_mouseX, float a_mouseY, float a_viewportWidth, float a_viewportHeight) noexcept;
    void DetachPlayerHandFromTarget();
}
