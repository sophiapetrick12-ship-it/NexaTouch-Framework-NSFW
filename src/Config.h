#pragma once

#include <filesystem>

namespace NexaTouch
{
    struct Config
    {
        bool enableInteractions{ false };
        bool enableEyeTracking{ true };
        bool enableCameraAdjustment{ false };
        bool enableAudioReactions{ true };
        bool requireXPMSSE{ true };
        bool verboseLogging{ false };
    };

    [[nodiscard]] Config LoadConfig(const std::filesystem::path& a_path);
    [[nodiscard]] bool SaveConfig(const Config& a_config, const std::filesystem::path& a_path);
}
