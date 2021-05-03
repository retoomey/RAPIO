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
  // The projection is geodetic
  auto project = dtr.getProjection(); // primary layer

  if (project == nullptr) {
    LogSevere("No projection ability for this datatype.\n");
    return false;
  }
  auto& p = *project;

  LLCoverage cover;
  std::string suffix = "png";
  bool optionSuccess = false;

  std::string bbox = "";
  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      suffix        = output.getAttr("suffix", suffix);
      optionSuccess = p.getLLCoverage(output, cover);

      // Web tile mode, currently only passed by tiler
      try {
        bbox = output.getAttr("BBOX", bbox);
      }catch (const std::exception& e) { }
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  if (!optionSuccess) {
    return false;
  }

  if (bbox.empty()) {
    LogInfo(
      "Image writer settings: (filetype: " << suffix << ", " << cover << ")\n");
  } else {
    LogInfo(
      "Image writer tile:" << bbox << "\n");
  }

  static std::shared_ptr<ProjLibProject> project2;
  static bool setup = false;
  if (!setup) {
    // Force merc for first pass (matching standard tiles)
    project2 = std::make_shared<ProjLibProject>(
      "+proj=merc +units=m +resolution=1", // standard google tile spherical mercator
      // WDSSII default (probably doesn't even need to be parameter)
      "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
      );
    project2->initialize();

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
  size_t rows    = cover.rows;
  size_t cols    = cover.cols;
  float top      = cover.topDegs;
  float left     = cover.leftDegs;
  float deltaLat = cover.deltaLatDegs;
  float deltaLon = cover.deltaLonDegs;

  float right  = 0;
  float bottom = 0;

  // Note here the variables are in the projection space
  bool transform = false;
  if (!bbox.empty()) {
    std::vector<std::string> pieces;
    Strings::splitWithoutEnds(bbox, ',', &pieces);
    if (pieces.size() == 4) {
      left      = std::stod(pieces[0]);  // lon
      top       = std::stod(pieces[3]);  // lat
      right     = std::stod(pieces[2]);  // lon
      bottom    = std::stod(pieces[1]);  // lat
      deltaLat  = (bottom - top) / rows; // good
      deltaLon  = (right - left) / cols;
      transform = true;
    }
  }
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
        // Maybe combine these
        unsigned char r, g, b, a;
        // FIXME: refactor projection, push transform into it I think?
        if (transform) {
          double aLat, aLon;
          double inx = startLon; // actually from XY to lat lon (src/dst)
          double iny = startLat;
          project2->xyToLatLon(inx, iny, aLat, aLon);
          const double v = p.getValueAtLL(aLat, aLon);
          test.getColor(v, r, g, b, a);
        } else {
          const double v = p.getValueAtLL(startLat, startLon);
          test.getColor(v, r, g, b, a);
        }

        // border
        if ((x < 2) || (y < 2) || (x > cols - 2) || (y > rows - 2)) {
          Magick::ColorRGB cc = Magick::ColorRGB(1.0, 0.0, 0.0);
          cc.alpha(0.0);
          *pixel = cc;
        } else {
          Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
          cc.alpha(1.0 - (a / 255.0));
          *pixel = cc;
        }
        startLon += deltaLon;
        pixel++;
      }
      startLat += deltaLat;
    }
    i.syncPixels();

    # if 0
    double bLat, bLon;
    double inxb = top;
    double inyb = left;
    project2->xyToLatLon(inxb, inyb, bLat, bLon);
    std::list<Drawable> text;
    text.push_back(DrawablePointSize(40));
    text.push_back(DrawableStrokeColor("black"));
    text.push_back(DrawableText(0, rows / 2, std::to_string(top) + " " + std::to_string(left)));
    text.push_back(DrawableText(50, rows / 2 + 50, std::to_string(bottom) + " " + std::to_string(right)));
    // text.push_back(DrawableText(50,rows/2+100, std::to_string(deltaLat)));
    text.push_back(DrawableText(50, rows / 2 + 100, std::to_string(bLat) + " " + std::to_string(bLon)));
    i.draw(text);
    # endif // if 0

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
