#pragma once

#include <RE/Skyrim.h>

namespace NexaTouch
{
    struct HumanoidSkeletonProfile
    {
        bool hasRoot{ false };
        bool hasHead{ false };
        bool hasLeftHand{ false };
        bool hasRightHand{ false };
        bool hasLeftBreast{ false };
        bool hasRightBreast{ false };
        bool hasBelly{ false };
        bool hasLeftButt{ false };
        bool hasRightButt{ false };
        bool hasLegacyButt{ false };

        [[nodiscard]] bool HasAnyButtNode() const noexcept
        {
            return hasLeftButt || hasRightButt || hasLegacyButt;
        }

        [[nodiscard]] bool HasCoreNodes() const noexcept;
        [[nodiscard]] bool HasExtendedTouchNodes() const noexcept;
    };

    [[nodiscard]] HumanoidSkeletonProfile InspectHumanoidSkeleton(RE::Actor& a_actor);
}
