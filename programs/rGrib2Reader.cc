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

std::string ConfigModelInfoXML = "misc/modelRRFS.xml"; //FIXME configurable
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
  o.setDescription("Grib2Reader read in GRIB2 files and writes out netcdf");
  o.setAuthors("Robert Toomey;Travis Smith");
  o.optional("modelConfigFile", "RRFS", "which model? (RRFS, HRRR, RAP13, RAC20, RUC20, RUC40, RUC60)");
}

void
Grib2ReaderAlg::processOptions(RAPIOOptions& o)
{ 
  ConfigModelInfoXML = "misc/model" + o.getString("modelConfigFile") + ".xml";
  std::cout << "1 = " << ConfigModelInfoXML << "\n";
}

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
            zspacing = m.getAttr("zspacingkm", float(1.0));
            // FIXME: possible divide by zero
            latspacing = abs((nwlat - selat) / (float) outputlats);
            lonspacing = abs((nwlon - selon) / (float) outputlons);
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
              "Vert spacing = " << zspacing << " km\n" <<
              "proj string =" << proj << "\n" <<
              "\n";
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
          const auto rotateWinds = m.getAttr("rotateWinds", std::string(""));
          std::cout << id << " " << type << " " << name << " " << units
             << " " << layer << "windrot: " << rotateWinds << "\n";
          ModelFields mf;
          mf.id = id;
          mf.type = type;
          mf.name = name;
          mf.units = units;
          mf.layer = layer;
          mf.rotateWinds = rotateWinds;
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
	                           static auto ugrid = LatLonGrid::Create(
                                   	windinfo[3],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
	                           static auto vgrid = LatLonGrid::Create(
                                   	windinfo[4],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
				   auto uwind = LatLonGrid::Create(
                                   	windinfo[3],
				   	mFields[i].units,
				   	LLH(nwlat, nwlon, .500), // origin
                                   	message->getTime(),
				   	latspacing,
				   	lonspacing, 
				   	outputlats,
				   	outputlons
				   );
	                           auto vwind = LatLonGrid::Create(
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
				  if (mFields[i].name.substr(0,1) == "U") {
					  std::cout << "it's U\n";
                                          uWindGridPresent = true;
				  } else if (mFields[i].name.substr(0,1) == "V") {
					  std::cout << "it's V\n";
                                          vWindGridPresent = true;
				  }
				  if (uWindGridPresent && vWindGridPresent) {
					  //convert call here
                                          uWindGridPresent = true;
                                          vWindGridPresent = true;
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


	  // call wind rotation code which should have logic like this:
	  /*
	   *
	   *
           SUBROUTINE W3FC03(SLON,FGU,FGV,CENLON,XLAT1,DIR,SPD)
C$$$   SUBPROGRAM  DOCUMENTATION  BLOCK
C
C SUBPROGRAM: W3FC03         GRID U,V WIND COMPS. TO DIR. AND SPEED
C   AUTHOR: ROGERS/BRILL     ORG: WD22       DATE: 00-03-27
C
C ABSTRACT: GIVEN THE GRID-ORIENTED WIND COMPONENTS ON A 
C   LAMBERT CONFORMAL GRID POINT, COMPUTE THE DIRECTION
C   AND SPEED OF THE WIND AT THAT POINT.  INPUT WINDS AT THE NORTH
C   POLE POINT ARE ASSUMED TO HAVE THEIR COMPONENTS FOLLOW THE WMO
C   STANDARDS FOR REPORTING WINDS AT THE NORTH POLE.
C   (SEE OFFICE NOTE 241 FOR WMO DEFINITION). OUTPUT DIRECTION
C   WILL FOLLOW WMO CONVENTION.
C
C PROGRAM HISTORY LOG:
C   00-03-27  BRILL/ROGERS 
C
C USAGE:  CALL W3FC03 (SLON,FGU,FGV,CENLON,XLAT1,DIR,SPD)
C
C   INPUT VARIABLES:
C     NAMES  INTERFACE DESCRIPTION OF VARIABLES AND TYPES
C     ------ --------- -----------------------------------------------
C     SLON   ARG LIST  REAL*4    STATION LONGITUDE (-DEG W)
C     FGU    ARG LIST  REAL*4    GRID-ORIENTED U-COMPONENT
C     FGV    ARG LIST  REAL*4    GRID-ORIENTED V-COMPONENT
C     CENLON ARG LIST  REAL*4    CENTRAL LONGITUDE
C     XLAT1  ARG LIST  REAL*4    TRUE LATITUDE #1
C
C   OUTPUT VARIABLES:
C     NAMES  INTERFACE DESCRIPTION OF VARIABLES AND TYPES
C     ------ --------- -----------------------------------------------
C     DIR    ARG LIST  REAL*4     WIND DIRECTION, DEGREES
C     SPD    ARG LIST  REAL*4     WIND SPEED
C
C   SUBPROGRAMS CALLED:
C     NAMES                                                   LIBRARY
C     ------------------------------------------------------- --------
C     ABS  ACOS   ATAN2   SQRT                                SYSTEM
C
C WARNING: THIS JOB WILL NOT VECTORIZE ON A CRAY
C
C ATTRIBUTES:
C   LANGUAGE: FORTRAN 90
C   MACHINE:  IBM SP
C
C$$$
      PARAMETER (DTR=3.1415926/180.0)
C
C  COMPUTE CONSTANT OF CONE
C
      COCON = SIN(XLAT1*DTR)
      ANGLE = COCON * (SLON-CENLON) * DTR
      A = COS(ANGLE)
      B = SIN(ANGLE)
      UNEW = A*FGU + B*FGV
      VNEW = -B*FGU + A*FGV
C
      CALL W3FC05(UNEW,VNEW,DIR,SPD)
      RETURN
      END
	   *       !
      ! Compute normal wind component and store in fg % u
      !

      truelat1 = field % truelat1
      truelat2 = field % truelat2
      xlonc = field % xlonc

      if( field % is_wind_grid_rel ) then
         call mpas_log_write(" is_wind_grid_rel is true ")
         call mpas_log_write(" truelat1, truelat2, xlonc are $r $r $r ",realArgs=(/truelat1, truelat2, xlonc/))

         IF (ABS(truelat1-truelat2) .GT. 0.1) THEN
            cone = ALOG10(COS(truelat1*rad_per_deg)) - &
                   ALOG10(COS(truelat2*rad_per_deg))
            cone = cone /(ALOG10(TAN((45.0 - ABS(truelat1)/2.0) * rad_per_deg)) - &
                   ALOG10(TAN((45.0 - ABS(truelat2)/2.0) * rad_per_deg)))        
         ELSE
            cone = SIN(ABS(truelat1)*rad_per_deg )  
         ENDIF

         call mpas_log_write(" cone value is $r ",realArgs=(/cone/))
         xlonc = rad_per_deg * xlonc
         call mpas_log_write(" rotating velocities ")
         call rotate_velocities(u_fg, v_fg, lonEdge, cone, xlonc, nfglevels_actual, nEdges)

      else 
         call mpas_log_write(" is_wind_grid_rel is false ")
      end if
      */
        } else {
          LogSevere("Failed to create projection\n");
          return;
        }
      }
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
