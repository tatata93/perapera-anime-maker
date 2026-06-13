// src/camera/ShotCameraAnimation.h
//
// ShotCamera2Dの位置とズームを時間軸上のキーとして管理します。

#pragma once

#include "camera/ShotCamera2D.h"

#include <vector>

namespace perapera
{
    enum class ShotCameraInterpolation
    {
        Hold = 0,
        Linear = 1,
        Smooth = 2
    };

    struct ShotCameraKey
    {
        int timelineFrame = 0;
        ShotCamera2D camera;
        ShotCameraInterpolation interpolation = ShotCameraInterpolation::Smooth;
    };

    class ShotCameraAnimation
    {
    public:
        const std::vector<ShotCameraKey>& keys() const;

        bool empty() const;
        bool hasKeyAt(int timelineFrame) const;

        void clear();

        void setKey(
            int timelineFrame,
            const ShotCamera2D& camera,
            ShotCameraInterpolation interpolation
        );

        bool removeKeyAt(int timelineFrame);

        ShotCamera2D evaluate(
            int timelineFrame,
            const ShotCamera2D& fallbackCamera
        ) const;

        // 作画フレームの挿入・削除で時間軸の長さが変わったときに、
        // 後続のカメラキー位置を保つために使う。
        void insertTimelineFrames(int firstFrame, int frameCount);
        void removeTimelineFrames(int firstFrame, int frameCount);

    private:
        std::vector<ShotCameraKey> keys_;

        void sortAndMergeKeys();
    };
}
