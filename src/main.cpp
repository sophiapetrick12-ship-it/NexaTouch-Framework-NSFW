#include <SKSE/SKSE.h>

#include "Config.h"
#include "InteractionController.h"
#include "SkeletonProbe.h"
#include "SettingsMenu.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <system_error>

namespace
{
    void InitializeLogging()
    {
        const auto logDirectory = SKSE::log::log_directory();
        if (!logDirectory) {
            return;
        }

        std::error_code error;
        std::filesystem::create_directories(*logDirectory, error);
        const auto logFile = *logDirectory / "NexaTouchFramework.log";

        try {
            auto logger = spdlog::basic_logger_mt("NexaTouchFramework", logFile.string(), true);
            spdlog::set_default_logger(std::move(logger));
            spdlog::set_level(spdlog::level::info);
            spdlog::flush_on(spdlog::level::info);
        } catch (const spdlog::spdlog_ex&) {
            // Logging must never be allowed to crash plugin initialization.
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
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    InitializeLogging();

    spdlog::info("NexaTouch Framework {} loading.", NEXATOUCH_VERSION);

    if (!IsSupportedRuntime()) {
        return false;
    }

    std::error_code error;
    const auto gameDirectory = std::filesystem::current_path(error);
    if (error) {
        spdlog::error("Unable to resolve the game working directory; configuration will use safe defaults.");
    }

    const auto configPath = (error ? std::filesystem::path{} : gameDirectory) /
                            "Data" / "SKSE" / "Plugins" / "NexaTouchFramework.ini";
    auto config = NexaTouch::LoadConfig(configPath);

    spdlog::info(
        "Configuration loaded: Interactions={}, EyeTracking={}, CameraAdjustment={}, AudioReactions={}, RequireXPMSSE={}, VerboseLogging={}",
        config.enableInteractions,
        config.enableEyeTracking,
        config.enableCameraAdjustment,
        config.enableAudioReactions,
        config.requireXPMSSE,
        config.verboseLogging);

    if (config.enableInteractions) {
        NexaTouch::InstallInteractionInput(config, configPath);
    } else {
        spdlog::info("Interactions are disabled in the INI; input handler was not registered.");
    }

    NexaTouch::RegisterSettingsMenu(config, configPath);

    spdlog::info("NexaTouch stage 0.2/0.3 initialized. G target acquisition and safe-handle session control are enabled.");
    spdlog::info("Humanoid skeleton probe supports core and extended touch-node checks; this is a capability probe, not proof of XPMSSE file provenance.");
    spdlog::info("No actor transforms, skeleton nodes, animation graphs, physics, or camera data are modified.");
    return true;
}
