#pragma once

#include <cmath>
#include <cstdint>

// Selected-tile outline brightness pulse. Pure math, no state, no I/O.
// Slow and small on purpose: the outline never flashes and stays legible
// at the dimmest point (base RGB times 0.90).
constexpr uint32_t kSelectionPulsePeriodMs = 4000;
constexpr float kSelectionPulseAmplitude   = 0.10f;
constexpr double kSelectionPulsePi         = 3.14159265358979323846;

// Returns a multiplier in [0.90, 1.10]; 1.0 at timeMs == 0; periodic with
// kSelectionPulsePeriodMs. The modulo runs in integer arithmetic so the
// float phase never drifts on long uptimes.
inline float selectionPulseScale(uint32_t timeMs) {
    const uint32_t phaseMs = timeMs % kSelectionPulsePeriodMs;
    const double phase     = 2.0 * kSelectionPulsePi * static_cast<double>(phaseMs) / static_cast<double>(kSelectionPulsePeriodMs);
    return 1.0f + kSelectionPulseAmplitude * static_cast<float>(std::sin(phase));
}
