#include "Config.h"
#include "InteractionController.h"
#include "SKSEMenuFramework.h"

#include <spdlog/spdlog.h>

#include <array>
#include <cstdint>
#include <filesystem>

namespace
{
    NexaTouch::Config* g_config{ nullptr };
    std::filesystem::path g_configPath;
    bool g_registered{ false };

    struct KeyMap { ImGuiMCP::ImGuiKey key; std::uint32_t scanCode; };
    constexpr std::array kKeyMap{
        KeyMap{ ImGuiMCP::ImGuiKey_A, 0x1E }, KeyMap{ ImGuiMCP::ImGuiKey_B, 0x30 },
        KeyMap{ ImGuiMCP::ImGuiKey_C, 0x2E }, KeyMap{ ImGuiMCP::ImGuiKey_D, 0x20 },
        KeyMap{ ImGuiMCP::ImGuiKey_E, 0x12 }, KeyMap{ ImGuiMCP::ImGuiKey_F, 0x21 },
        KeyMap{ ImGuiMCP::ImGuiKey_G, 0x22 }, KeyMap{ ImGuiMCP::ImGuiKey_H, 0x23 },
        KeyMap{ ImGuiMCP::ImGuiKey_I, 0x17 }, KeyMap{ ImGuiMCP::ImGuiKey_J, 0x24 },
        KeyMap{ ImGuiMCP::ImGuiKey_K, 0x25 }, KeyMap{ ImGuiMCP::ImGuiKey_L, 0x26 },
        KeyMap{ ImGuiMCP::ImGuiKey_M, 0x32 }, KeyMap{ ImGuiMCP::ImGuiKey_N, 0x31 },
        KeyMap{ ImGuiMCP::ImGuiKey_O, 0x18 }, KeyMap{ ImGuiMCP::ImGuiKey_P, 0x19 },
        KeyMap{ ImGuiMCP::ImGuiKey_Q, 0x10 }, KeyMap{ ImGuiMCP::ImGuiKey_R, 0x13 },
        KeyMap{ ImGuiMCP::ImGuiKey_S, 0x1F }, KeyMap{ ImGuiMCP::ImGuiKey_T, 0x14 },
        KeyMap{ ImGuiMCP::ImGuiKey_U, 0x16 }, KeyMap{ ImGuiMCP::ImGuiKey_V, 0x2F },
        KeyMap{ ImGuiMCP::ImGuiKey_W, 0x11 }, KeyMap{ ImGuiMCP::ImGuiKey_X, 0x2D },
        KeyMap{ ImGuiMCP::ImGuiKey_Y, 0x15 }, KeyMap{ ImGuiMCP::ImGuiKey_Z, 0x2C },
        KeyMap{ ImGuiMCP::ImGuiKey_0, 0x0B }, KeyMap{ ImGuiMCP::ImGuiKey_1, 0x02 },
        KeyMap{ ImGuiMCP::ImGuiKey_2, 0x03 }, KeyMap{ ImGuiMCP::ImGuiKey_3, 0x04 },
        KeyMap{ ImGuiMCP::ImGuiKey_4, 0x05 }, KeyMap{ ImGuiMCP::ImGuiKey_5, 0x06 },
        KeyMap{ ImGuiMCP::ImGuiKey_6, 0x07 }, KeyMap{ ImGuiMCP::ImGuiKey_7, 0x08 },
        KeyMap{ ImGuiMCP::ImGuiKey_8, 0x09 }, KeyMap{ ImGuiMCP::ImGuiKey_9, 0x0A },
        KeyMap{ ImGuiMCP::ImGuiKey_F1, 0x3B }, KeyMap{ ImGuiMCP::ImGuiKey_F2, 0x3C },
        KeyMap{ ImGuiMCP::ImGuiKey_F3, 0x3D }, KeyMap{ ImGuiMCP::ImGuiKey_F4, 0x3E },
        KeyMap{ ImGuiMCP::ImGuiKey_F5, 0x3F }, KeyMap{ ImGuiMCP::ImGuiKey_F6, 0x40 },
        KeyMap{ ImGuiMCP::ImGuiKey_F7, 0x41 }, KeyMap{ ImGuiMCP::ImGuiKey_F8, 0x42 },
        KeyMap{ ImGuiMCP::ImGuiKey_F9, 0x43 }, KeyMap{ ImGuiMCP::ImGuiKey_F10, 0x44 },
        KeyMap{ ImGuiMCP::ImGuiKey_F11, 0x57 }, KeyMap{ ImGuiMCP::ImGuiKey_F12, 0x58 },
        KeyMap{ ImGuiMCP::ImGuiKey_Space, 0x39 }, KeyMap{ ImGuiMCP::ImGuiKey_Enter, 0x1C },
        KeyMap{ ImGuiMCP::ImGuiKey_Tab, 0x0F }, KeyMap{ ImGuiMCP::ImGuiKey_Backspace, 0x0E },
        KeyMap{ ImGuiMCP::ImGuiKey_Escape, 0x01 }
    };

    ImGuiMCP::ImGuiKey ScanCodeToKey(std::uint32_t scanCode)
    {
        for (const auto& entry : kKeyMap) if (entry.scanCode == scanCode) return entry.key;
        return ImGuiMCP::ImGuiKey_None;
    }

    std::uint32_t KeyToScanCode(ImGuiMCP::ImGuiKey key)
    {
        for (const auto& entry : kKeyMap) if (entry.key == key) return entry.scanCode;
        return 0;
    }

    void SaveSettings()
    {
        if (g_config && !NexaTouch::SaveConfig(*g_config, g_configPath)) {
            spdlog::error("SKSE Menu Framework settings page: failed to save NexaTouchFramework.ini.");
        }
    }

    void __stdcall RenderSettingsPage()
    {
        if (!g_config) {
            ImGuiMCP::Text("NexaTouch configuration is unavailable.");
            return;
        }

        ImGuiMCP::Text("NexaTouch Framework");
        ImGuiMCP::Text("Interaction controls");

        bool enabled = g_config->enableInteractions;
        if (ImGuiMCP::Checkbox("Enable target selection", &enabled)) {
            NexaTouch::SetInteractionEnabled(enabled);
            SaveSettings();
        }

        auto selectedKey = ScanCodeToKey(g_config->interactionKeyScanCode);
        if (ImGuiMCPComponents::KeyBindingPicker("Interaction key", &selectedKey)) {
            const auto scanCode = KeyToScanCode(selectedKey);
            if (scanCode != 0) {
                g_config->interactionKeyScanCode = scanCode;
                SaveSettings();
            }
        }

        bool verbose = g_config->verboseLogging;
        if (ImGuiMCP::Checkbox("Verbose logging", &verbose)) {
            g_config->verboseLogging = verbose;
            spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);
            SaveSettings();
        }

        ImGuiMCP::Text("Current stage: safe NPC target selection only.");
        ImGuiMCP::Text("Hand model, body-zone selection, touch animations and NPC reactions are not implemented yet.");
        ImGuiMCP::Text("XPMSSE is the required humanoid skeleton baseline.");
    }
}

namespace NexaTouch
{
    void RegisterSettingsMenu(Config& a_config, const std::filesystem::path& a_configPath)
    {
        g_config = &a_config;
        g_configPath = a_configPath;
        if (g_registered) return;

        if (!SKSEMenuFramework::IsInstalled()) {
            spdlog::warn("SKSE Menu Framework is not loaded; NexaTouch settings page was not registered.");
            return;
        }

        SKSEMenuFramework::SetSection("NexaTouch Framework");
        SKSEMenuFramework::AddSectionItem("Settings", &RenderSettingsPage);
        g_registered = true;
        spdlog::info("NexaTouch settings page registered in SKSE Menu Framework.");
    }
}
