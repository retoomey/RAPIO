#pragma once
  
#include <memory>
#include <vector>

namespace rapio {
// Forward declaration
class RadialSet;

/**
 * Performs a fast 2D average filter on a RadialSet.
 * Handles circular azimuth wrapping and radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param radialWin        The size of the window in the radial (azimuth) dimension.
 * @param gateWin          The size of the window in the gate (range) dimension.
 * @param min_good_percent The minimum percentage (0-1) of valid data required
 */

std::shared_ptr<RadialSet> apply2DBlurFilter( std::shared_ptr<RadialSet> & radialSet, int radialWin, int gateWin, float min_good_percent);

/**
 * Performs a fast 2D Max Value filter on a RadialSet.
 * Handles circular azimuth wrapping and radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param radialWin        The size of the window in the radial (azimuth) dimension.
 * @param gateWin          The size of the window in the gate (range) dimension.
 * @param min_good_percent The minimum percentage (0-1) of valid data required
 */


std::shared_ptr<RadialSet>  apply2DDilationFilter( std::shared_ptr<RadialSet> & radialSet, int radialWin, int gateWin, float min_good_percent); 

/**
 * Performs a fast 2D Value filter on a RadialSet. Trims the value of 1 by 
 * the radialWindow and the gateWindow supplied. The Trim happens by expanding
 * the value of "0". 
 * You should only send "0" and "1" float values to this function
 * Handles circular azimuth wrapping and radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param radialWin        The size of the window in the radial (azimuth) dimension.
 * @param gateWin          The size of the window in the gate (range) dimension.
 * @param min_good_percent The minimum percentage (0-1) of valid data required
 */
std::shared_ptr<RadialSet>  apply2DTrimFilter( std::shared_ptr<RadialSet> & radialSet, int radialWin, int gateWin); 

/**
 * Performs a fast 2D Value filter on a RadialSet. Expands the value of 1 by 
 * the radialWindow and the gateWindow supplied. 
 * You should only send "0" and "1" float values to this function.
 * Handles circular azimuth wrapping and radar-specific data sentinels.
 *
 * @param radialSet        The shared pointer to the RadialSet to be filtered.
 * @param radialWin        The size of the window in the radial (azimuth) dimension.
 * @param gateWin          The size of the window in the gate (range) dimension.
 * @param min_good_percent The minimum percentage (0-1) of valid data required
 */
std::shared_ptr<RadialSet>  apply2DExpandFilter( std::shared_ptr<RadialSet> & radialSet, int radialWin, int gateWin); 

}

