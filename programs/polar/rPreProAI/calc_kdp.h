#pragma once

#include <memory>

namespace rapio {
// Forward declaration
class RadialSet;

/**
 * Computes the Kdp (Specific differential phase) using the triple median method.
 *
 * This method was developed by Peng Fei Zhange and Alexander Ryzhkov at NSSL circa 2013
 *
 * It's main feature is the identification of good data -vs- bad data using
 * a triple median of phase as the mean value in the computation with 
 * the standard deviation of phase. Large values of the std of phase ( >10.0 )
 * are considered "bad" data and should not be used in the Kdp computation.
 *
 * There is also a CC limit applied. CC < 0.8 is considered "bad" data. Althought
 * this limit is rarley applied. (For simplicity you can consider removing it)
 *
 * The method also applies "trimming". Kdp is only computed when the full filter_length
 * contains "good" data. This requires that data at the begining and at the end of a
 * "good" data segment do not produce a valid Kdp result. This is a key difference
 * beteween the many kdp computational schemes.
 *
 * Finally the method linearlly interpolates between segments of good data to produce
 * a result for each gate in the radail.
 *
 * @param PhiDP            The input phase
 * @param CC               The cross correlation coeffient azimuthally aligned to phase
 * @param KDP_filter_length_meters, The length of the segment use to compute the slope for Kdp
 *                                  we use meters for PAR and other non-WSR-88D radars
 * @returns Kdp            Specific Differential Phase
 */
std::shared_ptr<RadialSet>
compute_triple_median_Kdp(std::shared_ptr<RadialSet> PhiDP,
  std::shared_ptr<RadialSet>                         CC,
  int                                                KDP_filter_length_meters);

/**
 * Combines short Kdp and long Kdp based on a simple reflectivity threshold.
 * (improvment: meld the two values around the threshold rather than just swtiching)
 *
 * @param short_Kdp        The short filtered (9gates) Kdp (convective)
 * @param long_kdp         The longer Kdp (25 gates) gates(stratiform)
 * @param reflectivity     The Reflectivity (FIXME: why not base it on conv/strat split?)
 * @param refl_thresh      The reflectivity threshold to use (or conv/strat split)
 * @returns Kdp            Specific Differential Phase, combined
 */

std::shared_ptr<rapio::RadialSet>
combine_Kdp(std::shared_ptr<RadialSet> short_Kdp,
  std::shared_ptr<RadialSet>           long_Kdp,
  std::shared_ptr<RadialSet>           Ref,
  float                                refl_thresh);
} // namespace rapio
