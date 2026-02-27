#pragma once
#include <rArrayAlgorithm.h>
#include <rArray.h>
#include <cmath>

namespace rapio {
/* The ArraySampler is responsible for providing access to raw data while handling
 * spatial constraints. It serves as the foundation for specific sampling
 * algorithms like Bilinear or Nearest Neighbor interpolation.
 * * **Key Responsibilities:**
 * - Manages the raw 2D data source via `std::shared_ptr`.
 * - Implements boundary resolution strategies (Wrap, Clamp, None).
 * - Provides optimized index resolution for performance-critical sampling loops.
 *
 * @author Robert Toomey
 * @ingroup rapio_image
 * @brief Base class for array sampling from virtual or direct indexes.
 */
class ArraySampler : public ArrayAlgorithm {
public:

  /**
   * @brief Virtual destructor to ensure proper cleanup of derived sampling classes.
   */
  virtual
  ~ArraySampler() = default;

  /**
   * @brief Construct a new Array Sampler object.
   * * Initializes internal pointers to null and dimensions to zero.
   */
  ArraySampler() :
    myArrayIn(nullptr), myRefIn(nullptr), myMaxI(0), myMaxJ(0)
  { }

  // --- 1. Data Management ---

  /**
   * @brief Sets the input data source for the sampler.
   * * This method updates the internal raw pointer and dimension caches for
   * high-speed access during sampling.
   * * @param in A shared pointer to a 2D Array of floats.
   */
  virtual void
  setSource(std::shared_ptr<Array<float, 2> > in) override;

  // --- 2. Boundary Configuration ---

  /**
   * @brief Configures how the X-axis handles Out-of-Bounds (OOB) indices.
   * @param b The desired Boundary strategy (Wrap, Clamp, or None).
   */
  virtual void
  setBoundaryX(Boundary b) override { myXResolver = getResolver(b); }

  /**
   * @brief Configures how the Y-axis handles Out-of-Bounds (OOB) indices.
   * @param b The desired Boundary strategy (Wrap, Clamp, or None).
   */
  virtual void
  setBoundaryY(Boundary b) override { myYResolver = getResolver(b); }

  /**
   * @brief Pure virtual function to sample data at specific continuous coordinates.
   * * Subclasses (e.g., Nearest, Bilinear) must implement this logic.
   * * @param u The horizontal coordinate (X).
   * @param v The vertical coordinate (Y).
   * @param[out] out The resulting sampled value.
   * @return true if the sample was successful and within valid bounds.
   * @return false if the coordinate is OOB (and Boundary is set to None).
   */
  virtual bool
  sampleAt(float u, float v, float& out) = 0;

  /** * @brief Fast-path for exact integer indexing (used by process()).
   * Marked 'final' to prevent spatial interpolators from overriding it.
   * All interpolators instantly degrade to Nearest Neighbor on exact integers.
   */
  inline bool
  sampleAtIndex(int i, int j, float& out) final
  {
    if (!resolveX(i)) { return false; }
    if (!resolveY(j)) { return false; }
    out = (*myRefIn)[i][j];
    return true;
  }

protected:

  /** * @brief Function pointer type for boundary resolver methods.
   * * @param i The index to be validated/transformed (passed by reference).
   * @param max The maximum dimension for that axis.
   * @return true if the resulting index is valid for sampling.
   */
  using ResolverFunc = bool (ArraySampler::*)(int&, size_t) const;

  /// The active resolver for the X dimension.
  ResolverFunc myXResolver = &ArraySampler::resolveNone;
  /// The active resolver for the Y dimension.
  ResolverFunc myYResolver = &ArraySampler::resolveNone;

  /**
   * @brief Maps a Boundary enum to the corresponding internal resolver method.
   * @param b The boundary enum.
   * @return ResolverFunc The associated member function pointer.
   */
  ResolverFunc
  getResolver(Boundary b) const;

  // --- 4. Protected Helpers for Subclasses ---

  /**
   * @brief Resolves an X index based on the current boundary strategy.
   * @param i The index to resolve.
   * @return true if index is safe to use.
   */
  inline bool
  resolveX(int& i) const
  {
    return (this->*myXResolver)(i, myMaxI);
  }

  /**
   * @brief Resolves a Y index based on the current boundary strategy.
   * @param j The index to resolve.
   * @return true if index is safe to use.
   */
  inline bool
  resolveY(int& j) const
  {
    return (this->*myYResolver)(j, myMaxJ);
  }

  // --- 5. The Resolver Implementations ---

  /**
   * @brief Strict boundary check.
   * @return True only if i is within [0, max).
   */
  inline bool
  resolveNone(int& i, size_t max) const
  {
    return (i >= 0 && i < (int) max);
  }

  /**
   * @brief Toroidal wrap logic.
   * Transforms i so it wraps around the range [0, max).
   * @return Always true.
   */
  inline bool
  resolveWrap(int& i, size_t max) const
  {
    i = (i % (int) max + (int) max) % (int) max;
    return true;
  }

  /**
   * @brief Clamping logic.
   * Limits i to the range [0, max - 1].
   * @return Always true.
   */
  inline bool
  resolveClamp(int& i, size_t max) const
  {
    if (i < 0) { i = 0; } else if (i >= (int) max) { i = (int) max - 1; }
    return true;
  }

  // --- Data Members ---

  /// Stored shared pointer to ensure the data source stays in scope.
  std::shared_ptr<Array<float, 2> > myArrayIn;

  /// Raw pointer to the multi_array for high-performance indexing.
  boost::multi_array<float, 2> * myRefIn = nullptr;

  /// Cached width of the input array.
  size_t myMaxI = 0;

  /// Cached height of the input array.
  size_t myMaxJ = 0;
};
} // namespace rapio
