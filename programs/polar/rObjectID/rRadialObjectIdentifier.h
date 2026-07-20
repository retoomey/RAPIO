#pragma once
#include <RAPIO.h>
#include <memory>

namespace rapio {

class RadialObjectIdentifier {
public:
  //The basic single threshold identification
  //Needs no citation, all adjacent gates at or above threshold are identified as an object
  // and given the same value.
  static std::shared_ptr<RadialSet> ObjectID_Single_Threshold(std::shared_ptr<RadialSet> input, float threshold);

//Dixon, M., and G. Wiener, 1993: TITAN: Thunderstorm Identification, Tracking, Analysis,
//   and Nowcasting—A radar-based methodology. J. Atmos. Oceanic Technol., 10, 785–797, 
//   https://doi.org/10.1175/1520-0426(1993)010<0785:TTITAA>2.0.CO;2.

//https://github.com/ncar/lrose-titan

  //This is similar to TITAN and not like Lak's book with foothills. No sizes are checked.
  //TITAN also does some size checking. 
  //
  //Should be cited as "region growing" from a seed where the first threshold is the seed. 
  // If you can spare the configuration and the CPU use the Lak_Hysteresis method. 

  //A single threhsold approach is used to identify the starting "seed". 
  // From this seed, the each object grows one "ring" at a time allowing objects to grow
  // toward one another. 
  //
  static std::shared_ptr<RadialSet> ObjectID_Multi_Threshold(std::shared_ptr<RadialSet> input,
                                                      vector <float> & sorted_thresholds);

  //Lak's book requires sizes for the objects to be computed. Objects too small are removed 
  //matched vectors sorted_thresholds and min_size_in_gates. 

  //Lakshmanan, Valliappa. Automating the Analysis of Spatial Grids: 
  //    A Practical Guide to Data Mining Geospatial Images for Human & Environmental Applications. Springer, 2012.
  //6.4 pg 188-191

  //A single threhsold approach is used to identify the seed. If the object is large enough it 
  // is kept. From this seed, each object grows one "ring" at a time allowing objects to grow
  // toward one another. If a new object is detected at a lower threshold it's size is checked, small
  // objects are discarded. This removes noise in the data from makeing new objects from small seeds. 
  //  
  //configuration suggestion:    threholds set as multiples of on standarad deviation above the mean
  //                             sorted_thresholds = mean+3std, mean+2std, mean+1std
  //
  //                             double the minimum size 2,4,8 (small objects or smaller std) 
  //                                                     3,6,12 
  //                                                     5,10,20 (large objects or larger std)
  //objects located near the radar will be noiser than far away. 
  static std::shared_ptr<RadialSet> ObjectID_Hysteresis(std::shared_ptr<RadialSet> input,
                                                     vector <float> & sorted_thresholds,
                                                     vector <size_t> & min_size_in_gates);
};
//
//and now for the more CPU intensive and complex identification schemes......
// FIXME: add MCIT and possibly Kmeans here


} // namespace rapio
