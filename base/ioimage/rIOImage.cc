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
  std::map<std::string, std::string>              & keys
)
{
  DataType& dtr = *dt;

  // DataType LLH to data cell projection
  auto project = dtr.getProjection(); // primary layer

  if (project == nullptr) {
    LogSevere("No projection ability for this datatype.\n");
    return false;
  }
  auto& p = *project;

  // Setup projection and magic
  // This is destination projection
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

  // ----------------------------------------------------------------------
  // Read settings
  //
  std::string filename = keys["filename"];
  if (filename.empty()) {
    LogSevere("Need a filename to output\n");
    return false;
  }
  std::string suffix = keys["suffix"];
  if (suffix.empty()) {
    suffix = ".png";
  }

  std::string bbox, bboxsr;
  size_t rows;
  size_t cols;
  p.getBBOX(keys, rows, cols, bbox, bboxsr);
  std::string mode = keys["mode"];

  // FIXME: still need to cleanup suffix stuff
  if (keys["directfile"] == "false") {
    // We let writers control final suffix
    filename         = filename + "." + suffix;
    keys["filename"] = filename;
  }

  // ----------------------------------------------------------------------
  // Calculate bounding box for generating image
  // Leave code here for now...need to merge handle multiprojection
  // probably...but I want to prevent creating the projection over and over
  bool transform = false;
  if (bboxsr == "3857") { // if bbox in mercator need to project to lat lon for data lookup
    transform = true;
  }

  // Box settings
  double top      = 0;
  double left     = 0;
  double deltaLat = 0;
  double deltaLon = 0;
  if (!bbox.empty()) {
    std::vector<std::string> pieces;
    Strings::splitWithoutEnds(bbox, ',', &pieces);
    if (pieces.size() == 4) {
      // I think with projected it's upside down? or I've double flipped somewhere
      left = std::stod(pieces[0]);          // lon
      double bottom = std::stod(pieces[1]); // lat // is it flipped in the y?? must be
      double right  = std::stod(pieces[2]); // lon
      top = std::stod(pieces[3]);           // lat

      // Transform the points from lat/lon to merc if BBOXSR set to "4326"
      if (bboxsr == "4326") {
        double xout1, xout2, yout1, yout2;
        project2->LatLonToXY(bottom, left, xout1, yout1);
        project2->LatLonToXY(top, right, xout2, yout2);
        left      = xout1;
        bottom    = yout1;
        right     = xout2;
        top       = yout2;
        transform = true; // yes go from mer to lat for data
      }
      deltaLat = (bottom - top) / rows; // good
      deltaLon = (right - left) / cols;
    }
  }

  // ----------------------------------------------------------------------
  // Final rendering using the BBOX
  #if HAVE_MAGICK
  try{
    auto colormap        = dtr.getColorMap();
    const ColorMap& test = *colormap;
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

    i.write(filename);
  }catch (const Exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  #endif // if HAVE_MAGICK
  return false;
} // IOImage::encodeDataType
