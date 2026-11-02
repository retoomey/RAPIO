#include "rSparseEncoder.h"
#include "rConstants.h"
#include "rError.h"
#include "rStrings.h"

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
SparseEncoder::sparseT(DataGrid& grid, IOConfig& keys)
{
  const auto mode = keys.getSparseMode();

  if ((mode == IOConfig::SparseMode::None) || grid.getShort1D("pixel_x")) { return false; }

  auto arr = grid.get<Array<float, N> >(Constants::PrimaryDataName);

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

  const std::string units = grid.getUnits();
  const size_t pixelDim   = std::max<size_t>(runs, 1);

  grid.addDim("pixel", pixelDim);
  grid.changeArrayName(Constants::PrimaryDataName, "DisabledPrimary");
  grid.setVisible("DisabledPrimary", false);

  size_t dimIndex = grid.getDims().size() - 1;

  float * vals = grid.addFloat1DRef(Constants::PrimaryDataName, units, { dimIndex }).data();
  std::array<short *, N> axis;
  auto defaultNames = Axes<N>::names();

  for (size_t k = 0; k < N; ++k) {
    axis[k] = grid.addShort1DRef(defaultNames[k], "dimensionless", { dimIndex }).data();
  }
  int * counts = grid.addInt1DRef("pixel_count", "dimensionless", { dimIndex }).data();

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

  auto pixelsDataArray = grid.getNode(Constants::PrimaryDataName);

  if (pixelsDataArray) {
    pixelsDataArray->setFloat("missing_value", Constants::MissingData);
    pixelsDataArray->setFloat("BackgroundValue", bg);
    pixelsDataArray->setFloat("SparseGridCompression", ratio);
    pixelsDataArray->setLong("NumValidRuns", runs);
  }

  grid.setDataType("Sparse" + grid.getDataType());
  return true;
} // SparseEncoder::sparseT

template <size_t N>
void
SparseEncoder::unsparseT(DataGrid& grid, IOConfig& /*keys*/, const std::array<std::string, N>& axisNames,
  const std::string& countName)
{
  if (!IOConfig::getGlobalDoUnsparse()) {
    fLogInfo("{}D unsparse is deactivated. Probably for direct copying.", N);
    return;
  }

  std::string datatype    = grid.getDataType();
  const bool isSparseType = Strings::beginsWith(datatype, "Sparse");
  auto axis0 = grid.getShort1D(axisNames[0]);

  if (!axis0 && !isSparseType) { return; }

  if (!axis0 && grid.haveArrayName(axisNames[0])) {
    fLogSevere("Array {} exists but is not a 1D short array, can't unsparse.", axisNames[0]);
    return;
  }

  const auto sizes = grid.getSizes();

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

  auto valPtr = grid.getFloat1D(Constants::PrimaryDataName);

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

    auto countPtr = grid.getInt1D(countName);
    if ((valPtr->ref().size() != num_pixels) || (countPtr && (countPtr->ref().size() != num_pixels) )) {
      fLogSevere("Corrupt sparse data: value/count array length differs from {} ({})", axisNames[0], num_pixels);
      return;
    }
    for (size_t k = 0; k < N; ++k) {
      auto axisK = grid.getShort1D(axisNames[k]);
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
    if (valPtr ? (valPtr->ref().size() != 0) : grid.haveArrayName(Constants::PrimaryDataName)) {
      fLogSevere("Sparse grid has no {} array but has primary data, can't unsparse.", axisNames[0]);
      return;
    }
    fLogInfo("{}D Sparse grid has no pixel arrays. Assuming 0 runs (all background).", N);
  }

  fLogInfo("{}D Sparse Dimensions: {} for {} total pixels", N, num_pixels, total);

  float bg = Constants::MissingData;
  auto sparseArrayNode = grid.getNode(Constants::PrimaryDataName);

  if (sparseArrayNode) {
    sparseArrayNode->getFloat("BackgroundValue", bg);
  }

  const std::string dataunits = grid.getUnits();

  if (valPtr && !grid.changeArrayName(Constants::PrimaryDataName, "SparseData")) {
    fLogSevere("Unable to move sparse primary array out of the way, can't unsparse.");
    return;
  }

  std::vector<size_t> dimIndices(N);

  for (size_t k = 0; k < N; ++k) {
    dimIndices[k] = k;
  }

  auto dstArr = grid.add<float, N>(Constants::PrimaryDataName, dataunits, FLOAT, dimIndices, bg);

  if (!dstArr) {
    if (valPtr) { grid.changeArrayName("SparseData", Constants::PrimaryDataName); }
    fLogSevere("Unable to create full {}D array, can't unsparse.", N);
    return;
  }
  float * d = dstArr->ref().data();

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
  grid.setDataType(datatype);

  if (valPtr) { grid.deleteArrayName("SparseData"); }
  for (size_t k = 0; k < N; ++k) {
    grid.deleteArrayName(axisNames[k]);
  }
  grid.deleteArrayName(countName);

  if ((grid.getDims().size() > N) && (grid.getDims().back().name() == "pixel") ) {
    grid.popDim();
  }
} // SparseEncoder::unsparseT

void
SparseEncoder::unsparseRestore(DataGrid& grid)
{
  std::string datatype = grid.getDataType();

  if (!Strings::beginsWith(datatype, "Sparse")) { return; }

  if (grid.getDims().empty() || (grid.getDims().back().name() != "pixel")) {
    return;
  }

  grid.deleteArrayName(Constants::PrimaryDataName);
  grid.deleteArrayName("pixel_z");
  grid.deleteArrayName("pixel_y");
  grid.deleteArrayName("pixel_x");
  grid.deleteArrayName("pixel_count");
  grid.popDim();

  if (grid.haveArrayName("DisabledPrimary")) {
    grid.changeArrayName("DisabledPrimary", Constants::PrimaryDataName);
    grid.setVisible(Constants::PrimaryDataName, true);
  }

  Strings::removePrefix(datatype, "Sparse");
  grid.setDataType(datatype);
}

bool
SparseEncoder::sparse3DN(DataGrid& grid, IOConfig& keys, const std::vector<std::shared_ptr<Array<float, 2> > >& grids)
{
  if (keys.getSparseMode() == IOConfig::SparseMode::None) { return false; }

  if (grid.getShort1D("pixel_x")) {
    fLogInfo("Not making sparse since pixels already exists...");
    return false;
  }

  if (grids.empty()) { return false; }

  const size_t sizeZ = grids.size();
  const size_t sizeX = grids[0]->getX();
  const size_t sizeY = grids[0]->getY();

  float backgroundValue = Constants::MissingData;
  size_t neededPixels   = 0;
  float lastValue       = backgroundValue;

  for (size_t z = 0; z < sizeZ; z++) {
    if (!grids[z]) { continue; }
    auto& array = grids[z]->ref();
    for (size_t x = 0; x < sizeX; x++) {
      for (size_t y = 0; y < sizeY; y++) {
        float v = array[x][y];
        if ((v != backgroundValue) && (v != lastValue)) {
          ++neededPixels;
        }
        lastValue = v;
      }
    }
  }

  grid.addDim("pixel", neededPixels);
  size_t dimIndex = grid.getDims().size() - 1;

  auto& pixels = grid.addFloat1DRef(Constants::PrimaryDataName, grid.getUnits(), { dimIndex });
  const std::string Units = "dimensionless";
  auto& pixel_z = grid.addShort1DRef("pixel_z", Units, { dimIndex });
  auto& pixel_y = grid.addShort1DRef("pixel_y", Units, { dimIndex });
  auto& pixel_x = grid.addShort1DRef("pixel_x", Units, { dimIndex });
  auto& counts  = grid.addInt1DRef("pixel_count", Units, { dimIndex });

  lastValue = backgroundValue;
  size_t at = 0;

  for (size_t z = 0; z < sizeZ; z++) {
    if (!grids[z]) { continue; }
    auto& array = grids[z]->ref();
    for (size_t x = 0; x < sizeX; x++) {
      for (size_t y = 0; y < sizeY; y++) {
        float v = array[x][y];
        if (v != backgroundValue) {
          if (v == lastValue) {
            ++counts[at - 1];
          } else {
            pixel_z[at] = z;
            pixel_x[at] = x;
            pixel_y[at] = y;
            pixels[at]  = v;
            counts[at]  = 1;
            ++at;
          }
        }
        lastValue = v;
      }
    }
  }

  grid.setDataType("SparseLatLonHeightGrid");
  return true;
} // SparseEncoder::sparse3DN

bool
SparseEncoder::sparse2D(DataGrid& grid, IOConfig& keys)
{
  return sparseT<2>(grid, keys);
}

bool
SparseEncoder::sparse3D(DataGrid& grid, IOConfig& keys)
{
  return sparseT<3>(grid, keys);
}

void
SparseEncoder::unsparse2D(DataGrid& grid, size_t num_x, size_t num_y, IOConfig& keys,
  const std::string& pixelX, const std::string& pixelY, const std::string& pixelCount)
{
  if (grid.getShort1D(pixelX)) {
    const auto s = grid.getSizes();
    if ((s.size() < 3) || (s[0] != num_x) || (s[1] != num_y)) {
      fLogSevere("Caller expects a {}x{} grid but dimensions 0,1 differ, can't unsparse.", num_x, num_y);
      return;
    }
  }
  unsparseT<2>(grid, keys, { { pixelX, pixelY } }, pixelCount);
}

void
SparseEncoder::unsparse3D(DataGrid& grid, size_t num_x, size_t num_y, size_t num_z, IOConfig& keys,
  const std::string& pixelX, const std::string& pixelY, const std::string& pixelZ, const std::string& pixelCount)
{
  if (grid.getShort1D(pixelZ)) {
    const auto s = grid.getSizes();
    if ((s.size() < 4) || (s[0] != num_z) || (s[1] != num_x) || (s[2] != num_y)) {
      fLogSevere("Caller expects a {}x{}x{} (z,x,y) grid but dimensions 0,1,2 differ, can't unsparse.", num_z, num_x,
        num_y);
      return;
    }
  }
  unsparseT<3>(grid, keys, { { pixelZ, pixelX, pixelY } }, pixelCount);
}

// Explicit Template Instantiations
template bool
SparseEncoder::sparseT<2>(DataGrid&, IOConfig&);
template bool
SparseEncoder::sparseT<3>(DataGrid&, IOConfig&);
template void
SparseEncoder::unsparseT<2>(DataGrid&, IOConfig&, const std::array<std::string, 2>&, const std::string&);
template void
SparseEncoder::unsparseT<3>(DataGrid&, IOConfig&, const std::array<std::string, 3>&, const std::string&);
} // namespace rapio
