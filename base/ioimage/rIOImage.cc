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

  // Setup projection and magic
  static std::shared_ptr<ProjLibProject> project2;
  static bool setup = false;
  if (!setup) {
    // Force merc for first pass (matching standard tiles)
    project2 = std::make_shared<ProjLibProject>(
      // Web mercator
      "+proj=webmerc +datum=WGS84 +units=m +resolution=1",
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

  // Read settings
  LLCoverage cover;
  std::string suffix = "png";
  bool optionSuccess = false;

  std::string bbox   = "";
  std::string bboxsr = "3857"; // Spherical Mercator
  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      suffix        = output.getAttr("suffix", suffix);
      optionSuccess = p.getLLCoverage(output, cover);

      // Web tile mode, currently only passed by tiler
      try {
        bbox   = output.getAttr("BBOX", bbox);
        bboxsr = output.getAttr("BBOXSR", bboxsr);
      }catch (const std::exception& e) { }
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  if (!optionSuccess) {
    return false;
  }

  // Default is old way...pull back coverage.  We'll replace
  // them all with bbox calculation?
  size_t rows     = cover.rows;
  size_t cols     = cover.cols;
  double top      = cover.topDegs;
  double left     = cover.leftDegs;
  double deltaLat = cover.deltaLatDegs;
  double deltaLon = cover.deltaLonDegs;
  bool transform  = false;

  // New stuff for moment...trying to project correctly...
  // We'll create bbox from the zoom level...
  // FIXME: Sooo much cleanup will happen later..right now just want
  // things to start working.  I'm thinking projection library
  // will deal with lat/lon bboxes only..which will then be projected
  // to correct coordinate system..
  // Let's get center of the datatype...
  auto centerthing   = dt->getCenterLocation();
  double myCenterLat = centerthing.getLatitudeDeg();
  double myCenterLon = centerthing.getLongitudeDeg();

  if (bbox.empty()) { // New auto tile mode
    std::string mode     = "";
    int zoomLevel        = 0;
    double centerLatDegs = myCenterLat; // 35.22;
    double centerLonDegs = myCenterLon; // -97.44;
    try {
      auto output = dfs->getChild("output");
      mode      = output.getAttr("mode", mode);
      zoomLevel = output.getAttr("zoom", zoomLevel);
      centerLatDegs = output.getAttr("centerLatDegs", double(centerLatDegs));
      centerLonDegs = output.getAttr("centerLonDegs", double(centerLonDegs));
      // read rows, cols...

      // This 'should' be width in degrees of the tile at zoom level, everything else
      // has to be calculated in the projected space I think.  Basically divide the width
      // of the map (the earth)
      double halfDegWidth = 180.0 / pow(2, zoomLevel);

      // Project left and right sides to projected system..
      double xOutLeft, yOutLeft;

      double source1 = centerLonDegs - halfDegWidth;
      double source2 = centerLonDegs + halfDegWidth;

      project2->LatLonToXY(centerLatDegs, source1, xOutLeft, yOutLeft);
      double xOutRight, yOutRight;
      project2->LatLonToXY(centerLatDegs, source2, xOutRight, yOutRight);

      double widthx = abs(xOutRight - xOutLeft); // width of tile

      left      = xOutLeft;
      deltaLon  = widthx / cols;                      // March per pixel left to right
      deltaLat  = -(widthx / rows);                   // going 'down' decreases in lat and y
      top       = yOutLeft - (deltaLat * rows / 2.0); // Lat here is actually delta x
      transform = true;                               // go from x to lat lon for data lookup right?
    }catch (const std::exception& e) { }

    LogInfo(
      "Image writer settings: (filetype: " << suffix << ", " << cover << ")\n");
  } else {
    LogInfo(
      "Image writer tile:" << bbox << "\n");
  }

  #if HAVE_MAGICK
  auto colormap        = dtr.getColorMap();
  const ColorMap& test = *colormap;

  // const auto centerLLH = dtr.getLocation();

  double right  = 0;
  double bottom = 0;

  // Note here the variables are in the projection space
  if (!bbox.empty()) {
    std::vector<std::string> pieces;
    Strings::splitWithoutEnds(bbox, ',', &pieces);
    if (pieces.size() == 4) {
      // I think with projected it's upside down? or I've double flipped somewhere
      left   = std::stod(pieces[0]); // lon
      bottom = std::stod(pieces[1]); // lat // is it flipped in the y?? must be
      right  = std::stod(pieces[2]); // lon
      top    = std::stod(pieces[3]); // lat

      // Transform the points from lat/lon to merc if BBOXSR set to "4326"
      if (bboxsr == "4326") {
        double xout1, xout2, yout1, yout2;


        // test and die

        /*
         * double lattest = 30; double lontest = -80;
         * project2->LatLonToXY(lattest, lontest, xout1, yout1);
         * LogSevere("FIRST: 30, -80 " << xout1 << ", " << yout1 << "\n");
         *
         * double inx = xout1; double iny = yout1;
         * double latback, lonback;
         * project2->xyToLatLon(inx, iny, latback, lonback);
         * LogSevere("REVERSE: " << latback << ", " << lonback << "\n");
         */

        project2->LatLonToXY(bottom, left, xout1, yout1);
        project2->LatLonToXY(top, right, xout2, yout2);
        // project2->xyToLatLon(inx, iny, aLat, aLon);
        left   = xout1;
        bottom = yout1;
        right  = xout2;
        top    = yout2;

        // LogSevere("TRANSFORM TO " << top << ", " << left << ", " << bottom << ", " << right << "\n");
      }

      deltaLat  = (bottom - top) / rows; // good
      deltaLon  = (right - left) / cols;
      transform = true;
    }
  }

  // Final rendering
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

          # if 0
          double v  = abs(myCenterLon - aLon);
          double v2 = abs(myCenterLat - aLat);
          a = 255;
          r = 0;
          g = 0;
          b = 0;
          if (v < 2) { r = 255.0; } else { r = 0.0; }
          if (v2 < 2) { b = 255.0; } else { b = 0.0; }
          if ((v >= 2) && (v2 >= 2)) {
            const double vg = p.getValueAtLL(aLat, aLon);
            test.getColor(vg, r, g, b, a);
          }
          # endif // if 0
        } else {
          const double v = p.getValueAtLL(startLat, startLon);
          test.getColor(v, r, g, b, a);
        }

        // border
        # if 0
        if ((x < 2) || (y < 2) || (x > cols - 2) || (y > rows - 2)) {
          Magick::ColorRGB cc = Magick::ColorRGB(1.0, 0.0, 0.0);
          cc.alpha(0.0);
          *pixel = cc;
        } else {
        # endif
        Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
        cc.alpha(1.0 - (a / 255.0));
        *pixel = cc;
        # if 0
      }
        # endif
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
