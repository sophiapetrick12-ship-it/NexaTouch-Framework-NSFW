#include "InteractionController.h"
#include "SkeletonProbe.h"
#include "HandRenderer.h"

#include <RE/Skyrim.h>
#include <spdlog/spdlog.h>

#include <cstdint>

namespace
{
    constexpr float kMaximumTargetDistance = 250.0F;
    NexaTouch::Config* g_config{ nullptr };
    std::filesystem::path g_configPath;
    bool g_registered{ false };
    bool g_interactionActive{ false };
    bool g_interactionKeyDown{ false };
    std::uint32_t g_savedEnabledControls{ 0 };
    std::uint32_t g_savedStoredControls{ 0 };
    bool g_controlsStateSaved{ false };
    RE::ObjectRefHandle g_target;
    void RestorePlayerControls()
    {
        if (!g_controlsStateSaved) return;
        if (auto* controls = RE::ControlMap::GetSingleton()) {
            controls->SetControlsState(g_savedEnabledControls, g_savedStoredControls);
            spdlog::info("NexaTouch restored the player's original control-map state.");
        } else {
            spdlog::error("NexaTouch could not restore controls: ControlMap is unavailable.");
        }
        g_controlsStateSaved = false;
    }

    void DisableInteractionControls()
    {
        auto* controls = RE::ControlMap::GetSingleton();
        if (!controls) {
            spdlog::error("NexaTouch cannot safely start: ControlMap is unavailable.");
            return;
        }
        controls->GetControlsState(g_savedEnabledControls, g_savedStoredControls);
        g_controlsStateSaved = true;

        using Flag = RE::ControlMap::UEFlag;
        controls->ToggleControls(Flag::kMovement, false, false);
        controls->ToggleControls(Flag::kFighting, false, false);
        controls->ToggleControls(Flag::kActivate, false, false);
        controls->ToggleControls(Flag::kJumping, false, false);
        controls->ToggleControls(Flag::kSneaking, false, false);
        spdlog::info("NexaTouch disabled movement, combat, activate, jump and sneak controls; look controls remain enabled.");
    }

    void ReleaseTarget()
    {
        NexaTouch::DetachPlayerHandFromTarget();
        RestorePlayerControls();
        g_target.reset();
    }

    class InteractionInputSink final : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* a_event,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!a_event) return RE::BSEventNotifyControl::kContinue;

            for (auto event = *a_event; event; event = event->next) {
                if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) continue;
                auto button = event->AsButtonEvent();
                if (!button) continue;

                // While a NexaTouch session is active, swallow every physical
                // mouse button event before vanilla player handlers see it.
                // This deliberately covers both attack buttons and any remapped
                // mouse buttons, including their release/held events.
                if (button->GetDevice() == RE::INPUT_DEVICE::kMouse) {
                    if (g_interactionActive) {
                        if (g_config && g_config->verboseLogging && button->IsDown()) {
                            spdlog::debug("NexaTouch mouse button captured: idCode={}.", button->GetIDCode());
                        }
                        return RE::BSEventNotifyControl::kStop;
                    }
                    continue;
                }

                if (button->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;
                const auto key = button->GetIDCode();
                if (g_config && key == g_config->interactionKeyScanCode) {
                    if (!button->IsDown()) {
                        g_interactionKeyDown = false;
                        continue;
                    }

                    // G remains the sole keyboard control in an active session.
                    // It is consumed before Skyrim can interpret it as Activate/Talk.
                    if (!g_interactionKeyDown) {
                        g_interactionKeyDown = true;
                        if (g_config->verboseLogging) {
                            spdlog::debug("NexaTouch interaction key captured: scanCode={}.", key);
                        }
                        ToggleInteraction();
                    }
                    return RE::BSEventNotifyControl::kStop;
                }

                // Suppress movement, weapon switching, spell hotkeys, shouts,
                // and other vanilla keyboard actions while the session is active.
                if (g_interactionActive) {
                    if (g_config && g_config->verboseLogging && button->IsDown()) {
                        spdlog::debug("NexaTouch keyboard action blocked during session: scanCode={}.", key);
                    }
                    return RE::BSEventNotifyControl::kStop;
                }

                if (g_config && g_config->verboseLogging && button->IsDown()) {
                    spdlog::debug("NexaTouch keyboard input received: scanCode={}.", key);
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        void ToggleInteraction()
        {
            if (!g_config || !g_config->enableInteractions) {
                spdlog::info("Interaction key pressed, but target selection is disabled in NexaTouch settings.");
                return;
            }
            if (g_interactionActive) {
                g_interactionActive = false;
                ReleaseTarget();
                spdlog::info("Interaction mode ended; Pet Head hand detached and target session closed.");
                return;
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            auto pickData = RE::CrosshairPickData::GetSingleton();
            if (!player || !pickData) {
                spdlog::warn("Interaction key pressed, but player or crosshair target data is unavailable.");
                return;
            }

            auto pickedReference = pickData->GetActiveTarget().get();
            if (!pickedReference) {
                spdlog::info("Interaction key pressed, but no crosshair target was found.");
                return;
            }

            auto actor = pickedReference->As<RE::Actor>();
            if (!actor || actor == player) {
                spdlog::info("Target rejected: target is not a valid non-player actor.");
                return;
            }
            if (actor->IsDead() || actor->IsInCombat() || actor->GetCurrentScene()) {
                spdlog::info("Target rejected: actor is dead or in combat.");
                return;
            }

            const auto distance = player->GetDistance(actor);
            if (distance > kMaximumTargetDistance) {
                spdlog::info("Target rejected: {:.1f} game units is beyond the interaction range.", distance);
                return;
            }
            if (!actor->Get3D()) {
                spdlog::info("Target rejected: actor 3D is not currently loaded.");
                return;
            }

            const auto skeleton = NexaTouch::InspectHumanoidSkeleton(*actor);
            spdlog::info(
                "Target skeleton capability probe: coreNodes={}, extendedTouchNodes={} (root={}, head={}, leftHand={}, rightHand={}, leftBreast={}, rightBreast={}, belly={}, leftButt={}, rightButt={}, legacyButt={}, anyButt={}).",
                skeleton.HasCoreNodes(), skeleton.HasExtendedTouchNodes(), skeleton.hasRoot, skeleton.hasHead,
                skeleton.hasLeftHand, skeleton.hasRightHand, skeleton.hasLeftBreast, skeleton.hasRightBreast,
                skeleton.hasBelly, skeleton.hasLeftButt, skeleton.hasRightButt, skeleton.hasLegacyButt,
                skeleton.HasAnyButtNode());

            const auto lifeState = actor->AsActorState() ?
                static_cast<std::uint32_t>(actor->AsActorState()->GetLifeState()) : 0xFFFFFFFFU;
            if (lifeState != 0U) {
                spdlog::info("Target rejected: actor is not alive (lifeState={}).", lifeState);
                return;
            }
            g_target = actor->GetHandle();

            // Keep Skyrim's camera owned by the player. Never assign the NPC
            // to PlayerCamera::cameraTarget.
            DisableInteractionControls();
            if (!g_controlsStateSaved) {
                g_target.reset();
                spdlog::error("NexaTouch session cancelled because player controls could not be safely isolated.");
                return;
            }
            g_interactionActive = true;
            spdlog::info("Pet Head session started. FormID={:08X}, distance={:.1f}. Mouse buttons and keyboard actions are captured until G exits.", actor->GetFormID(), distance);
        }
    };

    InteractionInputSink g_inputSink;
}

namespace NexaTouch
{
    void InstallInteractionInput(Config& a_config, const std::filesystem::path& a_configPath)
    {
        g_config = &a_config;
        g_configPath = a_configPath;
        if (g_registered) return;

        auto inputManager = RE::BSInputDeviceManager::GetSingleton();
        if (!inputManager) {
            spdlog::error("Unable to register NexaTouch input after SKSE input-loaded notification: BSInputDeviceManager is still unavailable.");
            return;
        }
        inputManager->AddEventSink(&g_inputSink);
        g_registered = true;
        spdlog::info("NexaTouch interaction key registered after SKSE kInputLoaded. Current scan code={}.", g_config->interactionKeyScanCode);
    }

    bool IsPetHeadCursorTarget(float mouseX, float mouseY, float viewportWidth, float viewportHeight) noexcept
    {
        if (!g_interactionActive || !g_target) return false;
        auto reference = g_target.get();
        auto target = reference ? reference->As<RE::Actor>() : nullptr;
        if (!target || target->IsDead() || target->IsInCombat()) return false;
        RE::NiPoint3 contact;
        return IsPetHeadContact(*target, mouseX, mouseY, viewportWidth, viewportHeight, contact);
    }

    bool BeginPetHead(float mouseX, float mouseY, float viewportWidth, float viewportHeight)
    {
        if (!g_interactionActive || !g_target) return false;
        auto player = RE::PlayerCharacter::GetSingleton();
        auto reference = g_target.get();
        auto target = reference ? reference->As<RE::Actor>() : nullptr;
        if (!player || !target || target->IsDead() || !target->Get3D()) return false;
        const auto skeleton = InspectHumanoidSkeleton(*target);
        RE::NiPoint3 headContact;
        if (!IsPetHeadContact(*target, mouseX, mouseY, viewportWidth, viewportHeight, headContact)) return false;
        if (!skeleton.hasHead || !skeleton.hasRightHand) {
            spdlog::warn("Pet Head cancelled: required XPMSSE head/right-hand nodes are unavailable.");
            return false;
        }
        if (!AttachPlayerHandToTarget(*player, *target)) {
            spdlog::warn("Pet Head cancelled: player's hand could not be attached.");
            return false;
        }
        spdlog::info("Pet Head activated: visible player hand attached to target head.");
        return true;
    }

    void UpdatePetHead(float horizontal, float vertical, float stroke, float mouseX, float mouseY, float viewportWidth, float viewportHeight) noexcept
    {
        UpdatePetHeadHand(horizontal, vertical, stroke, mouseX, mouseY, viewportWidth, viewportHeight);
    }

    void EndPetHead()
    {
        DetachPlayerHandFromTarget();
    }

    bool IsInteractionActive() noexcept
    {
        return g_interactionActive;
    }

    void SetInteractionEnabled(bool a_enabled)
    {
        if (!g_config) return;
        g_config->enableInteractions = a_enabled;
        if (!a_enabled) {
            g_interactionActive = false;
            g_interactionKeyDown = false;
            ReleaseTarget();
        }
    }
}
