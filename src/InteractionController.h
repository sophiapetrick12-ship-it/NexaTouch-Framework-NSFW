#pragma once

#include "Config.h"
#include <filesystem>

namespace NexaTouch
{
    void InstallInteractionInput(Config& a_config, const std::filesystem::path& a_configPath);
    void SetInteractionEnabled(bool a_enabled);
    [[nodiscard]] bool IsInteractionActive() noexcept;
    [[nodiscard]] bool BeginPetHead();
    void UpdatePetHead(float a_horizontal, float a_vertical, float a_stroke) noexcept;
    void EndPetHead();
}
