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
    : set(set), myNumRadials(set.getNumRadials()), myNumGates(set.getNumGates()),
    myFirstGateMeters(set.getDistanceToFirstGateM()),
    myPrimaryData(set.getFloat2DRef()),
    myAzimuths(set.getFloat1DRef(RadialSet::Azimuth)),
    myBeamWidths(set.getFloat1DRef(RadialSet::BeamWidth)),
    myGateWidths(set.getFloat1DRef(RadialSet::GateWidth)){ }

  /** Iterate over radial set  */
  inline void
  iterateRadialGates(RadialSetCallback& callback)
  {
    callback.handleBeginLoop(this, set);
    for (size_t r = 0; r < myNumRadials; ++r) {
      // ------------------------------------------
      // Calculate meta data for the radial
      // This keeps users from having to do this stuff and
      // possibly make mistakes.
      //
      myCurrentRadial = r;

      myBeamWidthDegs     = myBeamWidths[r];
      myAzimuthDegs       = myAzimuths[r];
      myCenterAzimuthDegs = myAzimuths[r] + (myBeamWidthDegs / 2.0); // center
      myGateWidthMeters   = myGateWidths[r];
      myRangeMeters       = myFirstGateMeters;
      myCenterRangeMeters = myFirstGateMeters + (myGateWidthMeters / 2.0);

      // FIXME: Maybe later callback.handleRayStart(this);

      for (size_t g = 0; g < myNumGates; ++g) {
        myCurrentGate = g;

        // Invoke the callback, passing `this` to allow access to the iterator’s methods
        callback.handleGate(this);

        // Move to the next gate along the ray in range
        myCenterRangeMeters += myGateWidthMeters;
        myRangeMeters       += myGateWidthMeters;
      }
    }
    callback.handleEndLoop(this, set);
  } // iterateRadialGates

  /** Set the primary data value */
  inline void
  setValue(float v)
  {
    myPrimaryData[myCurrentRadial][myCurrentGate] = v;
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
  RadialSet& set;

  // Constants from RadialSet, no need for access methods, these
  // are just cached for our use.
  const size_t myNumRadials;
  const size_t myNumGates;
  const float myFirstGateMeters;
  ArrayFloat2DRef myPrimaryData;
  ArrayFloat1DRef myAzimuths;
  ArrayFloat1DRef myBeamWidths;
  ArrayFloat1DRef myGateWidths;

  // Iterator variables

  size_t myCurrentRadial;
  size_t myCurrentGate;

  float myAzimuthDegs;
  float myBeamWidthDegs;
  float myCenterAzimuthDegs;
  float myRangeMeters;
  float myCenterRangeMeters;
  float myGateWidthMeters;
};
}
