#include "Config.h"
#include "InteractionController.h"
#include "SKSEMenuFramework.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>

namespace
{
    NexaTouch::Config* g_config{ nullptr };
    std::filesystem::path g_configPath;
    bool g_registered{ false };
    SKSEMenuFramework::Model::HudElement* g_interactionHud{ nullptr };
    bool g_holdTracking{ false };
    bool g_waitingForKey{ false };
    bool g_holdCompleted{ false };
    bool g_petHeadActive{ false };
    std::chrono::steady_clock::time_point g_holdStarted{};

    void __stdcall RenderInteractionCursor()
    {
        if (!NexaTouch::IsInteractionActive() || SKSEMenuFramework::IsAnyBlockingWindowOpened()) {
            if (g_petHeadActive) NexaTouch::EndPetHead();
            g_petHeadActive = false;
            g_holdTracking = false;
            g_holdCompleted = false;
            return;
        }

        const auto mouse = ImGuiMCP::GetMousePos();
        if (!std::isfinite(mouse.x) || !std::isfinite(mouse.y) || mouse.x < -1000.0F || mouse.y < -1000.0F) {
            if (g_petHeadActive) NexaTouch::EndPetHead();
            g_petHeadActive = false;
            g_holdTracking = false;
            g_holdCompleted = false;
            return;
        }

        constexpr auto leftButton = static_cast<ImGuiMCP::ImGuiMouseButton>(ImGuiMCP::ImGuiMouseButton_Left);
        const auto now = std::chrono::steady_clock::now();
        auto* io = ImGuiMCP::GetIO();\n        const float width = io && io->DisplaySize.x > 1.0F ? io->DisplaySize.x : 1.0F;\n        const float height = io && io->DisplaySize.y > 1.0F ? io->DisplaySize.y : 1.0F;\n        const bool onHead = NexaTouch::IsPetHeadCursorTarget(mouse.x, mouse.y, width, height);
        if (!onHead) {
            if (g_petHeadActive) NexaTouch::EndPetHead();
            g_petHeadActive = false; g_holdTracking = false; g_holdCompleted = false;
        }
        if (onHead && ImGuiMCP::IsMouseDown(leftButton)) {
            if (!g_holdTracking) {
                g_holdTracking = true;
                g_holdCompleted = false;
                g_holdStarted = now;
            }
            const auto elapsed = std::chrono::duration<float>(now - g_holdStarted).count();
            if (elapsed >= 1.0F && !g_holdCompleted) { g_petHeadActive = NexaTouch::BeginPetHead(mouse.x, mouse.y, width, height); g_holdCompleted = g_petHeadActive; }
        } else {
            if (g_petHeadActive) NexaTouch::EndPetHead();
            g_petHeadActive = false; g_holdTracking = false; g_holdCompleted = false;
        }

        if (g_petHeadActive) {
            auto* io = ImGuiMCP::GetIO();
            const float width = io && io->DisplaySize.x > 1.0F ? io->DisplaySize.x : 1.0F;
            const float height = io && io->DisplaySize.y > 1.0F ? io->DisplaySize.y : 1.0F;
            const float horizontal = (mouse.x / width - 0.5F) * 2.0F;
            const float vertical = (mouse.y / height - 0.5F) * 2.0F;
            const float elapsed = std::chrono::duration<float>(now - g_holdStarted).count();
            const float stroke = 0.5F + 0.5F * std::sin(elapsed * 7.0F);
            NexaTouch::UpdatePetHead(horizontal, vertical, stroke, mouse.x, mouse.y, width, height);
        }

        float progress = 0.0F;
        if (g_holdTracking) {
            const auto elapsed = std::chrono::duration<float>(now - g_holdStarted).count();
            progress = (std::min)(elapsed, 1.0F);
        }

        auto* drawList = ImGuiMCP::GetForegroundDrawList();
        if (!drawList) return;

        // Do not show instructional text or a selector merely because G was
        // pressed. The label and ring are visible only while the current
        // head-contact test reports a valid head target.
        if (!onHead) return;

        ImGuiMCP::ImDrawListManager::AddText(
            drawList,
            ImGuiMCP::ImVec2{ mouse.x + 20.0F, mouse.y + 12.0F },
            IM_COL32(255, 255, 255, 245),
            g_petHeadActive ? "Pet Head" : "Pet Head - hold 1s");

        constexpr float kCursorRingRadius = 17.0F;
        constexpr float kPi = 3.14159265358979323846F;
        const ImGuiMCP::ImVec2 center{ mouse.x, mouse.y };

        ImGuiMCP::ImDrawListManager::AddCircle(
            drawList, center, kCursorRingRadius, IM_COL32(255, 255, 255, 235), 48, 2.0F);

        if (progress > 0.0F) {
            ImGuiMCP::ImDrawListManager::PathClear(drawList);
            ImGuiMCP::ImDrawListManager::PathArcTo(
                drawList, center, kCursorRingRadius, -kPi * 0.5F,
                -kPi * 0.5F + (2.0F * kPi * progress), 48);
            ImGuiMCP::ImDrawListManager::PathStroke(
                drawList, g_holdCompleted ? IM_COL32(90, 255, 150, 255) : IM_COL32(90, 190, 255, 255),
                static_cast<ImGuiMCP::ImDrawFlags>(0), 3.5F);
            ImGuiMCP::ImDrawListManager::PathClear(drawList);
        }
    }

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
        KeyMap{ ImGuiMCP::ImGuiKey_Escape, 0x01 },
        KeyMap{ ImGuiMCP::ImGuiKey_Apostrophe, 0x28 }, KeyMap{ ImGuiMCP::ImGuiKey_Comma, 0x33 },
        KeyMap{ ImGuiMCP::ImGuiKey_Minus, 0x0C }, KeyMap{ ImGuiMCP::ImGuiKey_Period, 0x34 },
        KeyMap{ ImGuiMCP::ImGuiKey_Slash, 0x35 }, KeyMap{ ImGuiMCP::ImGuiKey_Semicolon, 0x27 },
        KeyMap{ ImGuiMCP::ImGuiKey_Equal, 0x0D }, KeyMap{ ImGuiMCP::ImGuiKey_LeftBracket, 0x1A },
        KeyMap{ ImGuiMCP::ImGuiKey_Backslash, 0x2B }, KeyMap{ ImGuiMCP::ImGuiKey_RightBracket, 0x1B },
        KeyMap{ ImGuiMCP::ImGuiKey_GraveAccent, 0x29 }, KeyMap{ ImGuiMCP::ImGuiKey_CapsLock, 0x3A },
        KeyMap{ ImGuiMCP::ImGuiKey_NumLock, 0x45 },
        KeyMap{ ImGuiMCP::ImGuiKey_Keypad0, 0x52 }, KeyMap{ ImGuiMCP::ImGuiKey_Keypad1, 0x4F },
        KeyMap{ ImGuiMCP::ImGuiKey_Keypad2, 0x50 }, KeyMap{ ImGuiMCP::ImGuiKey_Keypad3, 0x51 },
        KeyMap{ ImGuiMCP::ImGuiKey_Keypad4, 0x4B }, KeyMap{ ImGuiMCP::ImGuiKey_Keypad5, 0x4C },
        KeyMap{ ImGuiMCP::ImGuiKey_Keypad6, 0x4D }, KeyMap{ ImGuiMCP::ImGuiKey_Keypad7, 0x47 },
        KeyMap{ ImGuiMCP::ImGuiKey_Keypad8, 0x48 }, KeyMap{ ImGuiMCP::ImGuiKey_Keypad9, 0x49 },
        KeyMap{ ImGuiMCP::ImGuiKey_KeypadDecimal, 0x53 }, KeyMap{ ImGuiMCP::ImGuiKey_KeypadDivide, 0xB5 },
        KeyMap{ ImGuiMCP::ImGuiKey_KeypadMultiply, 0x37 }, KeyMap{ ImGuiMCP::ImGuiKey_KeypadSubtract, 0x4A },
        KeyMap{ ImGuiMCP::ImGuiKey_KeypadAdd, 0x4E }
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

        if (ImGuiMCP::Button(g_waitingForKey ? "Press a key..." : "Change interaction key")) {
            g_waitingForKey = true;
        }
        if (g_waitingForKey) {
            ImGuiMCP::Text("Press any supported keyboard key to bind it.");
            for (const auto& entry : kKeyMap) {
                if (ImGuiMCP::IsKeyPressed(entry.key, false)) {
                    g_config->interactionKeyScanCode = entry.scanCode;
                    g_waitingForKey = false;
                    SaveSettings();
                    break;
                }
            }
        }
        const auto selectedKey = ScanCodeToKey(g_config->interactionKeyScanCode);
        const auto* keyName = selectedKey != ImGuiMCP::ImGuiKey_None ? ImGuiMCP::GetKeyName(selectedKey) : "Unknown";
        ImGuiMCP::Text("Current interaction key: %s", keyName ? keyName : "Unknown");

        bool verbose = g_config->verboseLogging;
        if (ImGuiMCP::Checkbox("Verbose logging", &verbose)) {
            g_config->verboseLogging = verbose;
            spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);
            SaveSettings();
        }

        ImGuiMCP::Text("Pet Head: aim at the NPC head, then hold left mouse for one second.");
        ImGuiMCP::Text("The player hand follows the cursor across the head contact area while held.");
        ImGuiMCP::Text("Release left mouse to withdraw the hand; press G to leave the interaction session.");
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
        g_interactionHud = SKSEMenuFramework::AddHudElement(&RenderInteractionCursor);
        if (g_interactionHud) {
            spdlog::info("NexaTouch interaction cursor HUD registered: fixed mouse ring and one-second hold progress.");
        } else {
            spdlog::error("Failed to register NexaTouch interaction cursor HUD.");
        }
        g_registered = true;
        spdlog::info("NexaTouch settings page registered in SKSE Menu Framework.");
    }
}
