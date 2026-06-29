// このファイルの役割:
// 原画間の空きTへ中割を均等配置する計画を作る。
// 中割は画像自動生成ではなく、後で手描きするための作画Fをタイムシート上へ置く準備である。

#include "core/TimesheetInbetweenPlanner.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>

namespace perapera {
namespace {

bool containsFrame(const std::vector<int>& values, int value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

std::set<int> occupiedTimelineFramesInInterval(
    const TimesheetCellTrack& track,
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame)
{
    std::set<int> occupied;
    for (const TimesheetEntry& entry : track.entries) {
        if (entry.timelineFrame > previousKeyTimelineFrame && entry.timelineFrame < nextKeyTimelineFrame) {
            occupied.insert(entry.timelineFrame);
        }
    }
    return occupied;
}

std::vector<int> emptyTimelineFramesInInterval(
    const TimesheetCellTrack& track,
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame)
{
    const std::set<int> occupied = occupiedTimelineFramesInInterval(
        track,
        previousKeyTimelineFrame,
        nextKeyTimelineFrame);

    std::vector<int> result;
    for (int t = previousKeyTimelineFrame + 1; t <= nextKeyTimelineFrame - 1; ++t) {
        if (occupied.find(t) == occupied.end()) {
            result.push_back(t);
        }
    }
    return result;
}

std::vector<int> komaStepTargets(
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame,
    int komaStep)
{
    std::vector<int> targets;
    if (komaStep <= 0 || previousKeyTimelineFrame + 1 >= nextKeyTimelineFrame) {
        return targets;
    }

    for (int t = previousKeyTimelineFrame + komaStep; t < nextKeyTimelineFrame; t += komaStep) {
        if (t > previousKeyTimelineFrame && t < nextKeyTimelineFrame) {
            targets.push_back(t);
        }
    }
    return targets;
}

std::vector<int> idealTargets(int previousKeyTimelineFrame, int nextKeyTimelineFrame, int requestedCount)
{
    std::vector<int> targets;
    if (requestedCount <= 0 || previousKeyTimelineFrame + 1 >= nextKeyTimelineFrame) {
        return targets;
    }

    const int span = nextKeyTimelineFrame - previousKeyTimelineFrame;
    targets.reserve(static_cast<std::size_t>(requestedCount));
    for (int index = 1; index <= requestedCount; ++index) {
        const double raw = static_cast<double>(previousKeyTimelineFrame) +
            static_cast<double>(span) * static_cast<double>(index) / static_cast<double>(requestedCount + 1);
        int target = static_cast<int>(std::lround(raw));
        target = std::clamp(target, previousKeyTimelineFrame + 1, nextKeyTimelineFrame - 1);

        // 丸めで同じTへ寄った場合は、近い未使用Tへずらす。
        if (containsFrame(targets, target)) {
            int best = 0;
            int bestDistance = 999999;
            for (int t = previousKeyTimelineFrame + 1; t <= nextKeyTimelineFrame - 1; ++t) {
                if (containsFrame(targets, t)) {
                    continue;
                }
                const int distance = std::abs(t - target);
                if (distance < bestDistance) {
                    best = t;
                    bestDistance = distance;
                }
            }
            if (best > 0) {
                target = best;
            }
        }
        targets.push_back(target);
    }

    std::sort(targets.begin(), targets.end());
    return targets;
}

int nearestEmptyFrame(int target, const std::vector<int>& emptyFrames, const std::vector<int>& alreadyChosen)
{
    int best = 0;
    int bestDistance = 999999;
    for (int candidate : emptyFrames) {
        if (containsFrame(alreadyChosen, candidate)) {
            continue;
        }
        const int distance = std::abs(candidate - target);
        if (distance < bestDistance || (distance == bestDistance && candidate < best)) {
            best = candidate;
            bestDistance = distance;
        }
    }
    return best;
}

} // namespace

TimesheetInbetweenPlacementPlan planTimesheetInbetweenPlacements(
    const Timesheet& timesheet,
    std::string_view cellId,
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame,
    int requestedCount)
{
    TimesheetInbetweenPlacementPlan plan;
    plan.requestedCount = requestedCount;
    plan.previousKeyTimelineFrame = previousKeyTimelineFrame;
    plan.nextKeyTimelineFrame = nextKeyTimelineFrame;

    if (requestedCount <= 0) {
        plan.message = "requested count must be positive";
        return plan;
    }
    if (previousKeyTimelineFrame <= 0 || nextKeyTimelineFrame <= 0 || previousKeyTimelineFrame >= nextKeyTimelineFrame) {
        plan.message = "invalid key interval";
        return plan;
    }
    if (previousKeyTimelineFrame + 1 >= nextKeyTimelineFrame) {
        plan.message = "no timeline slot between key drawings";
        return plan;
    }

    const TimesheetCellTrack* track = findTimesheetTrack(timesheet, cellId);
    if (track == nullptr) {
        plan.message = "timesheet track not found";
        return plan;
    }

    const std::vector<int> emptyFrames = emptyTimelineFramesInInterval(
        *track,
        previousKeyTimelineFrame,
        nextKeyTimelineFrame);
    if (static_cast<int>(emptyFrames.size()) < requestedCount) {
        std::ostringstream message;
        message << "not enough empty T slots: requested=" << requestedCount
                << " empty=" << emptyFrames.size();
        plan.message = message.str();
        return plan;
    }

    const std::vector<int> targets = idealTargets(previousKeyTimelineFrame, nextKeyTimelineFrame, requestedCount);
    for (int target : targets) {
        const int chosen = nearestEmptyFrame(target, emptyFrames, plan.timelineFrames);
        if (chosen <= 0) {
            plan.message = "failed to choose an empty T slot";
            plan.timelineFrames.clear();
            return plan;
        }
        plan.timelineFrames.push_back(chosen);
    }

    std::sort(plan.timelineFrames.begin(), plan.timelineFrames.end());
    plan.ok = static_cast<int>(plan.timelineFrames.size()) == requestedCount;
    if (plan.ok) {
        std::ostringstream message;
        message << "planned " << requestedCount << " inbetweens";
        plan.message = message.str();
    } else {
        plan.message = "failed to build placement plan";
    }
    return plan;
}

TimesheetInbetweenPlacementPlan planTimesheetInbetweenPlacementsForKomaStep(
    const Timesheet& timesheet,
    std::string_view cellId,
    int previousKeyTimelineFrame,
    int nextKeyTimelineFrame,
    int komaStep)
{
    TimesheetInbetweenPlacementPlan plan;
    plan.komaStep = komaStep;
    plan.previousKeyTimelineFrame = previousKeyTimelineFrame;
    plan.nextKeyTimelineFrame = nextKeyTimelineFrame;

    if (komaStep <= 0) {
        plan.message = "koma step must be positive";
        return plan;
    }
    if (previousKeyTimelineFrame <= 0 || nextKeyTimelineFrame <= 0 || previousKeyTimelineFrame >= nextKeyTimelineFrame) {
        plan.message = "invalid key interval";
        return plan;
    }
    if (previousKeyTimelineFrame + 1 >= nextKeyTimelineFrame) {
        plan.message = "no timeline slot between key drawings";
        return plan;
    }

    const TimesheetCellTrack* track = findTimesheetTrack(timesheet, cellId);
    if (track == nullptr) {
        plan.message = "timesheet track not found";
        return plan;
    }

    const std::vector<int> targets = komaStepTargets(
        previousKeyTimelineFrame,
        nextKeyTimelineFrame,
        komaStep);
    if (targets.empty()) {
        plan.message = "no target T slot for the requested koma step";
        return plan;
    }

    const std::set<int> occupied = occupiedTimelineFramesInInterval(
        *track,
        previousKeyTimelineFrame,
        nextKeyTimelineFrame);
    for (int target : targets) {
        if (occupied.find(target) == occupied.end()) {
            plan.timelineFrames.push_back(target);
        }
    }

    if (plan.timelineFrames.empty()) {
        plan.message = "all target T slots are already occupied";
        return plan;
    }

    plan.requestedCount = static_cast<int>(plan.timelineFrames.size());
    plan.ok = true;

    std::ostringstream message;
    message << "planned " << plan.requestedCount
            << " inbetweens by " << komaStep << "-koma step";
    plan.message = message.str();
    return plan;
}

} // namespace perapera
