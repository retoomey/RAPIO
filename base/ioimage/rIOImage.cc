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
  // Setup magic
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
  #else
    return false;
  #endif

  // Get DataType Array info to LatLon projection
  DataType& dtr = *dt;
  auto project = dtr.getProjection(); // primary layer
  if (project == nullptr) {
    LogSevere("No projection ability for this datatype.\n");
    return false;
  }
  auto& p = *project;

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

  // ----------------------------------------------------------------------
  // Bounding Box settings from given BBOX string
  size_t rows;
  size_t cols;
  double top      = 0;
  double left     = 0;
  double bottom   = 0;
  double right    = 0;
  auto proj = p.getBBOX(keys, rows, cols, left, bottom, right, top);

  // FIXME: still need to cleanup suffix stuff
  if (keys["directfile"] == "false") {
    // We let writers control final suffix
    filename         = filename + "." + suffix;
    keys["filename"] = filename;
  }

  bool transform = (proj != nullptr);

  // ----------------------------------------------------------------------
  // Final rendering using the BBOX
  bool box = keys["flags"] == "box";

  #if HAVE_MAGICK
  try{
    auto colormap        = dtr.getColorMap();
    const ColorMap& color = *colormap;
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
    auto deltaLat = (bottom - top) / rows; // good
    auto deltaLon = (right - left) / cols;
    unsigned char r, g, b, a;
    Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);
    for (size_t y = 0; y < rows; y++) {
      auto startLon = left;
      for (size_t x = 0; x < cols; x++) {
        if (transform) {
          double aLat, aLon;
          proj->xyToLatLon(startLon, startLat, aLat, aLon);
          const double v = p.getValueAtLL(aLat, aLon);
          color.getColor(v, r, g, b, a);
        } else {
          // Non transform for an equal angle based projection
          const double v = p.getValueAtLL(startLat, startLon);
          color.getColor(v, r, g, b, a);
        }

        // Box allows box around which is useful for debugging
        if (box && ((x < 2) || (y < 2) || (x > cols - 2) || (y > rows - 2))) {
           Magick::ColorRGB cc = Magick::ColorRGB(1.0, 0.0, 0.0);
           cc.alpha(0.0);
           *pixel = cc;
        }else{
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

    // Debug draw text on tile
    if (box){
      std::list<Drawable> text;
      text.push_back(DrawablePointSize(20));
      text.push_back(DrawableStrokeColor("black"));
      text.push_back(DrawableText(0, rows / 2, keys["TILETEXT"]));
      i.draw(text);
    }

    i.write(filename);
  }catch (const Exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  #endif // if HAVE_MAGICK
  return false;
} // IOImage::encodeDataType
