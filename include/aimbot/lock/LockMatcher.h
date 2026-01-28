#pragma once

#include <vector>

#include "aimbot/vision/Detection.h"
#include "aimbot/app/AppConfig.h"

namespace aimbot::lock
{
    // Associates current-frame detections with the previously locked target.
    class LockMatcher {
    public:
        explicit LockMatcher(::LockConfig cfg) : cfg_(cfg) {}

        // Return index of the best matching detection, or -1 if no match.
        int matchIndex(const Detection& locked,
            const std::vector<Detection>& dets) const;

    private:
        ::LockConfig cfg_;
    };
}