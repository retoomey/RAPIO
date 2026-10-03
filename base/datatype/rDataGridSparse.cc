#include "rDataGrid.h"
#include "rConstants.h"
#include "rError.h"
#include "rStrings.h"
#include "rIOConfig.h"

#include <array>
#include <algorithm>

using namespace rapio;

namespace {
template <size_t N> struct Axes;
template <> struct Axes<2> {
  static std::array<std::string, 2> names(){ return { { "pixel_x", "pixel_y" } }; }
};
template <> struct Axes<3> {
  static std::array<std::string, 3> names(){ return { { "pixel_z", "pixel_x", "pixel_y" } }; }
};

size_t
countRuns(const float * d, size_t n, float bg)
{
  size_t runs = 0;
  float last  = bg;

  for (size_t i = 0; i < n; ++i) {
    if ((d[i] != bg) && (d[i] != last) ) {
      runs++;
    }
    last = d[i];
  }
  return runs;
}

size_t
estimateRuns(const float * d, size_t rows, size_t rowLen, float bg)
{
  const size_t stride = std::max<size_t>(1, rows / 10);
  size_t runs = 0, sampled = 0;

  for (size_t r = 0; r < rows; r += stride) {
    sampled++;
    runs += countRuns(d + r * rowLen, rowLen, bg);
  }
  return runs * rows / sampled;
}

template <size_t N>
void
unravel(size_t flat, const std::array<size_t, N>& sz, std::array<size_t, N>& c)
{
  for (size_t k = N; k-- > 0;) {
    c[k]  = flat % sz[k];
    flat /= sz[k];
  }
}
} // anonymous namespace

namespace rapio {
template <size_t N>
bool
DataGrid::sparseT(IOConfig& keys)
{
  const auto mode = keys.getSparseMode();

  if ((mode == IOConfig::SparseMode::None) || getShort1D("pixel_x")) { return false; }

  auto arr = get<Array<float, N> >(Constants::PrimaryDataName);

  if (!arr) { return false; }
  auto& ref          = arr->ref();
  const float * d    = ref.data();
  const size_t total = ref.num_elements();

  if (total == 0) { return false; }

  std::array<size_t, N> sz;

  std::copy(ref.shape(), ref.shape() + N, sz.begin());

  for (size_t k = 0; k < N; ++k) {
    if (sz[k] > 32767) {
      fLogSevere("Grid dimension {} exceeds max short (32767). Cannot sparse.", sz[k]);
      return false;
    }
  }

  const float bg        = Constants::MissingData;
  const float threshold = keys.getSparseThreshold();
  const float runCost   = static_cast<float>(sizeof(float) + sizeof(int) + N * sizeof(short)) / sizeof(float);

  if (mode == IOConfig::SparseMode::Guess) {
    const float est = runCost * estimateRuns(d, total / sz[N - 1], sz[N - 1], bg) / total;
    if (est > threshold * 1.1f) {
      fLogDebug("Guess mode aborted sparsification. Est ratio: {:.2f} > threshold {:.2f}", est, threshold);
      return false;
    }
  }

  const size_t runs = countRuns(d, total, bg);
  const float ratio = runCost * runs / total;

  if ((mode != IOConfig::SparseMode::Force) && (ratio > threshold) ) {
    fLogInfo("---> {}D compression rejected: {}% >= threshold {}%", N, int(0.5 + 100 * ratio),
      int(0.5 + 100 * threshold));
    return false;
  }

  fLogInfo("---> {}D compression {}: {}% of original.", N,
    (mode == IOConfig::SparseMode::Force) ? "FORCED" : "accepted", int(0.5 + 100 * ratio));

  const std::string units = getUnits();
  // A zero-length dimension means NC_UNLIMITED to netcdf, which breaks writing (and only
  // one unlimited dimension is allowed in classic files).  So an all-background grid
  // stores a single dummy run: one background pixel at the origin.  On read it just
  // re-sets a cell that is already background, so it is harmless.
  const size_t pixelDim = std::max<size_t>(runs, 1);

  myDims.push_back(DataGridDimension("pixel", pixelDim));
  changeArrayName(Constants::PrimaryDataName, "DisabledPrimary");
  setVisible("DisabledPrimary", false);

  size_t dimIndex = myDims.size() - 1;

  float * vals = addFloat1DRef(Constants::PrimaryDataName, units, { dimIndex }).data();
  std::array<short *, N> axis;
  auto defaultNames = Axes<N>::names();

  for (size_t k = 0; k < N; ++k) {
    axis[k] = addShort1DRef(defaultNames[k], "dimensionless", { dimIndex }).data();
  }
  int * counts = addInt1DRef("pixel_count", "dimensionless", { dimIndex }).data();

  if (runs == 0) {
    vals[0] = bg;
    for (size_t k = 0; k < N; ++k) {
      axis[k][0] = 0;
    }
    counts[0] = 1;
  }

  size_t at  = 0;
  float last = bg;
  std::array<size_t, N> c;

  for (size_t i = 0; i < total; ++i) {
    const float v = d[i];
    if (v != bg) {
      if (v == last) {
        ++counts[at - 1];
      } else {
        unravel<N>(i, sz, c);
        for (size_t k = 0; k < N; ++k) {
          axis[k][at] = static_cast<short>(c[k]);
        }
        vals[at]   = v;
        counts[at] = 1;
        ++at;
      }
    }
    last = v;
  }

  auto pixelsDataArray = getNode(Constants::PrimaryDataName);

  if (pixelsDataArray) {
    pixelsDataArray->setFloat("missing_value", Constants::MissingData);
    pixelsDataArray->setFloat("BackgroundValue", bg);
    pixelsDataArray->setFloat("SparseGridCompression", ratio);
    pixelsDataArray->setLong("NumValidRuns", runs); // real runs; 0 means only the dummy pixel
  }

  setDataType("Sparse" + getDataType());
  return true;
} // DataGrid::sparseT

template <size_t N>
void
DataGrid::unsparseT(IOConfig& keys, const std::array<std::string, N>& axisNames, const std::string& countName)
{
  if (!IOConfig::getGlobalDoUnsparse()) {
    fLogInfo("{}D unsparse is deactivated. Probably for direct copying.", N);
    return;
  }

  std::string datatype    = getDataType();
  const bool isSparseType = Strings::beginsWith(datatype, "Sparse");
  auto axis0 = getShort1D(axisNames[0]);

  // No axis array and not typed Sparse: a normal dense grid, nothing to do.
  if (!axis0 && !isSparseType) { return; }

  // Array present but not readable as short[]: don't mistake that for "zero runs"
  if (!axis0 && haveArrayName(axisNames[0])) {
    fLogSevere("Array {} exists but is not a 1D short array, can't unsparse.", axisNames[0]);
    return;
  }

  const auto sizes = getSizes();

  if (sizes.size() < N) {
    fLogSevere("Grid dimensions incorrect for unsparsing.");
    return;
  }

  size_t total = 1;
  std::array<size_t, N> sz;

  for (size_t k = 0; k < N; ++k) {
    sz[k]  = sizes[k];
    total *= sz[k];
  }

  size_t num_pixels  = 0;
  const float * vals = nullptr;
  const int * counts = nullptr;
  std::vector<int> defaultCounts;
  std::array<const short *, N> axis = { };

  auto valPtr = getFloat1D(Constants::PrimaryDataName);

  // Structural checks happen before we modify anything. Individual bad runs are
  // handled leniently at decode time, matching the legacy reader.
  if (axis0) {
    num_pixels = axis0->ref().size();
    if (num_pixels > total) {
      fLogSevere("Corrupt?: num_pixels is {} while max_size is {}", num_pixels, total);
      return;
    }
    if (!valPtr) {
      fLogSevere("Expected primary data array, can't find to unsparse.");
      return;
    }

    // Legacy readers treat a missing count array as "every run has length 1"
    auto countPtr = getInt1D(countName);
    if ((valPtr->ref().size() != num_pixels) || (countPtr && (countPtr->ref().size() != num_pixels) )) {
      fLogSevere("Corrupt sparse data: value/count array length differs from {} ({})", axisNames[0], num_pixels);
      return;
    }
    for (size_t k = 0; k < N; ++k) {
      auto axisK = getShort1D(axisNames[k]);
      if (!axisK || (axisK->ref().size() != num_pixels) ) {
        fLogSevere("Corrupt sparse data: {} missing or length differs from {} ({})",
          axisNames[k], axisNames[0], num_pixels);
        return;
      }
      axis[k] = axisK->ref().data();
    }
    vals = valPtr->ref().data();
    if (countPtr) {
      counts = countPtr->ref().data();
    } else {
      defaultCounts.assign(num_pixels, 1);
      counts = defaultCounts.data();
    }
  } else {
    // Typed Sparse with no pixel arrays at all: the legacy writer writes nothing
    // when every cell is background (zero runs). Only accept that if there is no
    // real data here that we'd be overwriting.
    if (valPtr ? (valPtr->ref().size() != 0) : haveArrayName(Constants::PrimaryDataName)) {
      fLogSevere("Sparse grid has no {} array but has primary data, can't unsparse.", axisNames[0]);
      return;
    }
    fLogInfo("{}D Sparse grid has no pixel arrays. Assuming 0 runs (all background).", N);
  }

  fLogInfo("{}D Sparse Dimensions: {} for {} total pixels", N, num_pixels, total);

  float bg = Constants::MissingData;
  auto sparseArrayNode = getNode(Constants::PrimaryDataName);

  if (sparseArrayNode) {
    sparseArrayNode->getFloat("BackgroundValue", bg);
  }

  // --- commit: from here on we modify the grid ---
  const std::string dataunits = getUnits();

  if (valPtr && !changeArrayName(Constants::PrimaryDataName, "SparseData")) {
    fLogSevere("Unable to move sparse primary array out of the way, can't unsparse.");
    return;
  }

  std::vector<size_t> dimIndices(N);

  for (size_t k = 0; k < N; ++k) {
    dimIndices[k] = k;
  }

  auto dstArr = add<float, N>(Constants::PrimaryDataName, dataunits, FLOAT, dimIndices, bg);

  if (!dstArr) {
    // Put things back the way they were
    if (valPtr) { changeArrayName("SparseData", Constants::PrimaryDataName); }
    fLogSevere("Unable to create full {}D array, can't unsparse.", N);
    return;
  }
  float * d = dstArr->ref().data();

  // Same tolerance as the legacy reader: a run starting outside the grid is skipped,
  // a run that overruns the end of the grid is trimmed, and the start pixel is always
  // written when in bounds (so a count < 1 behaves like 1).
  size_t skipped = 0, trimmed = 0, badCounts = 0;

  for (size_t i = 0; i < num_pixels; ++i) {
    size_t flat   = 0;
    bool inBounds = true;
    for (size_t k = 0; k < N; ++k) {
      const short a = axis[k][i];
      if ((a < 0) || (static_cast<size_t>(a) >= sz[k])) { inBounds = false; break; }
      flat = flat * sz[k] + static_cast<size_t>(a);
    }
    if (!inBounds) { ++skipped; continue; }

    if (counts[i] < 0) { ++badCounts; }
    size_t n = (counts[i] > 0) ? static_cast<size_t>(counts[i]) : 1;
    if (n > total - flat) { n = total - flat; ++trimmed; }
    std::fill_n(d + flat, n, vals[i]);
  }
  if (skipped > 0)   { fLogSevere("Corrupt?: Skipped a total of {} runs starting outside the grid", skipped); }
  if (trimmed > 0)   { fLogSevere("Corrupt?: Trimmed a total of {} runlengths", trimmed); }
  if (badCounts > 0) { fLogSevere("Corrupt?: Found {} negative pixel counts", badCounts); }

  Strings::removePrefix(datatype, "Sparse");
  setDataType(datatype);

  if (valPtr) { deleteArrayName("SparseData"); }
  for (size_t k = 0; k < N; ++k) {
    deleteArrayName(axisNames[k]);
  }
  deleteArrayName(countName);

  // The pixel dimension may be absent if the reader omitted it (zero-run files)
  if ((myDims.size() > N) && (myDims.back().name() == "pixel") ) {
    myDims.pop_back();
  }
} // DataGrid::unsparseT

void
DataGrid::unsparseRestore()
{
  std::string datatype = getDataType();

  if (!Strings::beginsWith(datatype, "Sparse")) { return; }

  // Only valid if sparse2D/3D made this sparse (it leaves the original behind).
  // Otherwise the sparse arrays ARE the data (e.g. read from file), so deleting
  // them would lose everything.
  if (!haveArrayName("DisabledPrimary") || myDims.empty() || (myDims.back().name() != "pixel") ) {
    return;
  }

  deleteArrayName(Constants::PrimaryDataName);
  deleteArrayName("pixel_z");
  deleteArrayName("pixel_y");
  deleteArrayName("pixel_x");
  deleteArrayName("pixel_count");

  myDims.pop_back();

  changeArrayName("DisabledPrimary", Constants::PrimaryDataName);
  setVisible(Constants::PrimaryDataName, true);

  Strings::removePrefix(datatype, "Sparse");
  setDataType(datatype);
}

// ----------------------------------------------------------------------
// Instantiated Wrappers (Calling legacy unsparse versions for compatability)
// ----------------------------------------------------------------------

bool DataGrid::sparse2D(IOConfig& keys){ return sparseT<2>(keys); }

bool DataGrid::sparse3D(IOConfig& keys){ return sparseT<3>(keys); }

void
DataGrid::unsparse2D(size_t num_x, size_t num_y, IOConfig& keys, const std::string& pixelX, const std::string& pixelY,
  const std::string& pixelCount)
{
  if (getShort1D(pixelX)) {
    const auto s = getSizes();
    if ((s.size() < 3) || (s[0] != num_x) || (s[1] != num_y) ) {
      fLogSevere("Caller expects a {}x{} grid but dimensions 0,1 differ, can't unsparse.", num_x, num_y);
      return;
    }
  }
  unsparseT<2>(keys, { { pixelX, pixelY } }, pixelCount);
}

void
DataGrid::unsparse3D(size_t num_x, size_t num_y, size_t num_z, IOConfig& keys, const std::string& pixelX,
  const std::string& pixelY, const std::string& pixelZ, const std::string& pixelCount)
{
  if (getShort1D(pixelZ)) {
    const auto s = getSizes();
    if ((s.size() < 4) || (s[0] != num_z) || (s[1] != num_x) || (s[2] != num_y) ) {
      fLogSevere("Caller expects a {}x{}x{} (z,x,y) grid but dimensions 0,1,2 differ, can't unsparse.", num_z, num_x,
        num_y);
      return;
    }
  }
  unsparseT<3>(keys, { { pixelZ, pixelX, pixelY } }, pixelCount);
}
} // namespace rapio
