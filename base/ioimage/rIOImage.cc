#include "rIOImage.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"
#include "rImageDataTypeImp.h"

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
    LogSevere("Couldn't read image datatype at " << url << "\n");
  }

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
  const std::string                               & params,
  std::shared_ptr<XMLNode>                        dfs,
  bool                                            directFile,
  // Output for notifiers
  std::vector<Record>                             & records,
  std::vector<std::string>                        & files
)
{
  URL aURL(params);

  // Settings
  size_t cols      = 500;
  size_t rows      = 500;
  size_t degreeOut = 10.0;

  // The Imagick format/ending/etc.
  // FIXME: IODatatype does directory/file naming stuff so
  // we get a double ending at moment..will refactor
  std::string suffix = "png";
  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      cols = output.getAttr("cols", cols);
      rows = output.getAttr("rows", cols);
      // At least for now use the delta degree thing.  I'm sure
      // this will enhance or change for a static grid
      degreeOut = output.getAttr("degrees", size_t(degreeOut));
      suffix    = output.getAttr("suffix", suffix);
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }
  LogInfo(
    "Image writer settings: (" << cols << "x" << rows << ") degrees:" << degreeOut << " filetype:" << suffix << "\n");

  DataType& r = *dt;
  float top, left, deltaLat, deltaLon;
  if (!r.LLCoverageCenterDegree(degreeOut, rows, cols,
    top, left, deltaLat, deltaLon))
  {
    LogSevere("Don't know how to create a coverage grid for this datatype.\n");
    return false;
  }
  // ----------------------------------------------------

  static bool setup = false;
  if (!setup) {
    #if HAVE_MAGICK
    InitializeMagick(NULL);
    #else
    LogSevere("GraphicsMagick-c++-devel or ImageMagick-c++-devel is not installed, can't write out\n");
    #endif
    setup = true;
  }

  #if HAVE_MAGICK
  // For the moment, always have a color map, even if missing...
  static std::shared_ptr<ColorMap> colormap;
  if (colormap == nullptr) {
    colormap = ColorMap::readColorMap("/home/dyolf/Reflectivity.xml");
    if (colormap == nullptr) {
      colormap = std::make_shared<LinearColorMap>();
    }
  }
  const ColorMap& test = *colormap;

  // Calculate the lat lon 'box' of the image using sizes, etc.
  // Kinda of a pain actually.
  const auto centerLLH = r.getLocation();

  // DataType: getLatLonCoverage?  (default for RadialSet could be the degree thing?)

  // North America.  FIXME: General lat/lon box creator function
  // Latitude decreases as you go south
  // Longitude decreases as you go east
  // Ok we need a fixed aspect for non-square images...use the
  // long for the actual width

  /*  auto lon      = centerLLH.getLongitudeDeg();
   * auto left     = lon - degreeOut;
   * auto right    = lon + degreeOut;
   * auto width    = right - left;
   * auto deltaLon = width / cols;
   *
   * // To keep aspect ratio per cell, use deltaLon to back calculate
   * auto deltaLat = -deltaLon; // keep the same square per pixel
   * auto lat      = centerLLH.getLatitudeDeg();
   * auto top      = lat - (deltaLat * rows / 2.0);
   */

  try{
    // FIXME: Is there a way to create without 'begin' fill, would be faster
    // Magick::Image i(Magick::Geometry(cols, rows)); Fails missing file?
    Magick::Image i(Magick::Geometry(cols, rows), "white");

    // Determine if Warning exceptions are thrown.
    // Use is optional.  Set to true to block Warning exceptions.
    // image.quiet( false );
    i.opacity(true); // Breaks for me without this at moment.

    i.modifyImage();

    // Old school vslice/cappi, etc...where we hunt in the data space
    // using the coordinate of the destination.
    auto startLat = top;
    Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);
    for (size_t y = 0; y < rows; y++) {
      auto startLon = left;
      for (size_t x = 0; x < cols; x++) {
        // We'll do API over speed very slightly here, allow generic lat/lon pull ability
        const double v = r.getValueAtLL(startLat, startLon);

        // Current color mapping... FIXME: Full color map ability from wg2 ported next
        if (v == Constants::MissingData) {
          *pixel = Magick::Color("blue");
        } else {
          // Ok let's linearly map red to 'strength' of DbZ for POC
          // working at moment with any fancy color mapping
          // Magick::ColorRGB cc = Magick::ColorRGB(c.r/255.0, c.g/255.0, c.b/255.0);
          // FIXME: Flush out color map ability
          unsigned char r, g, b, a;
          test.getColor(v, r, g, b, a);
          Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
          *pixel = cc;
          // FIXME: transparent ability? if (transparent) {cc.alpha((255.0-c.a)/255.0); }
        }
        startLon += deltaLon;
        pixel++;
      }
      startLat += deltaLat;
    }
    i.syncPixels();

    // Our params are either a direct filename or a directory
    bool useSubDirs = true; // Use subdirs
    URL aURL        = IODataType::generateFileName(dt, params, suffix, directFile, useSubDirs);
    i.write(aURL.toString());
    // Make record
    IODataType::generateRecord(dt, aURL, "image", records, files);
  }catch (const Exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  #endif // if HAVE_MAGICK
  return false;
} // IOImage::encodeDataType
