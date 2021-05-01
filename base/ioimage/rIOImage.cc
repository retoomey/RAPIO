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
  std::shared_ptr<PTreeNode>                      dfs,
  bool                                            directFile,
  // Output for notifiers
  std::vector<Record>                             & records
)
{
  DataType& dtr = *dt;

  // For now at least, every image requires a projection.  I
  // could see modes where no projection is required
  auto project = dtr.getProjection(); // primary layer

  if (project == nullptr) {
    LogSevere("No projection ability for this datatype.\n");
    return false;
  }
  auto& p = *project;

  LLCoverage cover;
  std::string suffix = "png";
  bool optionSuccess = false;

  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      suffix        = output.getAttr("suffix", suffix);
      optionSuccess = p.getLLCoverage(output, cover);
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  if (!optionSuccess) {
    return false;
  }
  // ----------------------------------------------------

  LogInfo(
    "Image writer settings: (filetype: " << suffix << ", " << cover << ")\n");

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
  auto colormap        = dtr.getColorMap();
  const ColorMap& test = *colormap;

  const auto centerLLH = dtr.getLocation();
  // Pull back settings in coverage for marching
  const size_t rows    = cover.rows;
  const size_t cols    = cover.cols;
  const float top      = cover.topDegs;
  const float left     = cover.leftDegs;
  const float deltaLat = cover.deltaLatDegs;
  const float deltaLon = cover.deltaLonDegs;

  try{
    Magick::Image i;
    i.size(Magick::Geometry(cols, rows));
    i.magick("RGBA");
    i.opacity(false); // Our colormaps support alpha, so we want it

    // Determine if Warning exceptions are thrown.
    // Use is optional.  Set to true to block Warning exceptions.
    // image.quiet( false );

    i.modifyImage();

    // Old school vslice/cappi, etc...where we hunt in the data space
    // using the coordinate of the destination.
    auto startLat = top;
    Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);
    for (size_t y = 0; y < rows; y++) {
      auto startLon = left;
      for (size_t x = 0; x < cols; x++) {
        // We'll do API over speed very slightly here, allow generic lat/lon pull ability
        const double v = p.getValueAtLL(startLat, startLon);

        unsigned char r, g, b, a;
        test.getColor(v, r, g, b, a);
        Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
        cc.alpha(1.0 - (a / 255.0));
        *pixel    = cc;
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
    IODataType::generateRecord(dt, aURL, "image", records);
  }catch (const Exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  #endif // if HAVE_MAGICK
  return false;
} // IOImage::encodeDataType
