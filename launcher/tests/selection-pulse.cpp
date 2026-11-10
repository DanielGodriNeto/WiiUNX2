// WiiUNX2 GPL-3.0-or-later. Host tests for the selected-tile outline pulse curve.
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include "../src/gui/SelectionPulse.h"

int main() {
    assert(std::fabs(selectionPulseScale(0) - 1.0f) <= 1e-6f);

    float minValue = 2.0f;
    float maxValue = -1.0f;
    float previous = selectionPulseScale(0);
    for (uint32_t t = 0; t <= 8000; ++t) {
        const float value = selectionPulseScale(t);
        assert(std::isfinite(value));
        assert(value >= 0.90f && value <= 1.10f);
        assert(std::fabs(value - previous) < 0.001f);
        previous = value;
        if (value < minValue) minValue = value;
        if (value > maxValue) maxValue = value;
    }
    assert(maxValue > 1.09f);
    assert(minValue < 0.91f);

    for (uint32_t t = 0; t < 4294963296u; t += 9973u) {
        assert(std::fabs(selectionPulseScale(t) - selectionPulseScale(t + 4000u)) <= 1e-5f);
    }

    const float huge = selectionPulseScale(4294967295u);
    assert(std::isfinite(huge));
    assert(huge >= 0.90f && huge <= 1.10f);

    std::printf("selection-pulse: min=%f max=%f huge=%f\n", static_cast<double>(minValue), static_cast<double>(maxValue), static_cast<double>(huge));
    return 0;
}
