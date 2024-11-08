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
IOImage::writeMRMSTile(const std::string& filename, DataProjection& p, ProjLibProject& proj, size_t rows, size_t cols, double top, double left, double deltaLat, double deltaLon, bool transform, const ColorMap& color)
{ 
    std::vector<float> buffer(rows*cols);

    size_t index = 0;
    auto startLat = top;
    for (size_t y = 0; y < rows; y++) {
      auto startLon = left;
      for (size_t x = 0; x < cols; x++) {

	// Get the data value
	float v;
        if (transform) {
          double aLat, aLon;
          proj.xyToLatLon(startLon, startLat, aLat, aLon);
          v = p.getValueAtLL(aLat, aLon);
        } else {
          v = p.getValueAtLL(startLat, startLon);
        }

	buffer[index++] = v;
        startLon += deltaLon;
      }
      startLat += deltaLat;
    } 

    // Write the file...
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        LogSevere("Failed to open file: " << filename << "\n");
        return false;
    }

    file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(float));

    if (!file) {
        LogSevere("Failed to write data to file: " << filename << "\n");
        file.close();
	return false;
    }

    file.close();

    return true;
}

bool
IOImage::writeMAGICKTile(const std::string& filename, DataProjection& p, ProjLibProject& proj, size_t rows, size_t cols, double top, double left, double deltaLat, double deltaLon, bool transform, const ColorMap& color)
{ 
  // --------------------------------------
  // Setup magic only if we need it
  //
  static bool setup = false;

  if (!setup) {
    #if HAVE_MAGICK
    InitializeMagick(NULL);
    #else
    LogSevere("GraphicsMagick-c++-devel or ImageMagick-c++-devel is not installed, can't write out!\n");
    #endif
    setup = true;
  }
#if HAVE_MAGICK
  // --------------------------------------

  // NOTE: JPEG doesn't support transparency,
  // so if you're here to fix that give up.
  //
  Magick::Image i;
  i.size(Magick::Geometry(cols, rows));
  i.magick("RGBA");
  i.opacity(false); // Our colormaps support alpha, so we want it
  // image.quiet( false ); // Warning exceptions if wanted

  i.modifyImage();

  // Old school vslice/cappi, etc...where we hunt in the data space
  // using the coordinate of the destination.
  auto startLat = top;
  unsigned char r, g, b, a;
  Magick::PixelPacket * pixel = i.getPixels(0, 0, cols, rows);
  for (size_t y = 0; y < rows; y++) {
    auto startLon = left;
    for (size_t x = 0; x < cols; x++) {

      // Get the data value
      if (transform) {
        double aLat, aLon;
        proj.xyToLatLon(startLon, startLat, aLat, aLon);
        const double v = p.getValueAtLL(aLat, aLon);
        color.getColor(v, r, g, b, a);
      } else {
        // Non transform for an equal angle based projection
        const double v = p.getValueAtLL(startLat, startLon);
        color.getColor(v, r, g, b, a);
      }

      // Convert to Magick pixel
      Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
      cc.alpha(1.0 - (a / 255.0));
      *pixel = cc;
      startLon += deltaLon;
      pixel++;
    }
    startLat += deltaLat;
  }
  i.syncPixels();

  // Debug draw text on tile
  // Since we can do this in leaflet directly and easy, gonna deprecate
  //if (box) {
  //  std::list<Drawable> text;
  //  text.push_back(DrawablePointSize(20));
  //  text.push_back(DrawableStrokeColor("black"));
  //  text.push_back(DrawableText(0, rows / 2, keys["TILETEXT"]));
  //  i.draw(text);
  // }

  i.write(filename);
  return true;
#else
  return false;
#endif
}

bool
IOImage::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys
)
{
  // Get DataType Array info to LatLon projection
  DataType& dtr = *dt;
  auto project  = dtr.getProjection(); // primary layer
  if (project == nullptr) {
    LogSevere("No projection ability for this datatype.\n");
    return false;
  }
  auto& p = *project;

  // ----------------------------------------------------------
  // Get the filename we should write to
  std::string suffix = keys["suffix"];
  std::string filename;
  if (!resolveFileName(keys, suffix, "image-", filename)) {
    return false;
  }

  // ----------------------------------------------------------------------
  // Read settings
  //
  bool successful = false;

  try{
     std::string suffix = keys["suffix"];

     // A data tile attempt for 2D tabling in the imgui display.
     // This will break up the 2D array of the data into tiles, without
     // any projection, giving us a 2D tracking table.  
     if (suffix == "mrms2dtile"){
       //try {
       // I'm thinking we label 'tiles' in the data by a single id
       // This will be 'partitioned' kinda like the grid partition, right?
       // Maybe the code can be shared even
       //id = std::stoi(keys["mrms2dtileid"]); // could except
       // }
     }else{

     // These tiles require projection or the BBOX
     
     // ----------------------------------------------------------------------
     // Bounding Box settings from given BBOX string
     size_t rows, cols;
     double top    = 0;
     double left   = 0;
     double bottom = 0;
     double right  = 0;
     auto proj     = p.getBBOX(keys, rows, cols, left, bottom, right, top);
     const auto deltaLat = (bottom - top) / rows; // good
     const auto deltaLon = (right - left) / cols;
     const bool transform = (proj != nullptr);

     auto colormap         = dtr.getColorMap();
     const ColorMap& color = *colormap;

     if (suffix == "mrmstile"){
       successful = writeMRMSTile(filename, p, *proj, rows, cols, top, left, deltaLat, deltaLon, transform, color);
     }else{
       successful = writeMAGICKTile(filename, p, *proj, rows, cols, top, left, deltaLat, deltaLon, transform, color);
     }
     }

     // ----------------------------------------------------------
     // Post processing such as extra compression, ldm, etc.
     if (successful) {
       successful = postWriteProcess(filename, keys);
     }
  }catch (const Exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  return successful;
} // IOImage::encodeDataType
