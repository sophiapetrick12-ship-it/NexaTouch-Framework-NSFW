#include "SkeletonProbe.h"

namespace NexaTouch
{
    bool HumanoidSkeletonProfile::HasCoreNodes() const noexcept
    {
        return hasRoot && hasHead && hasLeftHand && hasRightHand;
    }

    bool HumanoidSkeletonProfile::HasExtendedTouchNodes() const noexcept
    {
        return HasCoreNodes() && hasLeftBreast && hasRightBreast && hasBelly && hasButt;
    }

    HumanoidSkeletonProfile InspectHumanoidSkeleton(RE::Actor& a_actor)
    {
        HumanoidSkeletonProfile profile;
        auto root = a_actor.Get3D();
        if (!root) {
            return profile;
        }

        const auto hasNode = [root](const char* a_name) {
            return root->GetObjectByName(RE::BSFixedString(a_name)) != nullptr;
        };

        profile.hasRoot = hasNode("NPC Root [Root]");
        profile.hasHead = hasNode("NPC Head [Head]");
        profile.hasLeftHand = hasNode("NPC L Hand [LHnd]");
        profile.hasRightHand = hasNode("NPC R Hand [RHnd]");
        profile.hasLeftBreast = hasNode("NPC L Breast");
        profile.hasRightBreast = hasNode("NPC R Breast");
        profile.hasBelly = hasNode("NPC Belly");
        profile.hasButt = hasNode("NPC Butt");
        return profile;
    }
}
