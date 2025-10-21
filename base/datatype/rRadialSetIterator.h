#pragma once

#include <rUtility.h>
#include <rRadialSet.h>

namespace rapio {
class RadialSetIterator;

/**
 * @brief Callback for a radial set iterator using a visitor pattern.
 *
 * Implement this class to handle processing of each gate within a
 * RadialSet by subclassing and defining the `handleGate` method.
 */
class RadialSetCallback : public Utility {
public:
  /** Called to handle a particular gate */
  virtual void
  handleGate(RadialSetIterator * it) = 0;

  /** Called before any loop */
  virtual void
  handleBeginLoop(RadialSetIterator *, const RadialSet& rs){ };

  /** Called after any loop */
  virtual void
  handleEndLoop(RadialSetIterator *, const RadialSet& rs){ };
};

/**.
 * @brief Iterator for RadialSet data with optimized performance.
 *
 * This iterator uses a visitor pattern for efficient processing of
 * large data sets, minimizing the overhead of standard iterators.
 * Iteration metadata (such as azimuth and range) are calculated once
 * per radial and are accessible to the callback.
 *
 * Example usage:
 * @code
 * class MyRadialCallback : public RadialSetCallback {
 * public:
 *     void handleGate(RadialSetIterator* it) override {
 *         // Set each gate's value to 5.0
 *         it->setValue(5.0);
 *     }
 * };
 *
 * RadialSet myRadialSet = ...; // Initialize RadialSet as needed
 * RadialSetIterator iterator(myRadialSet);
 * MyRadialCallback callback;
 *
 * iterator.iterateRadialGates(callback);
 * @endcode
 *
 * @author Robert Toomey
 */
class RadialSetIterator : public Utility {
public:
  RadialSetIterator(RadialSet& set)
    : myRadialSet(set),
  // Use pointer so we can change which array we use
  myOutputArray(set.getFloat2DPtr()){ }

  /** Set the array for output of the iterator */
  void
  setOutputArray(const std::string& key = Constants::PrimaryDataName)
  {
    // LogSevere("Primary before  " << (void*)(myOutputArray->data()) << "\n");
    myOutputArray = myRadialSet.getFloat2DPtr(key);
    // LogSevere("Primary after is " << (void*)(myOutputArray->data()) << "\n");
  }

  /** Iterate over radial set  */
  inline void
  iterateRadialGates(RadialSetCallback& callback)
  {
    auto const radials  = myRadialSet.getNumRadials();
    auto const gates    = myRadialSet.getNumGates();
    auto const fgMeters = myRadialSet.getDistanceToFirstGateM();
    auto const azDegs   = myRadialSet.getFloat1DRef(RadialSet::Azimuth);
    auto const bwDegs   = myRadialSet.getFloat1DRef(RadialSet::BeamWidth);
    auto const gwMeters = myRadialSet.getFloat1DRef(RadialSet::GateWidth);

    callback.handleBeginLoop(this, myRadialSet);
    for (size_t r = 0; r < radials; ++r) {
      // ------------------------------------------
      // Calculate meta data for the radial
      // This keeps users from having to do this stuff and
      // possibly make mistakes.
      //
      myCurrentRadial = r;

      myBeamWidthDegs     = bwDegs[r];
      myAzimuthDegs       = azDegs[r];
      myCenterAzimuthDegs = azDegs[r] + (myBeamWidthDegs * 0.5); // center
      myGateWidthMeters   = gwMeters[r];
      myRangeMeters       = fgMeters;
      myCenterRangeMeters = fgMeters + (myGateWidthMeters * 0.5);

      // FIXME: Maybe later callback.handleRayStart(this);

      for (size_t g = 0; g < gates; ++g) {
        myCurrentGate = g;

        // Invoke the callback, passing `this` to allow access to the iterator’s methods
        callback.handleGate(this);

        // Move to the next gate along the ray in range
        myCenterRangeMeters += myGateWidthMeters;
        myRangeMeters       += myGateWidthMeters;
      }
    }
    callback.handleEndLoop(this, myRadialSet);
  } // iterateRadialGates

  /** Set the primary data value */
  inline void
  setValue(float v)
  {
    //   LogSevere("Primary on set is " << (void*)(myOutputArray->data()) << "\n");
    // exit(1);
    (*myOutputArray)[myCurrentRadial][myCurrentGate] = v;
  }

  inline size_t
  getCurrentRadial() const { return myCurrentRadial; }

  inline size_t
  getCurrentGate() const { return myCurrentGate; }

  inline float
  getStartAzimuthDegs() const { return myAzimuthDegs; }

  inline float
  getBeamWidthDegs() const { return myBeamWidthDegs; }

  inline float
  getCenterAzimuthDegs() const { return myCenterAzimuthDegs; }

  inline float
  getStartGateWidthMeters() const { return myGateWidthMeters; }

  inline float
  getRangeMeters() const { return myRangeMeters; }

  inline float
  getCenterRangeMeters() const { return myCenterRangeMeters; }


private:
  /** Reference to our RadialSet */
  RadialSet& myRadialSet;

  // Cache for speed
  ArrayFloat2DPtr myOutputArray;

  // Iterator variables

  size_t myCurrentRadial; ///< Current radial number iterating
  size_t myCurrentGate;   ///< Current gate index iterating

  float myAzimuthDegs;       ///< Current start azimuth
  float myBeamWidthDegs;     ///< Current beamwidth of the radial
  float myCenterAzimuthDegs; ///< Current center of gate azimuth (middle of current gate)
  float myRangeMeters;       ///< Current range in meters of the current gate
  float myCenterRangeMeters; ///< Current center range (middle of current gate)
  float myGateWidthMeters;   ///< Current gate width in meters
};
}
