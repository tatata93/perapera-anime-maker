// src/timeline/TimelineViewState.cpp

#include "timeline/TimelineViewState.h"

#include <algorithm>

namespace perapera
{
    void TimelineViewState::clampToTimeline(int totalFrames)
    {
        pixelsPerFrame = std::clamp(pixelsPerFrame, 8.0f, 96.0f);

        const int lastFrame = std::max(0, totalFrames - 1);
        const int previousLastFrame =
            std::max(0, knownTotalFrames - 1);
        const bool rangeCoveredWholeTimeline =
            initialized
            && playbackRangeStart == 0
            && playbackRangeEnd == previousLastFrame;

        if (!initialized || rangeCoveredWholeTimeline)
        {
            playbackRangeStart = 0;
            playbackRangeEnd = lastFrame;
            initialized = true;
        }

        knownTotalFrames = std::max(0, totalFrames);

        playbackRangeStart = std::clamp(playbackRangeStart, 0, lastFrame);
        playbackRangeEnd = std::clamp(playbackRangeEnd, 0, lastFrame);

        if (playbackRangeStart > playbackRangeEnd)
        {
            std::swap(playbackRangeStart, playbackRangeEnd);
        }

        if (selectedCameraKeyFrame > lastFrame)
        {
            selectedCameraKeyFrame = -1;
        }
    }

    float TimelineViewState::contentWidth(int totalFrames) const
    {
        return static_cast<float>(std::max(1, totalFrames))
            * std::clamp(pixelsPerFrame, 8.0f, 96.0f);
    }
}
