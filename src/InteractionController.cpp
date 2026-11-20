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
    RE::ObjectRefHandle g_target;

    void ReleaseTarget()
    {
        NexaTouch::DetachPlayerHandFromTarget();
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
                if (!button || button->GetDevice() != RE::INPUT_DEVICE::kKeyboard) continue;

                const auto key = button->GetIDCode();
                if (g_config && key == g_config->interactionKeyScanCode) {
                    if (!button->IsDown()) {
                        g_interactionKeyDown = false;
                        continue;
                    }

                    // Consume the configured interaction key before Skyrim's
                    // normal input handlers can interpret it as Activate/Talk.
                    if (!g_interactionKeyDown) {
                        g_interactionKeyDown = true;
                        if (g_config->verboseLogging) {
                            spdlog::debug("NexaTouch interaction key captured: scanCode={}.", key);
                        }
                        ToggleInteraction();
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
            g_interactionActive = true;
            spdlog::info("Pet Head session started. FormID={:08X}, distance={:.1f}. Hold left mouse for one second over the head selector.", actor->GetFormID(), distance);
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

    bool BeginPetHead()
    {
        if (!g_interactionActive || !g_target) return false;
        auto player = RE::PlayerCharacter::GetSingleton();
        auto reference = RE::TESObjectREFR::LookupByHandle(g_target);
        auto target = reference ? reference->As<RE::Actor>() : nullptr;
        if (!player || !target || target->IsDead() || !target->Get3D()) return false;
        const auto skeleton = InspectHumanoidSkeleton(*target);
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

    void UpdatePetHead(float horizontal, float vertical, float stroke) noexcept
    {
        UpdatePetHeadHand(horizontal, vertical, stroke);
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
