#include "rIOImage.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"
// I'll leave for now.  May not be needed
// #include "rImageDataTypeImp.h"
#include "rImageDataType.h"

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
{
  #if HAVE_MAGICK
  // Make thread safe future proof
  // Note we never call DestroyMagick eh, modules currently
  // load up and typically never unload, so this works for
  // now unless we change that
  static std::once_flag magickInitFlag;
  std::call_once(magickInitFlag, []() {
    InitializeMagick(nullptr);
    fLogInfo("IOImage: Magick environment initialized at module load.");
  });
  #else
  fLogSevere("IOImage: Loaded module, but no Magick library installed!");
  #endif // if HAVE_MAGICK
}

IOImage::~IOImage()
{ }

void
IOImage::introduce(const std::string & name,
  std::shared_ptr<ImageType>         new_subclass)
{
  Factory<ImageType>::introduce(name, new_subclass);
}

// ---------------------------------------------------------
// SHARED MAGICK IMAGE GENERATOR (Fast Pass-By-Reference)
// ---------------------------------------------------------
#if HAVE_MAGICK
static bool
buildMagickImage(Magick::Image& i, std::shared_ptr<DataGrid> grid, std::map<std::string, std::string>& keys)
{
  auto sizes = grid->getSizes();

  if (sizes.size() < 2) { return false; }
  size_t rows = sizes[0];
  size_t cols = sizes[1];

  i.size(Magick::Geometry(cols, rows));
  i.magick("RGBA");
  i.opacity(false);
  i.depth(8);

  // Dynamic quality support
  long quality = 95;

  if (keys.count("quality")) { quality = std::stol(keys["quality"]); }
  i.quality(quality);

  i.modifyImage();
  Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);

  // --- BRANCH A: It's an ImageDataType ---
  if (auto imgArray = grid->getByte3D(Constants::PrimaryDataName)) {
    auto& data = imgArray->ref();
    for (size_t y = 0; y < rows; y++) {
      for (size_t x = 0; x < cols; x++) {
        Magick::ColorRGB cc(
          (uint8_t) data[y][x][0] / 255.0,
          (uint8_t) data[y][x][1] / 255.0,
          (uint8_t) data[y][x][2] / 255.0);
        cc.alpha(1.0 - ((uint8_t) data[y][x][3] / 255.0));
        *pixel++ = cc;
      }
    }
  }
  // --- BRANCH B: It's a Scientific Grid ---
  else if (auto floatArray = grid->getFloat2D(Constants::PrimaryDataName)) {
    auto aColorMap = grid->getColorMap();
    if (!aColorMap) {
      fLogSevere("Scientific grid requires a ColorMap to paint pixels.");
      return false;
    }
    auto& data = floatArray->ref();
    unsigned char r, g, b, a;

    for (size_t y = 0; y < rows; y++) {
      for (size_t x = 0; x < cols; x++) {
        aColorMap->getColor(data[y][x], r, g, b, a);
        Magick::ColorRGB cc(r / 255.0, g / 255.0, b / 255.0);
        cc.alpha(1.0 - (a / 255.0));
        *pixel++ = cc;
      }
    }
  } else {
    fLogSevere("IOImage: Unrecognized array configuration in DataGrid.");
    return false;
  }

  // --- 8-BIT INDEXED OPTIMIZATION ---
  if (keys["indexed"] == "true") {
    i.quantizeColors(256);
    i.quantizeDither(false);
    i.quantize();
  }

  i.syncPixels();
  return true;
} // buildMagickImage

#endif // if HAVE_MAGICK

std::shared_ptr<rapio::ImageType>
IOImage::getIOImage(const std::string& name)
{
  return (Factory<ImageType>::get(name, "Image builder"));
}

std::shared_ptr<DataType>
IOImage::readImageDataType(const URL& url)
{
  std::vector<char> buf;

  if ((IOURL::read(url, buf) <= 0) || buf.empty()) {
    fLogSevere("Couldn't read image datatype at {}", url.toString());
    return nullptr;
  }

  #if HAVE_MAGICK

  try {
    // 1. Give the raw file bytes to Magick to decode
    Magick::Blob blob(buf.data(), buf.size());
    Magick::Image img(blob);

    size_t cols     = img.columns();
    size_t rows     = img.rows();
    size_t channels = 4; // Normalize to RGBA for simplicity

    // 2. Create our new ImageDataType (which is a DataGrid under the hood)
    auto imageObj = ImageDataType::Create("Image", cols, rows, channels);
    auto& data    = imageObj->getByte3DRef(Constants::PrimaryDataName);

    // 3. Extract the decoded pixels
    // Note: getConstPixels forces Magick to decode the region into memory
    const Magick::PixelPacket * pixels = img.getConstPixels(0, 0, cols, rows);

    if (pixels) {
      size_t i = 0;
      for (size_t y = 0; y < rows; ++y) {
        for (size_t x = 0; x < cols; ++x) {
          // Note: Magick uses 16-bit Quantum depth in your CMake config,
          // so we scale it down to 8-bit bytes (0-255)
          data[y][x][0] = (int8_t) (pixels[i].red / 257);
          data[y][x][1] = (int8_t) (pixels[i].green / 257);
          data[y][x][2] = (int8_t) (pixels[i].blue / 257);
          data[y][x][3] = (int8_t) (255 - (pixels[i].opacity / 257)); // Magick uses opacity (0=opaque), we want alpha
          i++;
        }
      }
      fLogInfo("Successfully decoded {} ({}x{})", url.toString(), cols, rows);
      return imageObj;
    }
  } catch (Magick::Exception &error_) {
    fLogSevere("Magick++ exception decoding {}: {}", url.toString(), error_.what());
  }
  #else // if HAVE_MAGICK
  fLogSevere("ImageMagick/GraphicsMagick not compiled in, cannot decode image!");
  #endif // if HAVE_MAGICK

  return nullptr;
} // IOImage::readImageDataType

std::shared_ptr<DataType>
IOImage::createDataType(const std::string& params)
{
  // virtual to static
  return (IOImage::readImageDataType(URL(params)));
}

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

  // ADD THIS: Force fast compression for PNG tiles.  They will be bigger
  // if user doesn't request higher compression
  if ((suffix == "png") && (keys.count("quality") == 0)) {
    keys["quality"] = "10"; // Level 1 compression, 0 filtering
  }

  bool successful = false;

  try {
    if (suffix == "mrmstile") {
      successful = writeMRMSTile(dt, filename);
    } else {
      successful = writeMAGICKTile(dt, filename, keys);
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
} // IOImage::encodeDataType

bool
IOImage::writeMAGICKTile(std::shared_ptr<DataType> dt, const std::string& filename, std::map<std::string,
  std::string>& keys)
{
  #if HAVE_MAGICK
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (!grid) { return false; }

  Magick::Image i;
  if (!buildMagickImage(i, grid, keys)) { return false; }

  i.write(filename);
  return true;

  #else
  fLogSevere("Magick library not installed, can't write out!");
  return false;

  #endif // if HAVE_MAGICK
}

size_t
IOImage::encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer,
  std::map<std::string, std::string>              & keys
)
{
  #if HAVE_MAGICK
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);
  if (!grid) { return 0; }

  Magick::Image i;
  if (!buildMagickImage(i, grid, keys)) { return 0; }

  std::string format = keys["suffix"];
  if (format.empty()) { format = "PNG"; }
  Strings::toUpper(format);

  Magick::Blob blob;
  try {
    i.write(&blob, format);
    buffer.assign((char *) blob.data(), (char *) blob.data() + blob.length());
  } catch (std::exception& e) {
    fLogSevere("Magick failed to encode Blob to format {}: {}", format, e.what());
    return 0;
  }
  return buffer.size();

  #else // if HAVE_MAGICK
  return 0;

  #endif // if HAVE_MAGICK
}

bool
IOImage::writeMRMSTile(std::shared_ptr<DataType> dt, const std::string& filename)
{
  auto grid = std::dynamic_pointer_cast<DataGrid>(dt);

  if (!grid) {
    fLogSevere("IOImage requires a flattened DataGrid for MRMS tile writing. Received unsupported type: {}",
      dt->getDataType());
    return false;
  }

  auto array2D = grid->getFloat2D(Constants::PrimaryDataName);

  if (!array2D) {
    fLogSevere("Missing primary data array for MRMS tile writing.");
    return false;
  }

  auto sizes = grid->getSizes();

  if (sizes.size() < 2) { return false; }
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
} // IOImage::writeMRMSTile
