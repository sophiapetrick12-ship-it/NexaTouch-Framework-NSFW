#include "InteractionController.h"
#include "SkeletonProbe.h"

#include <RE/Skyrim.h>
#include <spdlog/spdlog.h>

#include <cstdint>

namespace
{
    constexpr std::uint32_t kGScanCode = 34;
    constexpr float kMaximumTargetDistance = 250.0F;

    class InteractionInputSink final : public RE::BSTEventSink<RE::InputEvent*>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* a_event,
            RE::BSTEventSource<RE::InputEvent*>*) override
        {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            for (auto event = *a_event; event; event = event->next) {
                if (event->GetEventType() != RE::INPUT_EVENT_TYPE::kButton) {
                    continue;
                }

                auto button = event->AsButtonEvent();
                if (!button ||
                    button->GetDevice() != RE::INPUT_DEVICE::kKeyboard ||
                    button->GetIDCode() != kGScanCode ||
                    !button->IsDown()) {
                    continue;
                }

                ToggleInteraction();
                break;
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        void ToggleInteraction()
        {
            if (active_) {
                active_ = false;
                target_.reset();
                spdlog::info("Interaction mode ended by G.");
                return;
            }

            auto player = RE::PlayerCharacter::GetSingleton();
            auto pickData = RE::CrosshairPickData::GetSingleton();
            if (!player || !pickData) {
                spdlog::warn("G pressed, but player or crosshair target data is unavailable.");
                return;
            }

            auto pickedReference = pickData->GetActiveTarget().get();
            if (!pickedReference) {
                spdlog::info("G pressed, but no crosshair target was found.");
                return;
            }

            auto actor = pickedReference->As<RE::Actor>();
            if (!actor || actor == player) {
                spdlog::info("G target rejected: target is not a valid non-player actor.");
                return;
            }

            if (actor->IsDead() || actor->IsInCombat()) {
                spdlog::info("G target rejected: actor is dead or in combat.");
                return;
            }

            const auto distance = player->GetDistance(actor);
            if (distance > kMaximumTargetDistance) {
                spdlog::info("G target rejected: {:.1f} game units is beyond the interaction range.", distance);
                return;
            }

            auto root = actor->Get3D();
            if (!root) {
                spdlog::info("G target rejected: actor 3D is not currently loaded.");
                return;
            }

            const auto skeleton = NexaTouch::InspectHumanoidSkeleton(*actor);
            spdlog::info(
                "Target skeleton capability probe: coreNodes={}, extendedTouchNodes={} (root={}, head={}, leftHand={}, rightHand={}, leftBreast={}, rightBreast={}, belly={}, butt={}).",
                skeleton.HasCoreNodes(),
                skeleton.HasExtendedTouchNodes(),
                skeleton.hasRoot,
                skeleton.hasHead,
                skeleton.hasLeftHand,
                skeleton.hasRightHand,
                skeleton.hasLeftBreast,
                skeleton.hasRightBreast,
                skeleton.hasBelly,
                skeleton.hasButt);

            target_ = actor->GetHandle();
            active_ = true;

            spdlog::info(
                "Interaction mode started. Target formID={:08X}, distance={:.1f}. Target is held as a safe handle; no actor transform or animation data has been changed.",
                actor->GetFormID(),
                distance);
            spdlog::info("Mouse-zone selection and actor positioning are not active in this build stage.");
        }

        bool active_{ false };
        RE::ObjectRefHandle target_;
    };

    InteractionInputSink g_inputSink;
    bool g_registered{ false };
}

namespace NexaTouch
{
    void InstallInteractionInput()
    {
        if (g_registered) {
            return;
        }

        auto inputManager = RE::BSInputDeviceManager::GetSingleton();
        if (!inputManager) {
            spdlog::error("Unable to register NexaTouch input: BSInputDeviceManager is unavailable.");
            return;
        }

        inputManager->AddEventSink(&g_inputSink);
        g_registered = true;
        spdlog::info("NexaTouch input handler registered. G scan code 34 starts/ends safe target selection.");
    }
}
