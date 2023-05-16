#include "rLakRadialSmoother.h"

using namespace rapio;

void
LakRadialSmoother::smooth(std::shared_ptr<RadialSet> rs, int half_size)
{
  RadialSet& r           = *rs;
  const size_t radials   = r.getNumRadials();
  const int scale_factor = half_size * 2;
  const size_t gates     = r.getNumGates();
  auto& data = r.getFloat2D()->ref();

  // For each radial in the radial set....
  for (size_t i = 0; i < radials; ++i) {
    int N     = 0;
    float sum = 0;

    // Work on a copy of the gates
    std::vector<float> workRadial(gates);

    for (size_t j = 0; j < gates; ++j) {
      workRadial[j] = data[i][j];
      // Sum until we get enough (a window size of scale_factor)
      if (j <= scale_factor) {
        if (Constants::isGood(workRadial[j])) {
          sum += workRadial[j];
          N++;
        }
      } else {
        // take off one if it's a good value - don't forget to keep track of the sum count.
        if ((N > 0) && Constants::isGood(workRadial[j - scale_factor - 1])) {
          sum -= workRadial[j - scale_factor - 1];
          N--;
        }
        // add one onto the sum if it is a good one, and keep track of the sum count.
        if (Constants::isGood(workRadial[j])) {
          sum += workRadial[j];
          N++;
        }
      }

      // we have enough to compute the average for the window!
      if ((j >= scale_factor) && (N > half_size)) {
        data[i][j - half_size] = sum / (float) (N);
      }
    }
  }
} // LakRadialSmoother
