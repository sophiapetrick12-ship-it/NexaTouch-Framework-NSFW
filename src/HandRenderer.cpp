#include "HandRenderer.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace
{
    RE::NiPointer<RE::NiAVObject> g_handClone;
    RE::NiPointer<RE::NiNode> g_handParent;
    RE::NiTransform g_handBoneInCloneRoot;
    bool g_handAttached{ false };
    RE::ObjectRefHandle g_handTarget;

    struct HandGeometryFilter
    {
        std::uint32_t visibleHandGeometry{ 0 };

        void Visit(RE::NiAVObject* a_object)
        {
            if (!a_object) return;

            if (auto* geometry = a_object->AsGeometry()) {
                const char* raw = geometry->name.c_str();
                std::string name = raw ? raw : "";
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });

                // Skyrim hand meshes are not consistently named "hand": the
                // equipped mesh can be FemaleHands/MaleHands. Keep those too.
                const bool keep =
                    name.find("hand") != std::string::npos ||
                    name.find("hands") != std::string::npos ||
                    name.find("wrist") != std::string::npos ||
                    name.find("forearm") != std::string::npos;
                geometry->SetAppCulled(!keep);
                if (keep) ++visibleHandGeometry;
            }

            if (auto* node = a_object->AsNode()) {
                auto& children = node->GetChildren();
                for (std::uint16_t i = 0; i < children.size(); ++i) {
                    if (children[i]) Visit(children[i].get());
                }
            }
        }
    };

    bool ReadHeadContact(RE::Actor& a_target, RE::NiPoint3& a_localContact) noexcept
    {
        auto* pickData = RE::CrosshairPickData::GetSingleton();
        auto* targetRoot = a_target.Get3D();
        if (!pickData || !targetRoot) return false;

        // Only accept a real engine crosshair hit on this exact actor.
        // This avoids activating Pet Head from a screen-space guess.
        auto pickedReference = pickData->GetActiveTarget().get();
        if (pickedReference != &a_target) return false;

        auto* headObject = targetRoot->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));
        auto* head = headObject ? headObject->AsNode() : nullptr;
        if (!head) return false;

        const auto& point = pickData->collisionPoint;
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) return false;

        // The engine supplies a world-space Havok/crosshair contact point.
        // Convert it into the head node's local space for the attached hand.
        if (point.GetDistance(head->world.translate) > 24.0F) return false;
        a_localContact = head->world.Invert() * point;
        return std::isfinite(a_localContact.x) &&
               std::isfinite(a_localContact.y) &&
               std::isfinite(a_localContact.z);
    }

    void PlaceHandAtContact(const RE::NiPoint3& a_localContact) noexcept
    {
        if (!g_handAttached || !g_handClone || !g_handParent) return;

        RE::NiTransform contact;
        contact.rotate = RE::NiMatrix3();
        // A tiny lift prevents z-fighting while keeping the palm at the hit.
        contact.translate = a_localContact + RE::NiPoint3(0.0F, 0.0F, 0.8F);
        g_handClone->local = contact * g_handBoneInCloneRoot.Invert();
    }
}

namespace NexaTouch
{
    bool IsPetHeadContact(RE::Actor& a_target, RE::NiPoint3& a_localContact) noexcept
    {
        return ReadHeadContact(a_target, a_localContact);
    }

    void DetachPlayerHandFromTarget()
    {
        if (g_handParent && g_handClone) {
            g_handParent->DetachChild(g_handClone.get());
        }
        g_handClone.reset();
        g_handParent.reset();
        g_handAttached = false;
        g_handTarget.reset();
        g_handBoneInCloneRoot = RE::NiTransform();
    }

    bool AttachPlayerHandToTarget(RE::Actor& a_player, RE::Actor& a_target)
    {
        DetachPlayerHandFromTarget();

        auto* playerRoot = a_player.Get3D();
        auto* targetRoot = a_target.Get3D();
        if (!playerRoot || !targetRoot) return false;

        auto* playerHand = playerRoot->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));
        auto* headObject = targetRoot->GetObjectByName(RE::BSFixedString("NPC Head [Head]"));
        auto* head = headObject ? headObject->AsNode() : nullptr;
        if (!playerHand || !head) {
            spdlog::warn("Pet Head unavailable: XPMSSE right-hand bone or target head node missing.");
            return false;
        }

        RE::NiPointer<RE::NiAVObject> clone{ static_cast<RE::NiAVObject*>(playerRoot->Clone()) };
        if (!clone || !clone->AsNode()) return false;

        auto* clonedHand = clone->GetObjectByName(RE::BSFixedString("NPC R Hand [RHnd]"));
        if (!clonedHand) {
            spdlog::warn("Pet Head unavailable: cloned player hand bone was not found.");
            return false;
        }

        HandGeometryFilter filter;
        filter.Visit(clone.get());
        if (filter.visibleHandGeometry == 0) {
            spdlog::warn("Pet Head unavailable: equipped player hand mesh was not found in the cloned player 3D.");
            return false;
        }

        // Preserve the hand bone's transform relative to the cloned root.
        // The clone is then positioned so that this bone lands on the actual
        // head contact point, rather than moving the clone by screen pixels.
        g_handBoneInCloneRoot = clone->world.Invert() * clonedHand->world;
        head->AttachChild(clone.get(), false);
        g_handClone = clone;
        g_handParent.reset(head);
        g_handTarget = a_target.GetHandle();
        g_handAttached = true;

        UpdatePetHeadHand();
        spdlog::info("Pet Head: equipped player hand clone attached ({} visible hand/wrist/forearm geometry objects).",
            filter.visibleHandGeometry);
        return true;
    }

    void UpdatePetHeadHand() noexcept
    {
        if (!g_handAttached || !g_handClone) return;
        auto targetNode = g_handParent;
        if (!targetNode) return;

        // The attached parent is the target's head. Resolve the actor through
        // the live crosshair hit on every update, so the hand follows contact.
        auto targetReference = g_handTarget.get();
        auto* actor = targetReference ? targetReference->As<RE::Actor>() : nullptr;
        if (!actor) return;

        RE::NiPoint3 localContact;
        if (ReadHeadContact(*actor, localContact)) {
            PlaceHandAtContact(localContact);
        }
    }
}
