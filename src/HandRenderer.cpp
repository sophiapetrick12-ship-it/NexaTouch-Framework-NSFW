#include "HandRenderer.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace
{
    RE::NiPointer<RE::NiAVObject> g_handClone;
    RE::NiPointer<RE::NiNode> g_handParent;

    class HandGeometryFilter final : public RE::PerformOpFunc
    {
    public:
        bool operator()(RE::NiAVObject* a_object) override
        {
            if (!a_object) return true;
            auto* geometry = a_object->AsGeometry();
            if (!geometry) return true;

            const char* rawName = geometry->name.c_str();
            std::string name = rawName ? rawName : "";
            std::transform(name.begin(), name.end(), name.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            const bool isHandPart =
                name.find("hand") != std::string::npos ||
                name.find("wrist") != std::string::npos ||
                name.find("forearm") != std::string::npos;

            geometry->SetAppCulled(!isHandPart);
            if (isHandPart) {
                ++visibleHandGeometry;
                spdlog::info("NexaTouch hand clone: retained player geometry '{}'.", rawName ? rawName : "<unnamed>");
            }
            return true;
        }

        std::uint32_t visibleHandGeometry{ 0 };
    };
}

namespace NexaTouch
{
    void DetachPlayerHandFromTarget()
    {
        if (g_handParent && g_handClone) {
            g_handParent->DetachChild(g_handClone.get());
            spdlog::info("NexaTouch player hand clone detached from target.");
        }
        g_handClone.reset();
        g_handParent.reset();
    }

    bool AttachPlayerHandToTarget(RE::Actor& a_player, RE::Actor& a_target)
    {
        DetachPlayerHandFromTarget();

        auto* playerRoot = a_player.Get3D();
        auto* targetRoot = a_target.Get3D();
        if (!playerRoot || !targetRoot) {
            spdlog::warn("Pet Head hand clone skipped: player or target 3D is unavailable.");
            return false;
        }

        auto* playerHandBone = playerRoot->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));
        auto* targetHeadObject = targetRoot->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));
        auto* targetHead = targetHeadObject ? targetHeadObject->AsNode() : nullptr;
        if (!playerHandBone || !targetHead) {
            spdlog::warn("Pet Head hand clone skipped: XPMSSE player right-hand bone or target head node is missing.");
            return false;
        }

        auto* clonedObject = playerRoot->Clone();
        auto* clonedRoot = clonedObject ? static_cast<RE::NiAVObject*>(clonedObject) : nullptr;
        if (!clonedRoot || !clonedRoot->AsNode()) {
            spdlog::error("Pet Head hand clone failed: player scene graph could not be cloned as a node.");
            if (clonedObject) clonedObject->DecRefCount();
            return false;
        }

        auto* clonedHandBone = clonedRoot->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));
        if (!clonedHandBone) {
            spdlog::error("Pet Head hand clone failed: cloned right-hand bone is missing.");
            clonedRoot->DecRefCount();
            return false;
        }

        HandGeometryFilter filter;
        clonedRoot->PerformOp(filter);
        if (filter.visibleHandGeometry == 0) {
            spdlog::error("Pet Head hand clone aborted safely: no separately named hand/wrist/forearm geometry was found in the player's live 3D. No substitute mesh was shown.");
            clonedRoot->DecRefCount();
            return false;
        }

        // Preserve the player's current skin/material/tint and cloned skin-bone
        // links. Place the cloned right-hand bone at the target head's local
        // origin; the small offset brings the palm slightly above the head pivot.
        const RE::NiTransform handLocal = clonedRoot->world.Invert() * clonedHandBone->world;
        RE::NiTransform contactOffset;
        contactOffset.translate = RE::NiPoint3(0.0F, 0.0F, 7.0F);
        clonedRoot->local = contactOffset * handLocal.Invert();

        targetHead->AttachChild(clonedRoot, false);
        g_handClone.reset(clonedRoot);
        g_handParent.reset(targetHead);

        spdlog::info("Pet Head hand clone attached: {} player hand geometry object(s) retained; non-hand player geometry culled.", filter.visibleHandGeometry);
        return true;
    }
}
