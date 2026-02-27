#pragma once
#include <rArrayAlgorithm.h>
#include <rNearestNeighbor.h>

namespace rapio {
/** A chainable filter that gets its values from the previous
 * pipe element.
 *
 * @author Robert Toomey
 * @ingroup rapio_image
 * @brief A chainable filter of array data.
 */
class ArrayFilter : public ArrayAlgorithm {
public:

  /** Create an array filter */
  ArrayFilter(std::shared_ptr<ArrayAlgorithm> upstream = nullptr)
    : myUpstream(upstream)
  { }

  /** Factory method for late setting upstream */
  void
  setUpstream(std::shared_ptr<ArrayAlgorithm> up)
  {
    myUpstream = up;
  }

  /** Pass configuration up the chain */
  void
  setSource(std::shared_ptr<Array<float, 2> > in) override
  {
    myUpstream->setSource(in);
  }

  /** Pass X boundary type up the chain */
  void
  setBoundaryX(Boundary b) override { myUpstream->setBoundaryX(b); }

  /** Pass Y boundary type up the chain */
  void
  setBoundaryY(Boundary b) override { myUpstream->setBoundaryY(b); }

  /** Apply filter to virtual index (slower, remapping use) */
  virtual bool
  sampleAt(float u, float v, float& out) override
  {
    return myUpstream->sampleAt(u, v, out);
  }

  /** Apply algorithm to a true integer index (faster) */
  virtual bool
  sampleAtIndex(int inI, int inJ, float& out)
  {
    return myUpstream->sampleAtIndex(inI, inJ, out);
  }

  /** Helper to automatically route fractional coordinates down the chain */
  inline bool
  callUpstream(float u, float v, float& out)
  {
    return myUpstream->sampleAt(u, v, out);
  }

  /** Helper to automatically route integer coordinates down the chain */
  inline bool
  callUpstream(int i, int j, float& out)
  {
    return myUpstream->sampleAtIndex(i, j, out);
  }

protected:

  /** The previous pipeline element */
  std::shared_ptr<ArrayAlgorithm> myUpstream;
};
}

// Placed inside the class definition in the .h file
#define DECLARE_FILTER_SAMPLERS \
  virtual bool sampleAt(float u, float v, float& out) override; \
  virtual bool sampleAtIndex(int i, int j, float& out) override; \
  template <typename T> bool doSample(T x, T y, float& out);

// Placed at the bottom of your .cc file
#define DEFINE_FILTER_SAMPLERS(ClassName) \
  bool ClassName::sampleAt(float u, float v, float& out){ return doSample(u, v, out); } \
  bool ClassName::sampleAtIndex(int i, int j, float& out){ return doSample(i, j, out); }
