#include <SKSE/SKSE.h>

#include "Config.h"
#include "InteractionController.h"
#include "SettingsMenu.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <system_error>

namespace
{
    NexaTouch::Config g_config;
    std::filesystem::path g_configPath;

    void InitializeLogging()
    {
        const auto logDirectory = SKSE::log::log_directory();
        if (!logDirectory) return;
        std::error_code error;
        std::filesystem::create_directories(*logDirectory, error);
        const auto logFile = *logDirectory / "NexaTouchFramework.log";
        try {
            auto logger = spdlog::basic_logger_mt("NexaTouchFramework", logFile.string(), true);
            spdlog::set_default_logger(std::move(logger));
            spdlog::set_level(spdlog::level::info);
            spdlog::flush_on(spdlog::level::info);
        } catch (const spdlog::spdlog_ex&) {
        }
    }

    [[nodiscard]] bool IsSupportedRuntime()
    {
        const auto runtime = REL::Module::get().version();
        spdlog::info("Detected Skyrim runtime: {}", runtime.string("."));
        constexpr REL::Version supportedRuntime{ 1, 5, 97, 0 };
        if (REL::Module::GetRuntime() != REL::Module::Runtime::SE || runtime != supportedRuntime) {
            spdlog::error("Unsupported Skyrim runtime. NexaTouch currently supports Skyrim SE 1.5.97.0 only.");
            return false;
        }
        spdlog::info("Skyrim SE 1.5.97.0 verified.");
        return true;
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
    {
        if (!a_message) return;
        if (a_message->type == SKSE::MessagingInterface::kInputLoaded) {
            spdlog::info("SKSE kInputLoaded received; registering NexaTouch keyboard input now.");
            NexaTouch::InstallInteractionInput(g_config, g_configPath);
        } else if (a_message->type == SKSE::MessagingInterface::kPostLoad) {
            spdlog::info("SKSE kPostLoad received; registering NexaTouch settings page with SKSE Menu Framework.");
            NexaTouch::RegisterSettingsMenu(g_config, g_configPath);
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    InitializeLogging();
    spdlog::info("NexaTouch Framework {} loading.", NEXATOUCH_VERSION);
    if (!IsSupportedRuntime()) return false;

    std::error_code error;
    const auto gameDirectory = std::filesystem::current_path(error);
    if (error) spdlog::error("Unable to resolve game working directory; configuration will use safe defaults.");

    g_configPath = (error ? std::filesystem::path{} : gameDirectory) /
                   "Data" / "SKSE" / "Plugins" / "NexaTouchFramework.ini";
    g_config = NexaTouch::LoadConfig(g_configPath);

    spdlog::info(
        "Configuration loaded: Interactions={}, InteractionKeyScanCode={}, EyeTracking={}, CameraAdjustment={}, AudioReactions={}, RequireXPMSSE={}, VerboseLogging={}",
        g_config.enableInteractions, g_config.interactionKeyScanCode, g_config.enableEyeTracking,
        g_config.enableCameraAdjustment, g_config.enableAudioReactions, g_config.requireXPMSSE,
        g_config.verboseLogging);

    const auto messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener("SKSE", OnSKSEMessage)) {
        spdlog::error("Unable to register SKSE messaging listener; keyboard input and menu registration will be unavailable.");
    } else {
        spdlog::info("Waiting for SKSE kInputLoaded and kPostLoad notifications.");
    }

    spdlog::info("NexaTouch stage 0.2/0.3 initialized. Configurable target acquisition and safe-handle session control are enabled.");
    spdlog::info("NexaTouch settings are provided through SKSE Menu Framework; no separate F1 menu is created.");
    spdlog::info("No actor transforms, skeleton nodes, animation graphs, physics, or camera data are modified.");
    return true;
}
