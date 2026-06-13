// src/timeline/TimelineViewState.h

#pragma once

namespace perapera
{
    class TimelineViewState
    {
    public:
        float pixelsPerFrame = 28.0f;
        bool playbackRangeEnabled = false;
        int playbackRangeStart = 0;
        int playbackRangeEnd = 0;
        int selectedCameraKeyFrame = -1;
        bool initialized = false;
        int knownTotalFrames = 0;

        void clampToTimeline(int totalFrames);

        float contentWidth(int totalFrames) const;
    };
}
