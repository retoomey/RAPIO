#include "rTextTimeHeightGrid.h"
#include "rConstants.h"
#include "rError.h"
#include <iostream>
#include <iomanip>

using namespace rapio;

TextTimeHeightGrid::~TextTimeHeightGrid() 
{ }

void 
TextTimeHeightGrid::introduceSelf(IOText * owner) 
{
  std::shared_ptr<IOSpecializer> newOne = std::make_shared<TextTimeHeightGrid>();
  owner->introduce("TimeHeightGrid", newOne);
}

std::shared_ptr<DataType> 
TextTimeHeightGrid::read(IOConfig& config) 
{
  // iotext is generally write-only for human-readable dumps via rdump
  return nullptr;
}

bool 
TextTimeHeightGrid::write(std::shared_ptr<DataType> dt, IOConfig& keys) 
{
  auto thg = std::dynamic_pointer_cast<TimeHeightGrid>(dt);
  if (!thg) {
    return false; // Not our datatype, pass it down the chain
  }

  auto timeArray = thg->getFloat1D("Time");
  auto htArray = thg->getFloat1D("Ht");
  auto dataArray = thg->getFloat2D(Constants::PrimaryDataName);

  if (!timeArray || !htArray || !dataArray) {
    fLogSevere("TimeHeightGrid missing required coordinate or data arrays for text output.");
    return false;
  }

  // Grab the internal data references
  auto& times = timeArray->ref();
  auto& heights = htArray->ref();
  auto& data = dataArray->ref();

  size_t num_times = thg->getNumTimes();
  size_t num_heights = thg->getNumHeights();

  std::cout << "\n=== TimeHeightGrid: " << thg->getTypeName() << " ===\n";
  std::cout << "Base Time: " << thg->getTime().getString("%Y-%m-%d %H:%M:%S UTC") << "\n";
  std::cout << "Units: " << thg->getUnits() << "\n\n";

  const int colW = 7; // Width for formatting columns

  // Print Header Row (Time offsets)
  std::cout << std::setw(10) << "Ht(m)\\T(s)";
  for (size_t t = 0; t < num_times; ++t) {
    std::cout << std::setw(colW) << static_cast<int>(times[t]);
  }
  std::cout << "\n" << std::string(10 + (num_times * colW), '-') << "\n";

  // Print Rows (Heights). Meteorological standard is highest altitude at the top.
  for (int h = static_cast<int>(num_heights) - 1; h >= 0; --h) {
    std::cout << std::setw(8) << static_cast<int>(heights[h]) << " |";
    
    for (size_t t = 0; t < num_times; ++t) {
      float val = data[t][h];
      
      // Handle Missing/Invalid Data elegantly
      if (!Constants::isGood(val)) {
        std::cout << std::setw(colW) << "---";
      } else {
        std::cout << std::setw(colW) << std::fixed << std::setprecision(1) << val;
      }
    }
    std::cout << "\n";
  }
  std::cout << std::string(10 + (num_times * colW), '=') << "\n\n";
  
  return true; // We successfully handled this write request
}
