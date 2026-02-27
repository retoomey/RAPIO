#pragma once

#include <rArray.h>

#include <memory>

namespace rapio {
/** How to handle boundary for array edges.
 * For example, RadialSet arrays may want to wrap in
 * the azimuth direction
 */
enum class Boundary {
  None, // Return False if OOB
  Wrap, // Modulo
  Clamp // Lock to edge
};

/** Interface for coordinate transformation.
 * For example, mapping one LatLonGrid to another one requires a geometry transform. */
class CoordMapper {
public:
  virtual
  ~CoordMapper() = default;
  /** Maps destination (i,j) to source fractional (u,v) */
  virtual bool
  map(int destI, int destJ, float& outU, float& outV) = 0;
};

/** ArrayAlgorithm
 *
 * Process on an Array
 *
 * Original ideas from Lak using Array vs the MRMS Image class.
 * We have an advantage in RAPIO since our data arrays are
 * all Array classes.
 *
 * We have sampler methods such as nearest neighbor,
 * cressman, etc.
 * We have filters such as ThresholdFilter, etc.
 *
 * FIXME: Think we could wrap opencv optionally for some things
 *
 * @author Robert Toomey
 * @ingroup rapio_image
 * @brief Algorithm API for processing our Array classes.
 */
class ArrayAlgorithm {
public:

  /** Destroy */
  virtual
  ~ArrayAlgorithm() = default;

  /** Introduce the samplers/filters */
  static void
  introduceSelf();

  /** Return help/list of registered samplers and filters */
  static std::string
  introduceHelp();

  /** Factory: "cressman:2", "threshold:0:50", "bilinear" */
  static std::shared_ptr<ArrayAlgorithm>
  create(const std::string& config);

  /** Parse string options in the factory */
  virtual bool
  parseOptions(const std::vector<std::string>& part, std::shared_ptr<ArrayAlgorithm> upstream = nullptr)
  {
    return true; // Default: no options to parse
  }

  /** Get help for us */
  virtual std::string
  getHelpString()
  {
    return "Help for array algorithm";
  }

  /** Set source array.  Needs to be called before sampleAt,
   * caches pointers and values for speed */
  virtual void
  setSource(std::shared_ptr<Array<float, 2> > in) = 0;

  /** Direct array to array processing.  Fastest but no
   * remapping/scaling ability allowed.  Best for simple
   * filters. Note that using source and dest the same will
   * technically work, but some filters might come out bad
   * if they are not in-place. */
  void
  process(std::shared_ptr<Array<float, 2> > source,
    std::shared_ptr<Array<float, 2> >       dest);

  /** * Centralized execution loop.
   * Handles fast-path indexing and complex mapping.
   */
  void
  remap(std::shared_ptr<Array<float, 2> > source,
    std::shared_ptr<Array<float, 2> >     dest,
    CoordMapper *                         mapper = nullptr);

  /** Apply algorithm to virtual index (slower, remapping use) */
  virtual bool
  sampleAt(float inI, float inJ, float& out) = 0;

  /** Apply algorithm to a true integer index (faster, fixed array) */
  virtual bool
  sampleAtIndex(int inI, int inJ, float& out) = 0;

  /** Set array wrapping function for the X/i direction */
  virtual void
  setBoundaryX(Boundary b) = 0;

  /** Set array wrapping function for the Y/j direction */
  virtual void
  setBoundaryY(Boundary b) = 0;

  /** Set both boundary wrapping functions */
  void
  setBoundary(Boundary a, Boundary b)
  {
    setBoundaryX(a);
    setBoundaryY(b);
  }

protected:

  /** Attempt to parse a string. If successful, overwrite out_val. Otherwise, do nothing. */
  template <typename T>
  static void
  parseParam(const std::string& str, T& out_val)
  {
    if (str.empty()) { return; }

    std::istringstream iss(str);
    T val;

    // Only overwrite if the stream successfully extracted the exact type
    if (iss >> val) {
      out_val = val;
    }
  }

  /** Safely extract from a vector and attempt to parse. */
  template <typename T>
  static void
  getParam(const std::vector<std::string>& parts, size_t index, T& out_val)
  {
    if (index < parts.size()) {
      parseParam(parts[index], out_val);
    }
  }
};
}
