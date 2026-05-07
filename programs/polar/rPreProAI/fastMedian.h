#pragma once

#include <memory>
#include <vector>

namespace rapio {
// Forward declaration
class RadialSet;

/**
 * Performs a fast median on a vector without any checks for missing data.
 * or min_good_percent. Probably too simple for you. Limited application.
 *
 * @param nums        The vector of data
 */
float
findSimpleMedian(std::vector<float> nums);

/**
 * Performs a fast 2D median filter on a RadialSet.
 * Handles circular azimuth wrapping and radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param radialWin        The size of the window in the radial (azimuth) dimension.
 * @param gateWin          The size of the window in the gate (range) dimension.
 * @param min_good_percent The minimum percentage (0-1) of valid data required
 */
void
applyFast2DMedian(std::shared_ptr<RadialSet> radialSet, int radialWin, int gateWin, float min_good_percent);

/**
 * Performs a fast 1D median filter on a RadialSet along the radial (gates only).
 * Handles radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param gateWin          The size of the window in the gate (range) dimension.
 * @param min_good_percent The minimum percentage (0-1) of valid data required.
 */
void
applyFast1DMedian_alongRadial(std::shared_ptr<RadialSet> radialSet, int gateWin, float min_good_percent);

/**
 * Performs a fast 1D median filter on a RadialSet across the radials (azimuth only).
 * Handles circular azimuth wrapping and radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param radialWin        The size of the window in the radial (azimuth) dimension.
 * @param min_good_percent The minimum percentage 0-1 to return a valid value.
 */
void
applyFast1D_accrossRadial(std::shared_ptr<RadialSet> radialSet, int radialWin, float min_good_percent);
} // namespace rapio
