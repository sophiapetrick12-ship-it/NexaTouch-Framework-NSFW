#include "Config.h"

#include <algorithm>
#include <charconv>
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
        if (a_value == "true" || a_value == "1" || a_value == "yes" || a_value == "on") return true;
        if (a_value == "false" || a_value == "0" || a_value == "no" || a_value == "off") return false;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::uint32_t> ParseUInt(const std::string& value)
    {
        std::uint32_t parsed{};
        const auto* begin = value.data();
        const auto* end = begin + value.size();
        const auto result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc{} || result.ptr != end) return std::nullopt;
        return parsed;
    }
}

namespace NexaTouch
{
    Config LoadConfig(const std::filesystem::path& a_path)
    {
        Config config;
        std::error_code error;
        if (!a_path.parent_path().empty()) std::filesystem::create_directories(a_path.parent_path(), error);

        if (!std::filesystem::exists(a_path, error)) {
            std::ofstream output(a_path);
            if (output) {
                output << "; NexaTouch Framework configuration\n"
                          "EnableInteractions=false\n"
                          "InteractionKeyScanCode=34\n"
                          "EnableEyeTracking=true\n"
                          "EnableCameraAdjustment=false\n"
                          "EnableAudioReactions=true\n"
                          "RequireXPMSSE=true\n"
                          "VerboseLogging=false\n";
            }
        }

        std::ifstream input(a_path);
        if (!input) return config;

        std::string line;
        while (std::getline(input, line)) {
            line = Trim(std::move(line));
            if (line.empty() || line.front() == ';' || line.front() == '#' || line.front() == '[') continue;
            const auto separator = line.find('=');
            if (separator == std::string::npos) continue;

            const auto key = Trim(line.substr(0, separator));
            const auto rawValue = Trim(line.substr(separator + 1));
            if (key == "InteractionKeyScanCode") {
                if (const auto value = ParseUInt(rawValue); value && *value <= 0xFF) config.interactionKeyScanCode = *value;
                continue;
            }
            const auto value = ParseBool(rawValue);
            if (!value) continue;
            if (key == "EnableInteractions") config.enableInteractions = *value;
            else if (key == "EnableEyeTracking") config.enableEyeTracking = *value;
            else if (key == "EnableCameraAdjustment") config.enableCameraAdjustment = *value;
            else if (key == "EnableAudioReactions") config.enableAudioReactions = *value;
            else if (key == "RequireXPMSSE") config.requireXPMSSE = *value;
            else if (key == "VerboseLogging") config.verboseLogging = *value;
        }
        return config;
    }

    bool SaveConfig(const Config& a_config, const std::filesystem::path& a_path)
    {
        std::error_code error;
        if (!a_path.parent_path().empty()) std::filesystem::create_directories(a_path.parent_path(), error);
        if (error) return false;

        std::ofstream output(a_path, std::ios::trunc);
        if (!output) return false;
        output << "; NexaTouch Framework configuration\n"
                  "EnableInteractions=" << (a_config.enableInteractions ? "true" : "false") << '\n'
               << "InteractionKeyScanCode=" << a_config.interactionKeyScanCode << '\n'
               << "EnableEyeTracking=" << (a_config.enableEyeTracking ? "true" : "false") << '\n'
               << "EnableCameraAdjustment=" << (a_config.enableCameraAdjustment ? "true" : "false") << '\n'
               << "EnableAudioReactions=" << (a_config.enableAudioReactions ? "true" : "false") << '\n'
               << "RequireXPMSSE=" << (a_config.requireXPMSSE ? "true" : "false") << '\n'
               << "VerboseLogging=" << (a_config.verboseLogging ? "true" : "false") << '\n';
        output.flush();
        return output.good();
    }
}
