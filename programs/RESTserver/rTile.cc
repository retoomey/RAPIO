#include "rTile.h"
#include <iostream>
#include <sys/stat.h>

#include <dirent.h>
#include <sys/stat.h>

#include <fstream>

#include "rDataType.h"
#include "rAttributeDataType.h"



using namespace rapio;

/*
 * A RAPIO algorithm for making tiles
 *
 * Basically all we do is use command parameters to
 * override the output settings dynamically.  These settings
 * are taken from rapiosettings.xml for 'default' behavior.
 *
 * Real time Algorithm Parameters and I/O
 * http://github.com/retoomey/RAPIO
 *:
 * @author Robert Toomey
 **/

//FIXME: colormap test
std::string colorm = "";
std::string path2 = "";
std::string path3 = "";
std::string filelist = "";
bool mult = false;
bool mult_list = false;
int lnum;

auto myMulti = std::make_shared<MultiDataType>();
std::vector<std::shared_ptr<DataType>> datatypes;

void
RAPIOTileAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription("RAPIO Tile Algorithm");
  o.setAuthors("Robert Toomey");

  // Optional zoom level, center lat and center lon
  // Note: Running as tile server you don't need any of these
  o.optional("zoom", "0", "Tile zoom level from 0 (whole world) up to 20 or more");
  o.optional("center-latitude", "38", "Latitude degrees for tile center");
  o.optional("center-longitude", "-98", "Longitude degrees for tile center");
  o.optional("image-height", "256", "Height or rows of the image in pixels");
  o.optional("image-width", "256", "Width or cols of the image in pixels");

  // Optional flags (my debugging stuff, or maybe future filters/etc.)
  o.optional("flags", "", "Extra flags for determining tile drawing");

  //FIXME: colormap test
  o.optional("map", "", "Colormap");
  o.boolean("multi", "Set this option on if using more than one image for a single picture.");
  o.boolean("multi_list", "for testing");
  o.optional("second_file","","If multi is turned on, enter the second file with this option.");
  o.optional("third_file","","If multi is turned on, enter the third file with this option.");
  o.optional("number_in_list", "", "number of files in the list, for testing file_list");
  o.optional("file_list", "", "If multi is turned on, enter a space-separated string of file names, such as 'firstfile secondfile thirdfile ...'");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOTileAlg::processOptions(RAPIOOptions& o)
{
  // Make an "<output>" string overriding the rapiosettings.xml.  These will
  // override any of those params. We only do a single level, which for now
  // at least is enough I think.  This API might develop more later if needed.
  // "<output mode="tile" suffix="png" cols="500" rows="500" zoom="12" centerLatDegs="35.22" centerLonDegs="-97.44"/>";

  //FIXME: colormap test
  colorm = o.getString("map");
  path2 = o.getString("second_file");
  path3 = o.getString("third_file");

  myOverride["mode"] = "tile"; // We want tile mode for output
  myOverride["zoom"] = o.getString("zoom");
  myOverride["cols"] = o.getString("image-width");
  myOverride["rows"] = o.getString("image-height");
  myOverride["centerLatDegs"] = o.getString("center-latitude");
  myOverride["centerLonDegs"] = o.getString("center-longitude");
  myOverride["flags"]         = o.getString("flags");

  // Steal the normal output location as suggested cache folder
  // This will probably work unless someone deliberately tries to break it by passing
  // our other crazier output options
  std::string cache = o.getString("o");

  if (cache.empty()) {
    cache = "CACHE";
  }
  myOverride["tilecachefolder"] = cache;
}

void
RAPIOTileAlg::processNewData(rapio::RAPIOData& d)
{

  const auto& infos    = ConfigParamGroupi::getIndexInputInfo();
  // Look for any data the system knows how to read...
  auto r = d.datatype<rapio::DataType>();

  if(datatypes.size() == infos.size()){

  }

  if(r != nullptr){

    r->setDataAttributeValue("ColorMap", colorm);

    datatypes.push_back(r);

    if(datatypes.size() == infos.size()){
      for(int i = 0; i < infos.size(); i++){
        myMulti->addDataType(datatypes[i]);
      }
      myMulti->setSendToWriterAsGroup(true);
      writeOutputProduct(r->getTypeName(), myMulti, myOverride); // Typename will be replaced by -O filters
  }
    /*if(mult_list == true){
      std::string ftemp = "";
      auto myMulti = std::make_shared<MultiDataType>();
      myMulti->addDataType(r);
      std::vector<std::shared_ptr<DataType>> datatypes;
      int count = 0;
      for(char & c : filelist){
        if(c != ' '){
          ftemp += c;
        }
        if(c == ' '){
          datatypes.push_back(IODataType::read<DataType>(ftemp));
          datatypes[count]->setDataAttributeValue("ColorMap",colorm);
          myMulti->addDataType(datatypes[count]);
          ftemp = "";
          count++;
        }
      }
      std::cout << "SIze of datatypes vector is: " << datatypes.size() << "\n";
      auto whats_this1 = myMulti->getDataType(0);
      std::cout << "TypeName: " << whats_this1->getTypeName() << "\n";
      auto whats_this2 = myMulti->getDataType(1);
      std::cout << "Size: " << myMulti->size() << "\n";

      myMulti->setSendToWriterAsGroup(true);

      writeOutputProduct(r->getTypeName(), myMulti, myOverride); // Typename will be replaced by -O filters
    } */
    else{

      //writeOutputProduct(r->getTypeName(), r, myOverride);
    }
  }



  /*if (r != nullptr) {
    LogInfo("-->Tile: " << r->getTypeName() << "\n");
    // Eh do we have to write it to disk?  Should stream it back, right?
    // We're gonna want to send it back on the command line right?
    writeOutputProduct(r->getTypeName(), r, myOverride); // Typename will be replaced by -O filters
  } */
} // RAPIOTileAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run a tile alg
  RAPIOTileAlg alg = RAPIOTileAlg();

  alg.executeFromArgs(argc, argv);
}
