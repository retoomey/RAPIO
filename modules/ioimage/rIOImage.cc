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
IOImage::writeMRMSTile(MultiDataType& mdt, const std::string& filename,
  ProjLibProject& proj, size_t rows, size_t cols, double top, double left, double deltaLat,
  double deltaLon, bool transform)
{
  // Precache color maps and data types for speed in loop lookup.
  // and check for null colormaps, null datatypes, etc.
  std::vector<DataProjection *> ps;
  size_t s = mdt.size();

  for (size_t i = 0; i < s; ++i) {
    auto aDT = mdt.getDataType(i);
    if (aDT) {
      auto aProjection = aDT->getProjection();
      if (aProjection) {
        ps.push_back(aProjection.get());
      }
    }
  }

  std::vector<float> buffer(rows * cols);

  size_t index  = 0;
  auto startLat = top;

  for (size_t y = 0; y < rows; y++) {
    auto startLon = left;
    for (size_t x = 0; x < cols; x++) {
      // Transform coordinate system if wanted
      double aLat = startLat, aLon = startLon;
      if (transform) {
        proj.xyToLatLon(startLon, startLat, aLat, aLon);
      }

      // Loop through datatypes.
      float v = Constants::DataUnavailable;
      for (size_t j = 0; j < ps.size(); ++j) {
        // Get the data value
        v = ps[j]->getValueAtLL(aLat, aLon);

        // For moment, first hit wins.  We could possible
        // add some sort of simple 'merge' later if wanted
        if (v != Constants::DataUnavailable) {
          break;
        }
      }

      buffer[index++] = v;
      startLon       += deltaLon;
    }
    startLat += deltaLat;
  }

  // Write the file...
  std::ofstream file(filename, std::ios::binary);

  if (!file) {
    LogSevere("Failed to open file: " << filename << "\n");
    return false;
  }

  file.write(reinterpret_cast<const char *>(buffer.data()), buffer.size() * sizeof(float));

  if (!file) {
    LogSevere("Failed to write data to file: " << filename << "\n");
    file.close();
    return false;
  }

  file.close();

  return true;
} // IOImage::writeMRMSTile

bool
IOImage::writeMAGICKTile(MultiDataType& mdt, const std::string& filename,
  ProjLibProject& proj, size_t rows, size_t cols, double top, double left, double deltaLat,
  double deltaLon, bool transform)
{
  // Precache color maps and data types for speed in loop lookup.
  // and check for null colormaps, null datatypes, etc.
  std::vector<DataProjection *> ps;
  std::vector<ColorMap *> cms;
  size_t s = mdt.size();

  for (size_t i = 0; i < s; ++i) {
    auto aDT = mdt.getDataType(i);
    if (aDT) {
      auto aColorMap   = aDT->getColorMap();
      auto aProjection = aDT->getProjection();
      if (aColorMap && aProjection) {
        ps.push_back(aProjection.get());
        cms.push_back(aColorMap.get());
      }
    }
  }

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
      // Transform coordinate system if wanted
      double aLat = startLat, aLon = startLon;
      if (transform) {
        proj.xyToLatLon(startLon, startLat, aLat, aLon);
      }

      // Loop through datatypes.
      for (size_t j = 0; j < ps.size(); ++j) {
        auto& p     = ps[j];
        auto& color = cms[j];

        // Get the data value
        const double v = p->getValueAtLL(aLat, aLon);
        color->getColor(v, r, g, b, a);

        // For moment, first hit wins.  We could possible
        // add some sort of simple 'merge' later if wanted
        if (v != Constants::DataUnavailable) {
          break;
        }
      }

      // Convert to Magick pixel
      Magick::ColorRGB cc = Magick::ColorRGB(r / 255.0, g / 255.0, b / 255.0);
      cc.alpha(1.0 - (a / 255.0));
      *pixel    = cc;
      startLon += deltaLon;
      pixel++;
    }
    startLat += deltaLat;
  }
  i.syncPixels();

  // Debug draw text on tile
  // Since we can do this in leaflet directly and easy, gonna deprecate
  // if (box) {
  //  std::list<Drawable> text;
  //  text.push_back(DrawablePointSize(20));
  //  text.push_back(DrawableStrokeColor("black"));
  //  text.push_back(DrawableText(0, rows / 2, keys["TILETEXT"]));
  //  i.draw(text);
  // }

  i.write(filename);
  return true;

  #else // if HAVE_MAGICK
  return false;

  #endif // if HAVE_MAGICK
} // IOImage::writeMAGICKTile

bool
IOImage::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys
)
{
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
    if (suffix == "mrms2dtile") {
      // try {
      // I'm thinking we label 'tiles' in the data by a single id
      // This will be 'partitioned' kinda like the grid partition, right?
      // Maybe the code can be shared even
      // id = std::stoi(keys["mrms2dtileid"]); // could except
      // }
    } else {
      // These tiles require projection or the BBOX

      // ----------------------------------------------------------------------
      // Bounding Box settings from given BBOX string
      // FIXME: Could put more of this into getBBOX I think.
      // This is also a marching box similar to the fusion/grid stuff so
      // could combine/reduce classes at some point
      size_t rows, cols;
      double top           = 0;
      double left          = 0;
      double bottom        = 0;
      double right         = 0;
      auto proj            = DataProjection::getBBOX(keys, rows, cols, left, bottom, right, top);
      const auto deltaLat  = (bottom - top) / rows; // good
      const auto deltaLon  = (right - left) / cols;
      const bool transform = (proj != nullptr);

      // If the incoming is a MultiDataType someone has already given us
      // a group of products to output......
      std::shared_ptr<MultiDataType> useMe;

      if (auto multiDT = std::dynamic_pointer_cast<MultiDataType>(dt)) {
        useMe = multiDT;
        // ... or we'll make it use a multi data type to allow combining multiple datatypes
        // into a single image.
      } else {
        useMe = std::make_shared<MultiDataType>();
        useMe->addDataType(dt);
      }

      if (suffix == "mrmstile") {
        successful = writeMRMSTile(*useMe, filename, *proj, rows, cols, top, left, deltaLat, deltaLon, transform);
      } else {
        successful =
          writeMAGICKTile(*useMe, filename, *proj, rows, cols, top, left, deltaLat, deltaLon, transform);
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
