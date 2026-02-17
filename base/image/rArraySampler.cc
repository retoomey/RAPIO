#include <rArraySampler.h>

using namespace rapio;

void
ArraySampler::setSource(std::shared_ptr<Array<float, 2> > in)
{
  myArrayIn = in;
  if (in) {
    myRefIn = in->ptr();
    myMaxI  = in->getX();
    myMaxJ  = in->getY();
  } else {
    myRefIn = nullptr;
    myMaxI  = 0;
    myMaxJ  = 0;
  }
}

ArraySampler::ResolverFunc
ArraySampler::getResolver(Boundary b) const
{
  switch (b) {
      case Boundary::Wrap:  return &ArraySampler::resolveWrap;

      case Boundary::Clamp: return &ArraySampler::resolveClamp;

      default:              return &ArraySampler::resolveNone;
  }
}
