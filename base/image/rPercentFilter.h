#pragma once
#include <rArrayFilter.h>
#include <rConstants.h>
#include <vector>
#include <string>

namespace rapio {
class PercentFilter : public ArrayFilter {
public:
  /** Create a PercentFilter */
  PercentFilter() = default;

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  /** Parse string options from the factory */
  virtual bool
  parseOptions(const std::string& params) override;

  // Advertises support for 2D spatial data only
  bool
  supportsDimensions(size_t dims) const override
  {
    return (dims == 2);
  }

  /** Apply the filter from src to dst */
  virtual void
  process2D(const std::shared_ptr<Array<float, 2> >& src,
    const std::shared_ptr<Array<float, 2> >        & dst) override;

private:

  /** Template for speed */
  template <typename BndX, typename BndY>
  void
  applyFilter(const std::shared_ptr<Array<float, 2> >& src,
    const std::shared_ptr<Array<float, 2> >          & dst);

  /** Default to a Median Filter (50th percentile) */
  float myPercentile = 0.5f;

  // Default to an 11x11 spatial window (5 pixels in each direction)

  /** Window size in X is 2*halfX+1 */
  int myHalfX = 5;

  /** Window size in Y is 2*halfY+1 */
  int myHalfY = 5;

  int myMinFillCount = 40;
};
}
