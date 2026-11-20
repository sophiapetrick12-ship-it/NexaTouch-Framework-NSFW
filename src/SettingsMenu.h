#pragma once
#include "Config.h"
#include <filesystem>

namespace NexaTouch
{
    void RegisterSettingsMenu(Config& a_config, const std::filesystem::path& a_configPath);
}
