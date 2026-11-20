#include "InteractionController.h"
#include "SkeletonProbe.h"

#include <RE/Skyrim.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>

namespace
{
    constexpr std::uint32_t kGScanCode = 34;
    constexpr std::uint32_t kF1ScanCode = 59;
    constexpr float kMaximumTargetDistance = 250.0F;

    NexaTouch::Config* g_config{ nullptr };
    std::filesystem::path g_configPath;
    bool g_registered{ false };
    bool g_interactionActive{ false };
    RE::ObjectRefHandle g_target;

    void OpenSettingsMenu();

    void OnSettingsMenuChoice(std::uint8_t a_choice)
    {
        if (!g_config) {
            return;
        }

        switch (a_choice) {
        case 0:
            g_config->enableInteractions = !g_config->enableInteractions;
            if (!g_config->enableInteractions) {
                g_interactionActive = false;
                g_target.reset();
            }
            spdlog::info("F1 menu: G target selection set to {}.", g_config->enableInteractions);
            break;
        case 1:
            g_config->verboseLogging = !g_config->verboseLogging;
            spdlog::set_level(g_config->verboseLogging ? spdlog::level::debug : spdlog::level::info);
            spdlog::info("F1 menu: verbose logging set to {}.", g_config->verboseLogging);
            break;
        default:
            return;
        }

        if (!NexaTouch::SaveConfig(*g_config, g_configPath)) {
            spdlog::error("F1 menu: failed to save NexaTouchFramework.ini.");
        }
        OpenSettingsMenu();
    }

    void OpenSettingsMenu()
    {
        if (!g_config) {
            spdlog::warn("F1 menu requested before configuration was ready.");
            return;
        }

        static std::string menuText;
        menuText =
            "NexaTouch Framework 0.2\n\n"
            "Current build: safe NPC target selection only. Hand, touch animations and NPC reactions are not implemented yet.\n\n"
            "Choose a setting:";

        const auto opened = RE::MessageBoxMenu::Create(
            menuText.c_str(),
            &OnSettingsMenuChoice,
            0,
            4,
            10,
            g_config->enableInteractions ? "Disable G target selection" : "Enable G target selection",
            g_config->verboseLogging ? "Disable verbose logging" : "Enable verbose logging",
            "Close");

        if (!opened) {
            spdlog::error("F1 menu could not be opened by Skyrim's MessageBoxMenu.");
        } else {
            spdlog::info("F1 settings menu opened.");
        }
    }

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
                    !button->IsDown()) {
                    continue;
                }

                const auto key = button->GetIDCode();
                if (g_config && g_config->verboseLogging) {
                    spdlog::debug("NexaTouch keyboard input received: scanCode={}.", key);
                }
                if (key == kF1ScanCode) {
                    OpenSettingsMenu();
                    break;
                }

                if (key == kGScanCode) {
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
                spdlog::info("G pressed, but target selection is disabled in NexaTouch settings.");
                return;
            }

            if (g_interactionActive) {
                g_interactionActive = false;
                g_target.reset();
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

            if (!actor->Get3D()) {
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

            g_target = actor->GetHandle();
            g_interactionActive = true;

            spdlog::info(
                "Interaction mode started. Target formID={:08X}, distance={:.1f}. Target is held as a safe handle; no actor transform or animation data has been changed.",
                actor->GetFormID(),
                distance);
            spdlog::info("Mouse-zone selection and actor positioning are not active in this build stage.");
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

        if (g_registered) {
            return;
        }

        auto inputManager = RE::BSInputDeviceManager::GetSingleton();
        if (!inputManager) {
            spdlog::error("Unable to register NexaTouch input after SKSE input-loaded notification: BSInputDeviceManager is still unavailable.");
            return;
        }

        inputManager->AddEventSink(&g_inputSink);
        g_registered = true;
        spdlog::info("NexaTouch input handler registered after SKSE kInputLoaded. G scan code 34; F1 scan code 59.");
    }
}
