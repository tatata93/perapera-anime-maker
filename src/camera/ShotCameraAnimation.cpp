// src/camera/ShotCameraAnimation.cpp

#include "camera/ShotCameraAnimation.h"

#include <algorithm>
#include <utility>

namespace perapera
{
    namespace
    {
        float interpolate(float a, float b, float t)
        {
            return a + (b - a) * t;
        }

        float smoothStep(float t)
        {
            const float clamped = std::clamp(t, 0.0f, 1.0f);
            return clamped * clamped * (3.0f - 2.0f * clamped);
        }
    }

    const std::vector<ShotCameraKey>& ShotCameraAnimation::keys() const
    {
        return keys_;
    }

    bool ShotCameraAnimation::empty() const
    {
        return keys_.empty();
    }

    bool ShotCameraAnimation::hasKeyAt(int timelineFrame) const
    {
        return std::any_of(
            keys_.begin(),
            keys_.end(),
            [timelineFrame](const ShotCameraKey& key)
            {
                return key.timelineFrame == timelineFrame;
            }
        );
    }

    void ShotCameraAnimation::clear()
    {
        keys_.clear();
    }

    void ShotCameraAnimation::setKey(
        int timelineFrame,
        const ShotCamera2D& camera,
        ShotCameraInterpolation interpolation
    )
    {
        const int safeTimelineFrame = std::max(0, timelineFrame);

        for (ShotCameraKey& key : keys_)
        {
            if (key.timelineFrame == safeTimelineFrame)
            {
                key.camera = camera;
                key.interpolation = interpolation;
                return;
            }
        }

        keys_.push_back(ShotCameraKey{
            safeTimelineFrame,
            camera,
            interpolation
        });

        sortAndMergeKeys();
    }

    bool ShotCameraAnimation::removeKeyAt(int timelineFrame)
    {
        const auto oldEnd = keys_.end();
        const auto newEnd = std::remove_if(
            keys_.begin(),
            oldEnd,
            [timelineFrame](const ShotCameraKey& key)
            {
                return key.timelineFrame == timelineFrame;
            }
        );

        if (newEnd == oldEnd)
        {
            return false;
        }

        keys_.erase(newEnd, oldEnd);
        return true;
    }

    ShotCamera2D ShotCameraAnimation::evaluate(
        int timelineFrame,
        const ShotCamera2D& fallbackCamera
    ) const
    {
        if (keys_.empty())
        {
            return fallbackCamera;
        }

        if (timelineFrame <= keys_.front().timelineFrame)
        {
            return keys_.front().camera;
        }

        if (timelineFrame >= keys_.back().timelineFrame)
        {
            return keys_.back().camera;
        }

        const auto nextKeyIterator = std::upper_bound(
            keys_.begin(),
            keys_.end(),
            timelineFrame,
            [](int frame, const ShotCameraKey& key)
            {
                return frame < key.timelineFrame;
            }
        );

        const ShotCameraKey& nextKey = *nextKeyIterator;
        const ShotCameraKey& previousKey = *(nextKeyIterator - 1);

        if (previousKey.interpolation == ShotCameraInterpolation::Hold)
        {
            return previousKey.camera;
        }

        const int frameDistance =
            std::max(1, nextKey.timelineFrame - previousKey.timelineFrame);

        float t = static_cast<float>(timelineFrame - previousKey.timelineFrame)
            / static_cast<float>(frameDistance);

        if (previousKey.interpolation == ShotCameraInterpolation::Smooth)
        {
            t = smoothStep(t);
        }

        ShotCamera2D result;
        result.centerX = interpolate(previousKey.camera.centerX, nextKey.camera.centerX, t);
        result.centerY = interpolate(previousKey.camera.centerY, nextKey.camera.centerY, t);
        result.zoom = interpolate(previousKey.camera.zoom, nextKey.camera.zoom, t);
        return result;
    }

    void ShotCameraAnimation::insertTimelineFrames(int firstFrame, int frameCount)
    {
        if (frameCount <= 0)
        {
            return;
        }

        const int safeFirstFrame = std::max(0, firstFrame);

        for (ShotCameraKey& key : keys_)
        {
            if (key.timelineFrame >= safeFirstFrame)
            {
                key.timelineFrame += frameCount;
            }
        }
    }

    void ShotCameraAnimation::removeTimelineFrames(int firstFrame, int frameCount)
    {
        if (frameCount <= 0)
        {
            return;
        }

        const int safeFirstFrame = std::max(0, firstFrame);
        const int removedEnd = safeFirstFrame + frameCount;

        keys_.erase(
            std::remove_if(
                keys_.begin(),
                keys_.end(),
                [safeFirstFrame, removedEnd](const ShotCameraKey& key)
                {
                    return key.timelineFrame >= safeFirstFrame
                        && key.timelineFrame < removedEnd;
                }
            ),
            keys_.end()
        );

        for (ShotCameraKey& key : keys_)
        {
            if (key.timelineFrame >= removedEnd)
            {
                key.timelineFrame -= frameCount;
            }
        }

        sortAndMergeKeys();
    }

    void ShotCameraAnimation::sortAndMergeKeys()
    {
        std::stable_sort(
            keys_.begin(),
            keys_.end(),
            [](const ShotCameraKey& a, const ShotCameraKey& b)
            {
                return a.timelineFrame < b.timelineFrame;
            }
        );

        std::vector<ShotCameraKey> mergedKeys;
        mergedKeys.reserve(keys_.size());

        for (const ShotCameraKey& key : keys_)
        {
            if (!mergedKeys.empty()
                && mergedKeys.back().timelineFrame == key.timelineFrame)
            {
                mergedKeys.back() = key;
            }
            else
            {
                mergedKeys.push_back(key);
            }
        }

        keys_ = std::move(mergedKeys);
    }
}
