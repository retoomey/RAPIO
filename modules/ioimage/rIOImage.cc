#include "rIOImage.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"
#include "rImageDataTypeImp.h"

#include <fstream>

// GraphicMagick or ImageMagick in centos/fedora
#if HAVE_MAGICK
# include <Magick++.h>
using namespace Magick;
#else
# warning "GraphicsMagick or ImageMagick libraries not found, currently ioimage will be disabled"
#endif

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOImage();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOImage::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the ImageMagick/GraphicsMagick library to output supported formats.";
  return help;
}

void
IOImage::initialize()
{ }

IOImage::~IOImage()
{ }

void
IOImage::introduce(const std::string & name,
  std::shared_ptr<ImageType>         new_subclass)
{
  Factory<ImageType>::introduce(name, new_subclass);
}

std::shared_ptr<rapio::ImageType>
IOImage::getIOImage(const std::string& name)
{
  return (Factory<ImageType>::get(name, "Image builder"));
}

std::shared_ptr<DataType>
IOImage::readImageDataType(const URL& url)
{
  std::vector<char> buf;

  IOURL::read(url, buf);

  if (!buf.empty()) {
    std::shared_ptr<ImageDataTypeImp> g = std::make_shared<ImageDataTypeImp>(buf);
    return g;
  } else {
    fLogSevere("Couldn't read image datatype at {}", url.toString());
  }

  return nullptr;
} // IOImage::readImageDataType

std::shared_ptr<DataType>
IOImage::createDataType(const std::string& params)
{
  // virtual to static
  return (IOImage::readImageDataType(URL(params)));
}

#if 0
bool
IOImage::writeMAGICKTile(std::shared_ptr<DataType> dt, const std::string& filename)
{
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);
  auto aColorMap = dt->getColorMap();
  
  if (!grid || !aColorMap) {
    fLogSevere("Invalid grid or colormap for image writing.");
    return false;
  }
  
  auto array2D = grid->getFloat2D(Constants::PrimaryDataName);
  if (!array2D) {
    fLogSevere("Missing primary data array for image writing.");
    return false;
  }

  auto sizes = grid->getSizes();
  if (sizes.size() < 2) return false;
  size_t rows = sizes[0];
  size_t cols = sizes[1];
  
  static bool setup = false;
  if (!setup) {
#if HAVE_MAGICK
    InitializeMagick(NULL);
#else
    fLogSevere("GraphicsMagick-c++-devel or ImageMagick-c++-devel is not installed, can't write out!");
#endif
    setup = true;
  }

#if HAVE_MAGICK
  Magick::Image i;
  i.size(Magick::Geometry(cols, rows));
  i.magick("RGBA");
  i.opacity(false);
  i.modifyImage();
  
  unsigned char r = 0, g = 0, b = 0, a = 0;
  Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);
  auto& data = array2D->ref();
  
  for (size_t y = 0; y < rows; y++) {
    for (size_t x = 0; x < cols; x++) {
      float v = data[y][x];
      
      // Paint the pixel based on the colormap
      aColorMap->getColor(v, r, g, b, a);

      Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
      cc.alpha(1.0 - (a / 255.0)); 
      *pixel = cc;
      pixel++;
    }
  }
  i.syncPixels();
  i.write(filename);
  return true;
#else
  return false;
#endif
}

bool
IOImage::writeMRMSTile(std::shared_ptr<DataType> dt, const std::string& filename)
{
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (!grid) return false;
  
  auto array2D = grid->getFloat2D(Constants::PrimaryDataName);
  if (!array2D) return false;

  auto sizes = grid->getSizes();
  if (sizes.size() < 2) return false;
  size_t rows = sizes[0];
  size_t cols = sizes[1];
  
  auto& data = array2D->ref();
  std::vector<float> buffer(rows * cols);
  size_t index = 0;
  
  for (size_t y = 0; y < rows; y++) {
    for (size_t x = 0; x < cols; x++) {
      buffer[index++] = data[y][x];
    }
  }
  
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    fLogSevere("Failed to open file: {}", filename);
    return false;
  }
  file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size() * sizeof(float));
  file.close();
  return true;
}

bool
IOImage::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys
)
{
  std::string suffix = keys["suffix"];
  std::string filename;
  if (!resolveFileName(keys, suffix, "image-", filename)) {
    return false;
  }
  
  bool successful = false;
  try {
    // The web server or rGetTile already handed us a perfectly pre-sized LatLonGrid!
    // Just pass it directly to the writers.
    if (suffix == "mrmstile") {
      successful = writeMRMSTile(dt, filename);
    } else {
      successful = writeMAGICKTile(dt, filename);
    }
    
    if (successful) {
      successful = postWriteProcess(filename, keys);
      showFileInfo("Image writer: ", keys);
    }
  } catch (const std::exception& e) {
    fLogSevere("Exception during image output: {}", e.what());
  }
  return successful;
}

#endif

bool
IOImage::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys
)
{
  std::string suffix = keys["suffix"];
  std::string filename;
  
  // FIX: Hardcode "png" as the default fallback so Magick++ knows what to encode!
  if (!resolveFileName(keys, "png", "image-", filename)) {
    return false;
  }
  
  // If the user didn't specify a suffix, default our internal router to PNG
  if (suffix.empty()) {
    suffix = "png";
  }
  
  bool successful = false;
  try {
    if (suffix == "mrmstile") {
      successful = writeMRMSTile(dt, filename);
    } else {
      successful = writeMAGICKTile(dt, filename);
    }
    
    if (successful) {
      successful = postWriteProcess(filename, keys);
      showFileInfo("Image writer: ", keys);
    } else {
      fLogSevere("IOImage: The underlying writer (MAGICK or MRMS) failed to generate the image.");
    }
  } catch (const std::exception& e) {
    fLogSevere("IOImage: Magick++ threw a fatal exception during image output: {}", e.what());
  }
  return successful;
}

bool
IOImage::writeMAGICKTile(std::shared_ptr<DataType> dt, const std::string& filename)
{
  // FIX: Add loud logging so we aren't guessing why it failed
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (!grid) {
    fLogSevere("IOImage requires a flattened DataGrid. Received unsupported type: {}", dt->getDataType());
    return false;
  }
  
  auto aColorMap = dt->getColorMap();
  if (!aColorMap) {
    fLogSevere("IOImage requires a valid ColorMap to paint pixels.");
    return false;
  }
  
  auto array2D = grid->getFloat2D(Constants::PrimaryDataName);
  if (!array2D) {
    fLogSevere("IOImage requires a primary 2D Float array to paint.");
    return false;
  }

  auto sizes = grid->getSizes();
  if (sizes.size() < 2) return false;
  size_t rows = sizes[0];
  size_t cols = sizes[1];
  
  static bool setup = false;
  if (!setup) {
#if HAVE_MAGICK
    InitializeMagick(NULL);
#else
    fLogSevere("GraphicsMagick-c++-devel or ImageMagick-c++-devel is not installed, can't write out!");
    return false;
#endif
    setup = true;
  }

#if HAVE_MAGICK
  Magick::Image i;
  i.size(Magick::Geometry(cols, rows));
  i.magick("RGBA");
  i.opacity(false);
  i.modifyImage();
  
  unsigned char r = 0, g = 0, b = 0, a = 0;
  Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);
  auto& data = array2D->ref();
  
  for (size_t y = 0; y < rows; y++) {
    for (size_t x = 0; x < cols; x++) {
      float v = data[y][x];
      
      // Paint the pixel based on the colormap
      aColorMap->getColor(v, r, g, b, a);

      Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
      cc.alpha(1.0 - (a / 255.0)); 
      *pixel = cc;
      pixel++;
    }
  }
  i.syncPixels();
  i.write(filename);
  return true;
#else
  return false;
#endif
}

bool
IOImage::writeMRMSTile(std::shared_ptr<DataType> dt, const std::string& filename)
{
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (!grid) {
    fLogSevere("IOImage requires a flattened DataGrid for MRMS tile writing. Received unsupported type: {}", dt->getDataType());
    return false;
  }
  
  auto array2D = grid->getFloat2D(Constants::PrimaryDataName);
  if (!array2D) {
    fLogSevere("Missing primary data array for MRMS tile writing.");
    return false;
  }

  auto sizes = grid->getSizes();
  if (sizes.size() < 2) return false;
  size_t rows = sizes[0];
  size_t cols = sizes[1];
  
  auto& data = array2D->ref();
  std::vector<float> buffer(rows * cols);
  size_t index = 0;
  
  for (size_t y = 0; y < rows; y++) {
    for (size_t x = 0; x < cols; x++) {
      buffer[index++] = data[y][x];
    }
  }
  
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    fLogSevere("Failed to open file: {}", filename);
    return false;
  }
  file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size() * sizeof(float));
  file.close();
  return true;
}
