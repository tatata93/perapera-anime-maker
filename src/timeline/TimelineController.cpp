// src/timeline/TimelineController.cpp

#include "timeline/TimelineController.h"

#include "camera/ShotCameraAnimation.h"

#include <algorithm>
#include <cmath>

namespace perapera
{
    int TimelineController::frameAtContentX(
        float contentX,
        float pixelsPerFrame,
        int totalFrames
    ) const
    {
        const float safePixelsPerFrame =
            std::max(1.0f, pixelsPerFrame);
        const int lastFrame = std::max(0, totalFrames - 1);

        return std::clamp(
            static_cast<int>(std::floor(contentX / safePixelsPerFrame)),
            0,
            lastFrame
        );
    }

    float TimelineController::frameCenterX(
        int frame,
        float pixelsPerFrame
    ) const
    {
        return (static_cast<float>(std::max(0, frame)) + 0.5f)
            * std::max(1.0f, pixelsPerFrame);
    }

    int TimelineController::findCameraKeyNearX(
        const ShotCameraAnimation& animation,
        float contentX,
        float pixelsPerFrame,
        float hitRadiusPx
    ) const
    {
        int nearestFrame = -1;
        float nearestDistance = std::max(0.0f, hitRadiusPx);

        for (const ShotCameraKey& key : animation.keys())
        {
            const float keyX = frameCenterX(
                key.timelineFrame,
                pixelsPerFrame
            );
            const float distance = std::abs(contentX - keyX);

            if (distance <= nearestDistance)
            {
                nearestDistance = distance;
                nearestFrame = key.timelineFrame;
            }
        }

        return nearestFrame;
    }

    bool TimelineController::moveCameraKey(
        ShotCameraAnimation& animation,
        int sourceFrame,
        int destinationFrame,
        int totalFrames
    ) const
    {
        if (sourceFrame < 0 || totalFrames <= 0)
        {
            return false;
        }

        const int safeDestination = std::clamp(
            destinationFrame,
            0,
            totalFrames - 1
        );

        if (safeDestination == sourceFrame)
        {
            return false;
        }

        ShotCamera2D camera;
        ShotCameraInterpolation interpolation =
            ShotCameraInterpolation::Smooth;
        bool found = false;

        for (const ShotCameraKey& key : animation.keys())
        {
            if (key.timelineFrame != sourceFrame)
            {
                continue;
            }

            camera = key.camera;
            interpolation = key.interpolation;
            found = true;
            break;
        }

        if (!found)
        {
            return false;
        }

        animation.removeKeyAt(sourceFrame);
        animation.setKey(safeDestination, camera, interpolation);
        return true;
    }

    TimelinePlaybackStep TimelineController::nextPlaybackStep(
        int currentFrame,
        int rangeStart,
        int rangeEnd,
        int totalFrames,
        bool loopEnabled
    ) const
    {
        const int lastFrame = std::max(0, totalFrames - 1);
        int safeStart = std::clamp(rangeStart, 0, lastFrame);
        int safeEnd = std::clamp(rangeEnd, 0, lastFrame);

        if (safeStart > safeEnd)
        {
            std::swap(safeStart, safeEnd);
        }

        if (currentFrame < safeStart || currentFrame > safeEnd)
        {
            return TimelinePlaybackStep{safeStart, false};
        }

        if (currentFrame >= safeEnd)
        {
            return TimelinePlaybackStep{
                loopEnabled ? safeStart : safeEnd,
                !loopEnabled
            };
        }

        return TimelinePlaybackStep{currentFrame + 1, false};
    }
}
