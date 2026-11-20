#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>

namespace
{
    void InitializeLogging()
    {
        auto logDirectory = SKSE::log::log_directory();
        if (!logDirectory) {
            return;
        }

        std::filesystem::create_directories(*logDirectory);
        auto logFile = *logDirectory / "NexaTouchFramework.log";

        auto logger = spdlog::basic_logger_mt("NexaTouchFramework", logFile.string(), true);
        spdlog::set_default_logger(std::move(logger));
        spdlog::set_level(spdlog::level::info);
        spdlog::flush_on(spdlog::level::info);
        spdlog::info("NexaTouch Framework {} loading.", NEXATOUCH_VERSION);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    InitializeLogging();

    if (auto* logger = spdlog::default_logger_raw()) {
        logger->info("SKSE plugin entry point reached. No interaction features are enabled yet.");
    }

    return true;
}
