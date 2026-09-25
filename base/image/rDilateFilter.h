#pragma once
#include <rArrayFilter.h>
#include <rConstants.h>
#include <string>
#include <vector>

namespace rapio {
class DilateFilter : public ArrayFilter {
public:
  DilateFilter() = default;
  static void
  introduceSelf();

  virtual std::string
  getHelpString() override;
  virtual bool
  parseOptions(const std::string& params) override;
  virtual void
  process(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >       dst) override;

private:
  template <typename BndX, typename BndY>
  void
  applyFilter(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >           dst);

  int mySizeX         = 3;
  int mySizeY         = 3;
  int myHalfSizeX     = 1;
  int myHalfSizeY     = 1;
  float myMinFillFrac = 0.33f;
  int myMinFillCount  = 0;
  bool myDilateLarge  = true;
};
}
