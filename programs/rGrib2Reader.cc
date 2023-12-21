#include <rGrib2Reader.h>

#include <iostream>

using namespace rapio;

/** A Grib2 reader
 *
 * Using rrfs data:
 * rgrib2reader -i file=rrfs.t23z.prslev.f000.conus_3km.grib2 -o=test
 *
 * @author Robert Toomey; Travis Smith
 **/

const std::string ConfigModelInfoXML = "misc/modelRRFS.xml"; //FIXME configurable
const std::string modelProjectionsXML = "misc/modelProjections.xml";

bool myReadSettings = false;

struct ModelFields
{
  /* data */
  std::string id;
  std::string type;
  std::string name;
  std::string units;
  std::string layer;

};
std::vector<ModelFields> mFields;

void
Grib2ReaderAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Grib2Reader read in GRIB2 files and writes out netcdf");
  o.setAuthors("Robert Toomey;Travis Smith");
}

void
Grib2ReaderAlg::processOptions(RAPIOOptions& o)
{ }

void
Grib2ReaderAlg::getModelProjectionInfo(std::string& modeltype)
{ 
  std::cout << "Reading model projection info for " << modeltype
    << "\n";
  auto doc = Config::huntXML(modelProjectionsXML);
  try {
    if (doc != nullptr) {
      auto tree       = doc->getTree();
      auto modelprojections = tree->getChildOptional("modelprojections");
      std::cout << "not nullptr!\n";
      std::cout << tree << "\n\n\n------------\n";

      if (modelprojections != nullptr) {
        std::cout << "Also not nullptr!\n";
        auto projections = modelprojections->getChildren("field");
        std::cout << "Looking for" << modeltype << "\n";
        for (auto& m: projections) {
          // wget attributes
          const auto model = m.getAttr("model", std::string(""));
          if (model == modeltype) {
            std::cout << "--------------Found " << modeltype << "!\n";
            inputx = m.getAttr("x", std::size_t());
            inputy = m.getAttr("y", std::size_t());
            outputlons = m.getAttr("outputlons", std::size_t());
            outputlats = m.getAttr("outputlats", std::size_t());
            nwlon = m.getAttr("nwlon", float(1.0));
            nwlat = m.getAttr("nwlat", float(1.0));
            selon = m.getAttr("selon", float(1.0));
            selat = m.getAttr("selat", float(1.0));
            // FIXME: possible divide by zero
            latspacing = (nwlat - selat) / (float) outputlats;
            lonspacing = (nwlon - selon) / (float) outputlons;
            proj = m.getAttr("proj", std::string(""));
            std::cout << 
              "X = " << inputx << "\n" <<
              "Y = " << inputy << "\n" <<
              "# lons = " << outputlons << "\n" <<
              "# lats = " << outputlats << "\n" <<
              "NW lon = " << nwlon << "\n" <<
              "NW lat = " << nwlat << "\n" <<
              "SE lon = " << selon << "\n" <<
              "SE lat = " << selat << "\n" <<
              "Lonspacing = " << lonspacing << " deg -- " << lonspacing*111. << "km\n" <<
              "Latspacing = " << latspacing << " deg -- " << latspacing*111. << "km\n" <<
              "proj string =" << proj << "\n" <<
              "\n";
            /*
  size_t input_y; // # of input model rows
  size_t output_lons; // # of output columns
  size_t output_lats; // # of output rows
  float nwlon;  // NW corner of output
  float nwlat;  // NW corner of output
  float selon;  // SE corner of output
  float selat;  // SE corner of output
  std::string proj; 
  */
          }
          /*
          const auto type = m.getAttr("type", std::string(""));
          const auto name = m.getAttr("name", std::string(""));
          const auto units = m.getAttr("unit", std::string(""));
          const auto layer = m.getAttr("layer", std::string(""));
          std::cout << id << " " << type << " " << name << " " << units
             << " " << layer << "\n";
          ModelFields mf;
          mf.id = id;
          mf.type = type;
          mf.name = name;
          mf.units = units;
          mf.layer = layer;
          mFields.push_back(mf); 
          count++;  
          std::cout << "Count = " << count << "\n";
          */


        }
      }
    } else {
      std::cout << "could not reach model projections file\n";
    }
    
    
  } 
  catch(const std::exception& e)
  {
    LogSevere("Error parsing XML from " << ConfigModelInfoXML << "\n");
  }
  
}

void
Grib2ReaderAlg::whichFieldsToProcess()
{ 
  std::cout << "LOADING fields to process:\n";
  auto doc = Config::huntXML(ConfigModelInfoXML);

  try
  {
    /* code */
    myReadSettings = true;
    size_t count = 0;
    if (doc != nullptr) {
      auto tree       = doc->getTree();
      auto modelfields = tree->getChildOptional("modelfields");
      std::cout << "not nullptr!\n";
      std::cout << tree << "\n\n\n------------\n";

    
      if (modelfields != nullptr) {
        std::cout << "Also not nullptr!\n";
        auto fields = modelfields->getChildren("field");
        for (auto& m: fields) {
          // wget attributes
          const auto id = m.getAttr("id", std::string(""));
          const auto type = m.getAttr("type", std::string(""));
          const auto name = m.getAttr("name", std::string(""));
          const auto units = m.getAttr("unit", std::string(""));
          const auto layer = m.getAttr("layer", std::string(""));
          std::cout << id << " " << type << " " << name << " " << units
             << " " << layer << "\n";
          ModelFields mf;
          mf.id = id;
          mf.type = type;
          mf.name = name;
          mf.units = units;
          mf.layer = layer;
          mFields.push_back(mf); 
          count++;  
          std::cout << "Count = " << count << "\n";


        }
      }
      
    } else {
      std::cout << "no model info to process file found\n";
    }
  }
  
  catch(const std::exception& e)
  {
    LogSevere("Error parsing XML from " << ConfigModelInfoXML << "\n");
  }
  
}

void
Grib2ReaderAlg::processNewData(rapio::RAPIOData& d)
{
  // test loading field data
  std::string whichModel = "RRFS";
  getModelProjectionInfo(whichModel);
  whichFieldsToProcess();
  // Look for Grib2 data only
  auto grib2 = d.datatype<rapio::GribDataType>();

  if (grib2 != nullptr) {
    LogInfo("Grib2 data incoming...testing...\n");
    // ------------------------------------------------------------------------
    // Catalog test doing a .idx or wgrib2 listing attempt
    //
    {
      ProcessTimer grib("Testing print catalog speed\n");
      grib2->printCatalog();
      LogInfo("Finished in " << grib << "\n");
    }

    // test if we can pull the individual fields that are being asked for in 
    // the config file.

    for (size_t i=0;i<mFields.size();i++) {

      auto array2Da = grib2->getFloat2D(mFields[i].id, mFields[i].layer);
      if (array2Da != nullptr) {
        LogInfo("Found '" << mFields[i].id << "'\n");
        LogInfo("Dimensions: " << array2Da->getX() << ", " << array2Da->getY() << "\n");
      }

    }
    // ------------------------------------------------------------------------
    // 3D test
    //
    // This gets back layers into a 3D grid, which can be convenient for
    // certain types of data.  Also if you plan to work only with the data numbers
    // and don't care about transforming/projection say to mrms output grids.
    // Note that the dimensions have to match for all layers given
    const std::string name3D = "TMP";
    const std::vector<std::string> layers = { "2 mb", "5 mb", "7 mb" };

    LogInfo("Trying to read '" << name3D << "' as direct 3D from data...\n");
    auto array3D = grib2->getFloat3D(name3D, layers);

    if (array3D != nullptr) {
      LogInfo("Found '" << name3D << "'\n");
      LogInfo("Dimensions: " << array3D->getX() << ", " << array3D->getY() << ", " << array3D->getZ() << "\n");
    } else {
      LogSevere("Couldn't get 3D '" << name3D << "' out of grib data\n");
    }

    // ------------------------------------------------------------------------
    // 2D test
    //
    /**/
    const std::string name2D = "TMP";
    const std::string layer  = "surface";

    LogInfo("Trying to read " << name2D << " as direct 2D from data...\n");
    auto array2D = grib2->getFloat2D(name2D, layer);

    if (array2D != nullptr) {
      LogInfo("Found '" << name2D << "'\n");
      LogInfo("Dimensions: " << array2D->getX() << ", " << array2D->getY() << "\n");

      auto& ref = array2D->ref(); // or (*ref)[x][y]
      LogInfo("First 10x10 values:\n");
      LogInfo("----------------------------------------------------\n");
      for (size_t x = 0; x < 10; ++x) {
        for (size_t y = 0; y < 10; ++y) {
          std::cout << ref[x][y] << ",  ";
        }
        std::cout << "\n";
      }
      LogInfo("----------------------------------------------------\n");

      LogInfo("Trying to read " << name2D << " as grib message.\n");
      auto message = grib2->getMessage(name2D, layer);
      if (message != nullptr) {
        LogInfo("Success with higher interface..read message '" << name2D << "'\n");
        // FIXME: add methods for various things related to messages
        // and possibly fields.  We should be able to query a field
        // off a message at some point.
        // FIXME: Enums if this stuff is needed/useful..right now most just
        // return numbers which would make code kinda unreadable I think.
        // message->getFloat2D(optional fieldnumber == 1)
        auto& m = *message;
        LogInfo("    Time of the message is " << m.getDateString() << "\n");
        LogInfo("    Message in file is number " << m.getMessageNumber() << "\n");
        LogInfo("    Message file byte offset: " << m.getFileOffset() << "\n");
        LogInfo("    Message byte length: " << m.getMessageLength() << "\n");
        // Time theTime = message->getTime();
        LogInfo("    Center ID is " << m.getCenterID() << "\n");
        LogInfo("    SubCenter ID is " << m.getSubCenterID() << "\n");
      }


      // Create a brand new LatLonGrid
      size_t num_lats, num_lons;

      // Raw copy for orientation test.  Good for checking data orientation correct
      const bool rawCopy = false;
      if (rawCopy) {
        num_lats = array2D->getX();
        num_lons = array2D->getY();
      } else {
        num_lats = 1750;
        num_lons = 3500;
      }

      // Reverse spacing from the cells and range
      // FIXME: We could have another create method using degrees I think
      float lat_spacing = (55.0 - 10.0) / (float) (num_lats); // degree spread/cell count
      float lon_spacing = (130.0 - 60.0) / (float) num_lons;

      auto llgridsp = LatLonGrid::Create(
        "SurfaceTemp",           // MRMS name and also colormap
        "degreeK",               // Units
        LLH(55.0, -130.0, .500), // CONUS origin
        message->getTime(),
        lat_spacing,
        lon_spacing,
        num_lats, // X
        num_lons  // Y
      );

      // Just copy to check lat/lon orientation
      if (rawCopy) {
        auto& out   = llgridsp->getFloat2DRef();
        size_t numX = array2D->getX();
        size_t numY = array2D->getY();
        LogSevere("OUTPUT NUMX NUMY " << numX << ", " << numY << "\n");
        for (size_t x = 0; x < numX; ++x) {
          for (size_t y = 0; y < numY; ++y) {
            out[x][y] = ref[x][y];
          }
        }
      } else {
	      // test for temperature
        size_t numX = array2D->getX();
        size_t numY = array2D->getY();
        LogSevere("OUTPUT NUMX NUMY " << numX << ", " << numY << "\n");
        for (size_t x = 0; x < numX; ++x) {
          for (size_t y = 0; y < numY; ++y) {
            ref[x][y] = ref[x][y]-273.15;
          }
        }


        // ------------------------------------------------
        // ALPHA: Create projection lookup mapping
        // FIXME: Ok projection needs a LOT more work we need info
        // from the grib message on source projection, etc.
        // ALPHA: For moment just make one without caching or anything.
        // This is slow on first call, but we'd be able to cache in a
        // real time situation.
        // Also PROJ6 and up uses different strings
        // But basically we'll 'declare' our projection somehow
        Project * project = new ProjLibProject(
          // axis: Tell project that our data is east and south heading
          //"+proj=lcc +axis=esu +lon_0=-98 +lat_0=38 +lat_1=33 +lat_2=45 +x_0=0 +y_0=0 +units=km +resolution=3"
          "+proj=lcc +axis=esu +lon_0=-97.5 +lat_0=38.5 +lat_1=38.5 +lat_2=38.5 +x_0=0 +y_0=0 +units=km +resolution=3"
        );
        bool success = project->initialize();
        LogInfo("Created projection:  " << success << "\n");
        // Project from source project to output
        if (success) {
          project->toLatLonGrid(array2D, llgridsp);
        } else {
          LogSevere("Failed to create projection\n");
          return;
        }
      }
      // Typename will be replaced by -O filters
      writeOutputProduct(llgridsp->getTypeName(), llgridsp);
    } else {
      LogSevere("Couldn't read 2D data '" << name2D << "' from grib2.\n");
    }
  }
} // Grib2ReaderAlg::processNewData

int
main(int argc, char * argv[])
{
  // Run this thing standalone.
  Grib2ReaderAlg alg = Grib2ReaderAlg();

  alg.executeFromArgs(argc, argv);
}
