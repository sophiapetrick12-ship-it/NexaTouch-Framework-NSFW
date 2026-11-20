#include "Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <string>

namespace
{
    [[nodiscard]] std::string Trim(std::string a_value)
    {
        const auto first = a_value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return {};
        }

        const auto last = a_value.find_last_not_of(" \t\r\n");
        return a_value.substr(first, last - first + 1);
    }

    [[nodiscard]] std::optional<bool> ParseBool(std::string a_value)
    {
        std::ranges::transform(a_value, a_value.begin(), [](unsigned char a_char) {
            return static_cast<char>(std::tolower(a_char));
        });

        if (a_value == "true" || a_value == "1" || a_value == "yes" || a_value == "on") {
            return true;
        }
        if (a_value == "false" || a_value == "0" || a_value == "no" || a_value == "off") {
            return false;
        }
        return std::nullopt;
    }
}

namespace NexaTouch
{
    Config LoadConfig(const std::filesystem::path& a_path)
    {
        Config config;

        std::error_code error;
        std::filesystem::create_directories(a_path.parent_path(), error);

        if (!std::filesystem::exists(a_path, error)) {
            std::ofstream output(a_path);
            if (output) {
                output << "; NexaTouch Framework configuration\n"
                          "; Interaction features remain disabled until their assets and runtime checks are implemented.\n"
                          "EnableInteractions=false\n"
                          "EnableEyeTracking=true\n"
                          "EnableCameraAdjustment=false\n"
                          "EnableAudioReactions=true\n"
                          "RequireXPMSSE=true\n"
                          "VerboseLogging=false\n";
            }
        }

        std::ifstream input(a_path);
        if (!input) {
            return config;
        }

        std::string line;
        while (std::getline(input, line)) {
            line = Trim(std::move(line));
            if (line.empty() || line.front() == ';' || line.front() == '#' || line.front() == '[') {
                continue;
            }

            const auto separator = line.find('=');
            if (separator == std::string::npos) {
                continue;
            }

            const auto key = Trim(line.substr(0, separator));
            const auto value = ParseBool(Trim(line.substr(separator + 1)));
            if (!value) {
                continue;
            }

            if (key == "EnableInteractions") {
                config.enableInteractions = *value;
            } else if (key == "EnableEyeTracking") {
                config.enableEyeTracking = *value;
            } else if (key == "EnableCameraAdjustment") {
                config.enableCameraAdjustment = *value;
            } else if (key == "EnableAudioReactions") {
                config.enableAudioReactions = *value;
            } else if (key == "RequireXPMSSE") {
                config.requireXPMSSE = *value;
            } else if (key == "VerboseLogging") {
                config.verboseLogging = *value;
            }
        }

        return config;
    }
}


namespace NexaTouch
{
    bool SaveConfig(const Config& a_config, const std::filesystem::path& a_path)
    {
        std::error_code error;
        std::filesystem::create_directories(a_path.parent_path(), error);
        if (error) {
            return false;
        }

        std::ofstream output(a_path, std::ios::trunc);
        if (!output) {
            return false;
        }

        output << "; NexaTouch Framework configuration\n"
                  "; Settings exposed by the current in-game F1 menu.\n"
                  "EnableInteractions=" << (a_config.enableInteractions ? "true" : "false") << '\n'
               << "EnableEyeTracking=" << (a_config.enableEyeTracking ? "true" : "false") << '\n'
               << "EnableCameraAdjustment=" << (a_config.enableCameraAdjustment ? "true" : "false") << '\n'
               << "EnableAudioReactions=" << (a_config.enableAudioReactions ? "true" : "false") << '\n'
               << "RequireXPMSSE=" << (a_config.requireXPMSSE ? "true" : "false") << '\n'
               << "VerboseLogging=" << (a_config.verboseLogging ? "true" : "false") << '\n';

        output.flush();
        return output.good();
    }
}
