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

protected:

  /** The previous pipeline element */
  std::shared_ptr<ArrayAlgorithm> myUpstream;
};
}
