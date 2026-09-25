#pragma once
#include <rArray.h>
#include <rStrings.h>
#include <rArrayBoundary.h>

#include <memory>
#include <vector>
#include <string>

namespace rapio {
/** A stage in the array pipeline */
class ArrayStage {
public:
  virtual
  ~ArrayStage() = default;

  // Param/Help ability --------------------------

  /** Parse a param string for a stage.  All stages take the
   * form of 'name:params' */
  virtual bool
  parseOptions(const std::string& params)
  {
    return true; // Default implementation cleanly accepts empty/no params
  }

  /** Get the help for this sampler.  Abstract to enforce help */
  virtual std::string
  getHelpString() = 0;

  // Boundary ability ----------------------------

  /** Stages can support various array wrapping methods.
   * filters or samplers can ignore this if they are single
   * in place such as threshold.  Typically used for matrix
   * based samplers or filters */
  void
  setBoundary(Boundary x, Boundary y)
  {
    myXBoundary = x;
    myYBoundary = y;
  }

  /** Return the X boundary */
  Boundary
  getXBoundary() const { return myXBoundary; }

  /** Return the Y boundary */
  Boundary
  getYBoundary() const { return myYBoundary; }

protected:

  /** Default is no extra params, subclasses should add parameters */
  virtual bool parseOptions(const std::vector<std::string>& parts){ return true; }

  // FIXME:  Why not N dimension possibility (or at least 3)

  /** Boundary for the X dimension */
  Boundary myXBoundary = Boundary::None;

  /** Boundary for the Y dimension */
  Boundary myYBoundary = Boundary::None;
};
}
