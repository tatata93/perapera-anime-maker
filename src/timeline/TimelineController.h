// src/timeline/TimelineController.h

#pragma once

namespace perapera
{
    class ShotCameraAnimation;

    struct TimelinePlaybackStep
    {
        int timelineFrame = 0;
        bool shouldStop = false;
    };

    class TimelineController
    {
    public:
        int frameAtContentX(
            float contentX,
            float pixelsPerFrame,
            int totalFrames
        ) const;

        float frameCenterX(int frame, float pixelsPerFrame) const;

        int findCameraKeyNearX(
            const ShotCameraAnimation& animation,
            float contentX,
            float pixelsPerFrame,
            float hitRadiusPx
        ) const;

        bool moveCameraKey(
            ShotCameraAnimation& animation,
            int sourceFrame,
            int destinationFrame,
            int totalFrames
        ) const;

        TimelinePlaybackStep nextPlaybackStep(
            int currentFrame,
            int rangeStart,
            int rangeEnd,
            int totalFrames,
            bool loopEnabled
        ) const;
    };
}
