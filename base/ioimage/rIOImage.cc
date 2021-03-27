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
  // ----------------------------------------------------
  // FIXME: These settings duplicate a lot with other modules
  // like the GDAL one.  But in theory each module can have
  // completely unique settings. Debating on pulling this
  // into common code

  // Settings
  size_t cols      = 500;
  size_t rows      = 500;
  size_t degreeOut = 10.0;

  std::string mode   = "full";
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
      mode      = output.getAttr("mode", suffix);
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  DataType& r = *dt;
  float top, left, deltaLat, deltaLon;

  bool optionSuccess = false;
  if (mode == "full") {
    optionSuccess = r.LLCoverageFull(rows, cols, top, left, deltaLat, deltaLon);
  } else if (mode == "degrees") {
    optionSuccess = r.LLCoverageCenterDegree(degreeOut, rows, cols, top, left, deltaLat, deltaLon);
  }

  if (!optionSuccess) {
    LogSevere("Don't know how to create a coverage grid for this datatype using mode '" << mode << "'.\n");
    return false;
  }
  // ----------------------------------------------------

  LogInfo(
    "Image writer settings: (" << cols << "x" << rows << ") degrees:" << degreeOut << " filetype:" << suffix << "\n");

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
  auto colormap        = r.getColorMap();
  const ColorMap& test = *colormap;

  const auto centerLLH = r.getLocation();
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

        // if (v == Constants::MissingData) {
        //  *pixel = Magick::Color("blue");
        // } else {
        unsigned char r, g, b, a;
        test.getColor(v, r, g, b, a);
        Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
        *pixel = cc;
        // FIXME: transparent ability? if (transparent) {cc.alpha((255.0-c.a)/255.0); }
        // }
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
