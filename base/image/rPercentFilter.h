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
  parseOptions(const std::vector<std::string>& parts) override;

  /** Apply the filter from src to dst */
  virtual void
  process(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >       dst) override;

private:

  /** Template for speed */
  template <typename BndX, typename BndY>
  void
  applyFilter(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >           dst);

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
