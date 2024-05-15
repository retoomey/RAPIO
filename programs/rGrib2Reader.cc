#include <rGrib2Reader.h>

#include <iostream>

using namespace rapio;

/** A Grib2 reader
 *
 * Using rrfs data:
 * rgrib2reader -i file=rrfs.t23z.prslev.f000.conus_3km.grib2 -model=RRFS -o=test
 *
 * @author Robert Toomey; Travis Smith
 **/

std::string ConfigModelInfoXML = "misc/modelRRFS.xml"; //default
const std::string modelProjectionsXML = "misc/modelProjections.xml";

bool myReadSettings = false;

//
// Model fields from xml config file
//
struct ModelFields
{
  /* data */
  std::string id;
  std::string type;
  std::string name;
  std::string units;
  std::string layer;
  std::string rotateWinds;

};
std::vector<ModelFields> mFields;

//
// have wind field conversion structures been initialized?
//
bool initWindConversion = false;
bool uWindGridPresent = false;
bool vWindGridPresent = false;

void
Grib2ReaderAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Grib2Reader reads in GRIB2 files and writes out netcdf");
  o.setAuthors("Robert Toomey;Travis Smith");
  o.optional("model", "RRFS", "which model? Valid models include:\n - RRFS\n - HRRR\n");
 // - RAP13\n - RAP20\n - RUC20\n - RUC40\n - RUC60\n - GFS\n
}

void
Grib2ReaderAlg::processOptions(RAPIOOptions& o)
{ 
  ConfigModelInfoXML = "misc/model" + o.getString("model") + ".xml";
}

void
Grib2ReaderAlg::getModelProjectionInfo(std::string& modeltype)
{ 
  auto doc = Config::huntXML(modelProjectionsXML);
  try {
    if (doc != nullptr) {
      auto tree       = doc->getTree();
      auto modelprojections = tree->getChildOptional("modelprojections");

      if (modelprojections != nullptr) {
        auto projections = modelprojections->getChildren("field");
        for (auto& m: projections) {
          // wget attributes
          const auto model = m.getAttr("model", std::string(""));
          if (model == modeltype) {
            //std::cout << "--------------Found " << modeltype << "!\n";
            inputx = m.getAttr("x", std::size_t());
            inputy = m.getAttr("y", std::size_t());
            outputlons = m.getAttr("outputlons", std::size_t());
            outputlats = m.getAttr("outputlats", std::size_t());
            nwlon = m.getAttr("nwlon", float(1.0));
            nwlat = m.getAttr("nwlat", float(1.0));
            selon = m.getAttr("selon", float(1.0));
            selat = m.getAttr("selat", float(1.0));
            zspacing = m.getAttr("zspacingkm", float(1.0));
            // FIXME: possible divide by zero
            latspacing = abs((nwlat - selat) / (float) outputlats);
            lonspacing = abs((nwlon - selon) / (float) outputlons);
            proj = m.getAttr("proj", std::string(""));
/*
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
              "Vert spacing = " << zspacing << " km\n" <<
              "proj string =" << proj << "\n" <<
              "\n";
	      */
            break;
          }

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
  auto doc = Config::huntXML(ConfigModelInfoXML);

  try
  {
    /* code */
    myReadSettings = true;
    size_t count = 0;
    if (doc != nullptr) {
      auto tree       = doc->getTree();
      auto modelfields = tree->getChildOptional("modelfields");

    
      if (modelfields != nullptr) {
        auto fields = modelfields->getChildren("field");
        for (auto& m: fields) {
          // wget attributes
          const auto id = m.getAttr("id", std::string(""));
          const auto type = m.getAttr("type", std::string(""));
          const auto name = m.getAttr("name", std::string(""));
          const auto units = m.getAttr("unit", std::string(""));
          const auto layer = m.getAttr("layer", std::string(""));
          const auto rotateWinds = m.getAttr("rotateWinds", std::string(""));
          //std::cout << id << " " << type << " " << name << " " << units
          //   << " " << layer << "windrot: " << rotateWinds << "\n";
          ModelFields mf;
          mf.id = id;
          mf.type = type;
          mf.name = name;
          mf.units = units;
          mf.layer = layer;
          mf.rotateWinds = rotateWinds;
          mFields.push_back(mf); 
          count++;  
          //std::cout << "Count = " << count << "\n";


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

/*
 * void
Grib2ReaderAlg::rotateWindGrids()

{
}
*/
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
        LogInfo("Trying to read " << mFields[i].id << " as grib message.\n");
        auto message = grib2->getMessage(mFields[i].id, mFields[i].layer);
        auto& m = *message;
        LogInfo("    Time of the message is " << m.getDateString() << "\n");
        LogInfo("    Message in file is number " << m.getMessageNumber() << "\n");
        LogInfo("    Message file byte offset: " << m.getFileOffset() << "\n");
        LogInfo("    Message byte length: " << m.getMessageLength() << "\n");
        // Time theTime = message->getTime();
        LogInfo("    Center ID is " << m.getCenterID() << "\n");
        LogInfo("    SubCenter ID is " << m.getSubCenterID() << "\n");

        // Messages have (n) fields each
        auto field = message->getField(1);
        if (field != nullptr) {
          auto& f = *field;
          LogInfo("For field 1 of the message:\n");
          LogInfo("    GRIB Version: " << f.getGRIBEditionNumber() << "\n");
          LogInfo("    GRIB Discipline #: " << f.getDisciplineNumber() << "\n");
          LogInfo("    Time of the field is " << f.getDateString() << "\n");
          LogInfo("    Grid def template number is: " << f.getGridDefTemplateNumber() << "\n");
        }
        auto llgridsp = LatLonGrid::Create(
          mFields[i].name,           // MRMS name and also colormap
          mFields[i].units,               // Units
          LLH(nwlat, nwlon, .500), // CONUS origin
          message->getTime(),
          latspacing,
          lonspacing,
          outputlats,
          outputlons
          /* data */
        );
        llgridsp->setSubType(mFields[i].layer);
	// call proj to convert to LatLonGrid from native projection
	// This bit sets up the projection (input from the modelProjections.xml)
        Project * project = new ProjLibProject(
          // axis: Tell project that our data is east and south heading
          //"+proj=lcc +axis=esu +lon_0=-98 +lat_0=38 +lat_1=33 +lat_2=45 +x_0=0 +y_0=0 +units=km +resolution=3"
          proj
        );
        bool success = project->initialize();
        LogInfo("Created projection:  " << success << "\n");
	//
        // Project from source project to LatLonGrid and write it out
	//
        if (success) {
          project->toLatLonGrid(array2Da, llgridsp);
          writeOutputProduct(llgridsp->getTypeName(), llgridsp);

	  //
	  // if this is a grid-relative wind field that needs to be
	  // converted to earth-relative, then stuff it in a holding structure
	  //

	  if (mFields[i].rotateWinds == "") {
          	  LogInfo("No winds to rotate\n");
	  } else {
		  std::vector<std::string> windinfo;
		  Strings::split(mFields[i].rotateWinds, ':', &windinfo);
        	  LogInfo("windinfo:  ");
		  for (size_t i=0;i<windinfo.size();i++) {
        	  	LogInfo(i << ":\"" << windinfo[i] << "\"\n");
		  }
        	  LogInfo("\n");
		  if (windinfo.size() > 0) {
			  if (!initWindConversion) {
				  //first time this has been called,
				  // so set up the arrays
        	                   LogInfo("Initializing Wind Rotation arrays\n");
				   initWindConversion = true;
	                           ugrid = LatLonGrid::Create(
                                   	windinfo[3],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
	                           vgrid = LatLonGrid::Create(
                                   	windinfo[4],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
				   uwind = LatLonGrid::Create(
                                   	windinfo[3],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
	                           vwind = LatLonGrid::Create(
                                   	windinfo[4],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
	
			  } 
			  if (windinfo[0] == "LCC") {
				  LogInfo("Doing Lambert Conformal\n");
                                  //"LCC:38.5:-97.5:UWind:VWind"
				  float lat = atof(windinfo[1].c_str());
				  float lon = atof(windinfo[2].c_str());
				  //FIXME?: this requires that the first letter
				  //of the data field be "U" or "V"
				  if (mFields[i].name.substr(0,1) == "U") {
                                          uWindGridPresent = true;
					  ugrid = llgridsp->Clone();
				  } else if (mFields[i].name.substr(0,1) == "V") {
                                          vWindGridPresent = true;
					  vgrid = llgridsp->Clone();
				  }
				  if (uWindGridPresent && vWindGridPresent) {
					  //convert call here
					  convertWinds(ugrid, vgrid, uwind, vwind, lat, lon);
                                          uWindGridPresent = false;
                                          vWindGridPresent = false;
					  // make sure we have the correct TypeName 
					  uwind->setTypeName(windinfo[3]);
					  vwind->setTypeName(windinfo[4]);
					  // write winds
          				  writeOutputProduct(uwind->getTypeName(), uwind);
          				  writeOutputProduct(vwind->getTypeName(), vwind);
				  }
			  } else if (windinfo[0] == "PS") {
				  LogInfo("Doing Polar Sterographic\n");
			  } else {
			          LogInfo("Projection not found\n");
			  }
		  } else {
			  LogInfo("Projection not found\n");
		  }


	  }
        } else {
          LogSevere("Failed to create projection\n");
          return;
        }
      }
    }
  }
} // Grib2ReaderAlg::processNewData

void
Grib2ReaderAlg::convertWinds(std::shared_ptr<rapio::LatLonGrid> ugrid,
                  std::shared_ptr<rapio::LatLonGrid> vgrid,
                  std::shared_ptr<rapio::LatLonGrid> &uwind,
                  std::shared_ptr<rapio::LatLonGrid> &vwind,
                  float xlat1, float cenlon) {

	uwind = ugrid->Clone();
	vwind = vgrid->Clone();

	float DTR=3.1415926/180.0;
	float cocon = sin(xlat1*DTR);
	auto& ugrid_ = ugrid->getFloat2DRef();
	auto& vgrid_ = vgrid->getFloat2DRef();
	auto& uwind_ = uwind->getFloat2DRef();
	auto& vwind_ = vwind->getFloat2DRef();
	// order here is vwind[lat][lon]
	LLH nwcorner = ugrid->getLocation();
	for (size_t j = 0; j < ugrid->getNumLats(); j++) { //lat
		for (size_t i = 0; i < ugrid->getNumLons(); i++) { //lon
			// this needs to be its own function
			// based on EMC code (FORTRAN subroutine W3FC03)
			// https://www.emc.ncep.noaa.gov/mmb/rreanl/w3fc03.f
			// also see this, as the code may not be relevant in the
			// S. hemisphere and at the poles (needs to be checked)
			// https://www.lib.ncep.noaa.gov/ncepofficenotes/files/014082A4.pdf
			LLH llhs = ugrid->getCenterLocationAt(j,i);
			float slon = llhs.getLongitudeDeg();	
			float angle = cocon * (slon-cenlon) * DTR;
			float a = cos(angle);
			float b = sin(angle);
			if (ugrid_[j][i] == Constants::MissingData ||
					ugrid_[j][i] == Constants::MissingData ||
					vgrid_[j][i] == Constants::DataUnavailable ||
					vgrid_[j][i] == Constants::DataUnavailable)
			{
				uwind_[j][i] == ugrid_[j][i];
				vwind_[j][i] == vgrid_[j][i];
			} else {
				uwind_[j][i] = a*ugrid_[j][i] + b*vgrid_[j][i];
				vwind_[j][i] = -b*ugrid_[j][i] + a*vgrid_[j][i];
//		  std::cout << "U = " << ugrid_[j][i] << "/" << uwind_[j][i]
//			  << " V = " << vgrid_[j][i] << "/" << vwind_[j][i] 
//			  << " Loc = " << slon  << "/" << llhs.getLatitudeDeg() 
//			  << " U2 = " << uwind->getFloat2DRef()[j][i]
//			  << "\n";
			}
		}
	}
} // Grib2ReaderAlg::convertWinds


int
main(int argc, char * argv[])
{
  // Run this thing standalone.
  Grib2ReaderAlg alg = Grib2ReaderAlg();

  alg.executeFromArgs(argc, argv);
}
