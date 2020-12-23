#include "rIOImage.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rRadialSetLookup.h"

#include "rImageDataTypeImp.h"

// GraphicMagick in centos/fedora
// Not 100% sure yet we'll stick to this library,
// will expand a bit before making a requirement
// /usr/include/GraphicsMagick/Magick++.h
#include <GraphicsMagick/Magick++.h>
#include "rRadialSet.h"

using namespace rapio;
using namespace Magick;

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
{
  // Our geotiff read/write?
  //ImageTiff::introduceSelf();
}

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

IOImage::~IOImage()
{ }

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
IOImage::createDataType(const URL& path)
{
  // virtual to static
  return (IOImage::readImageDataType(path));
}

bool
IOImage::encodeDataType(std::shared_ptr<DataType> dt,
  const URL                                      & aURL,
  std::shared_ptr<DataFormatSetting>             dfs)
{
  // Subtype based on file suffix maybe?  I feel that
  // geotiff, jpg, png, etc. could all be grouped within
  // image
  // FIXME: First pass just trying to write a POC file
  // or do we do it by datatype...or both...
  // not sure yet, let's see it work...
  static bool setup = false;
  if (!setup){
    InitializeMagick(NULL);
    setup = true;
  }

  // First pass I'm ignoring a lot of boilerplate stuff

  // FIXME: cleanup.  Messy for now, just do RadialSets
  std::shared_ptr<RadialSet> radialSet = std::dynamic_pointer_cast<RadialSet>(dt);
  if (!radialSet){
    LogSevere("Image writer only handles RadialSet data at the moment\n");
    return false;
  }

  // FIXME: Refactoring xml settings/etc coming soon I think.  Hardcode here
  size_t cols = 500; // If not the same we squish, probably need to change that
  size_t rows = 500;
  size_t degreeOut = 1; // One degree lat/lon from center of radar for box

  // I actually still don't understand Lak's genius here lol
  RadialSet& r = *radialSet;
  // FIXME: API needs to be better.  Humm maybe just default name is primary internally?
  // Does this possibly allow hosting smart ptr to leave scope in optimization?
  auto& data = radialSet->getFloat2D("primary")->ref();
  auto myLookup = RadialSetLookup(r); // Bin azimuths

  // Calculate the lat lon 'box' of the image using sizes, etc.
  // Kinda of a pain actually.
  const auto centerLLH = r.getRadarLocation();

  // North America.  FIXME: General lat/lon box creator function
  // Latitude decreases as you go south
  // Longitude decreases as you go east
  auto lat = centerLLH.getLatitudeDeg();
  auto top = lat+degreeOut;
  auto bottom = lat-degreeOut;
  auto height = bottom-top;
  auto deltaLat = height/rows;

  auto lon = centerLLH.getLongitudeDeg();
  auto left = lon-degreeOut;
  auto right = lon+degreeOut;
  auto width = right-left;
  auto deltaLon = width/cols;

//  LogSevere("Center:" << lat << ", " << lon << "\n");
//  LogSevere("TopLeft:" << top << ", " << left << "\n");
//  LogSevere("BottomRight:" << bottom << ", " << right << "\n");
//  LogSevere("Delta values " << deltaLat << ", " << deltaLon << "\n");

  //Image image;
  Magick::Blob output_blob;
  try{
    // FIXME: Is there a way to create without 'begin' fill, would be faster
    Magick::Image i(Magick::Geometry(cols, rows), "white");

    // Determine if Warning exceptions are thrown.
    // Use is optional.  Set to true to block Warning exceptions.
    // image.quiet( false );
    i.opacity(true); // Breaks for me without this at moment.

    size_t nx = cols;
    size_t ny = rows;
    float azDegs, rangeMeters;
    int radial, gate;

    i.modifyImage();
    Magick::PixelPacket *pixel_cache = i.getPixels(0,0,nx,ny);

    // Old school vslice/cappi, etc...where we hunt in the data space
    // using the coordinate of the destination.
    auto startLat = top;
    for(size_t y=0; y<ny;y++){
      auto startLon = left;
      for(size_t x=0; x<nx; x++){
	  Magick::PixelPacket *pixel = pixel_cache+y*nx+x;

	 // Project Lat Lon to closest Az Range, then lookup radial/gate
         Project::LatLonToAzRange(lat, lon, startLat, startLon, azDegs, rangeMeters);
	 bool good = myLookup.getRadialGate(azDegs, rangeMeters, &radial, &gate);

	 // Color c = cm(v); bleh creating a color object like this has to be slower
	 double v = good? data[radial][gate]: Constants::MissingData;

	 // Current color mapping... FIXME: Full color map ability from wg2 ported next
	  if (v == Constants::MissingData){
	    *pixel = Magick::Color("blue");
	  }else{
            // Ok let's linearly map red to 'strength' of DbZ for POC
	    // working at moment with any fancy color mapping
	    // Magick::ColorRGB cc = Magick::ColorRGB(c.r/255.0, c.g/255.0, c.b/255.0);
	    // FIXME: Flush out color map ability
	    if (v > 200) v = 200;
	    if (v < -50.0) v = -50.0;
	    float weight = (250-v)/250.0;
             Magick::ColorRGB cc = Magick::ColorRGB(weight, 0, 0);
             *pixel = cc;
             // FIXME: transparent ability? if (transparent) {cc.alpha((255.0-c.a)/255.0); }
	  }
	  // if (x<10){ *pixel = Magick::Color("green");};
	  //if (y<10){ *pixel = Magick::Color("yellow");};
	startLon += deltaLon;
      }
      startLat += deltaLat;
    }
    i.syncPixels(); 

    // FIXME: Bleh this uses the suffix of the path to determine type.  
    // That will actually be good if we update the config ability
    // FIXME: Generalize and optimize the 'dfs' setting xml
    // FIXME: Do we write ourselves then compress, etc. or let this library
    // handle that directly here.
    i.write(aURL.toString()+".png");

  }catch( const Exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  return false;
}
