#include "InteractionController.h"
#include "SkeletonProbe.h"
#include "HandRenderer.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/spdlog.h>

#include <cstdint>

namespace
{
    constexpr float kMaximumTargetDistance = 250.0F;
    NexaTouch::Config* g_config{ nullptr };
    std::filesystem::path g_configPath;
    bool g_registered{ false };
    bool g_interactionActive{ false };
    RE::ObjectRefHandle g_target;

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
                if (!button || button->GetDevice() != RE::INPUT_DEVICE::kKeyboard || !button->IsDown()) continue;

                const auto key = button->GetIDCode();
                if (g_config && g_config->verboseLogging) {
                    spdlog::debug("NexaTouch keyboard input received: scanCode={}.", key);
                }
                if (g_config && key == g_config->interactionKeyScanCode) {
                    ToggleInteraction();
                    break;
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
                NexaTouch::SetHeadTouchActive(false);
                g_interactionActive = false;
                g_target.reset();
                spdlog::info("Interaction mode ended by configured key.");
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
            if (actor->IsDead() || actor->IsInCombat()) {
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
                "Target skeleton capability probe: coreNodes={}, extendedTouchNodes={} (root={}, head={}, leftHand={}, rightHand={}, leftBreast={}, rightBreast={}, belly={}, butt={}).",
                skeleton.HasCoreNodes(), skeleton.HasExtendedTouchNodes(), skeleton.hasRoot, skeleton.hasHead,
                skeleton.hasLeftHand, skeleton.hasRightHand, skeleton.hasLeftBreast, skeleton.hasRightBreast,
                skeleton.hasBelly, skeleton.hasButt);
            spdlog::info("XPMSSE butt nodes: left={}, right={}", skeleton.hasLeftButt, skeleton.hasRightButt);

            g_target = actor->GetHandle();
            g_interactionActive = true;
            spdlog::info("Target session started. FormID={:08X}, distance={:.1f}; safe handle only, no actor transform or animation data changed.",
                actor->GetFormID(), distance);
            spdlog::info("Interaction cursor ring is active; body-zone selection, actor positioning and hand/touch animation are not implemented in this stage.");
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

    bool IsInteractionActive() noexcept
    {
        return g_interactionActive;
    }

    void SetHeadTouchActive(bool a_active)
    {
        const auto handle = g_target;
        const auto tasks = SKSE::GetTaskInterface();
        if (!tasks) {
            spdlog::error("Pet Head operation could not be queued: SKSE task interface is unavailable.");
            return;
        }

        tasks->AddTask([handle, a_active]() {
            if (!a_active) {
                NexaTouch::DetachPlayerHandFromTarget();
                if (handle) {
                    auto reference = handle.get();
                    auto actor = reference ? reference->As<RE::Actor>() : nullptr;
                    if (actor) {
                        if (auto faceData = actor->GetFaceGenAnimationData()) {
                            RE::BSSpinLockGuard lock(faceData->lock);
                            faceData->ClearExpressionOverride();
                            faceData->Reset(0.0F, true, true, true, false);
                        }
                    }
                }
                spdlog::info("Pet Head ended: cloned hand removed and temporary facial expression cleared.");
                return;
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            auto reference = handle.get();
            auto actor = reference ? reference->As<RE::Actor>() : nullptr;
            if (!player || !actor || actor == player) {
                spdlog::warn("Pet Head hand clone skipped: player or target actor handle is invalid.");
                return;
            }

            if (!NexaTouch::AttachPlayerHandToTarget(*player, *actor)) {
                spdlog::warn("Pet Head did not start: the player's actual hand mesh could not be attached safely.");
                return;
            }

            if (auto faceData = actor->GetFaceGenAnimationData()) {
                RE::BSSpinLockGuard lock(faceData->lock);
                faceData->exprOverride = false;
                faceData->SetExpressionOverride(10, 0.70F);
                faceData->exprOverride = true;
            }
            spdlog::info("Pet Head active: player's cloned 3D hand is attached to the NPC head and the NPC happy reaction is enabled.");
        });
    }

    void SetInteractionEnabled(bool a_enabled)
    {
        if (!g_config) return;
        g_config->enableInteractions = a_enabled;
        if (!a_enabled) {
            SetHeadTouchActive(false);
            g_interactionActive = false;
            g_target.reset();
        }
    }
}
