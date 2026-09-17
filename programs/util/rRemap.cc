#include <rRemap.h>
#include <rArrayPipeline.h>
#include <rLatLonGrid.h>
#include <rRadialSet.h>
#include <rLatLonGridProjection.h>
#include <rRadialSetProjection.h>

using namespace rapio;

void
Remap::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "Remap is used to remap LatLonArea based classes to another grid specification.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid("nw(37, -100) se(30.5, -93) h(0.5,0.5,1) s(0.01, 0.01)");
  
  o.optional("mode", "nearest", "Pipeline string for output generation.");
  o.optional("size", "3", "Size of matrix if not nearest.  For example, Cressman and bilinear");
  
  // RadialSet options
  o.optional("g", "-1", "Gatewidth in meters, -1 for source");
  o.addGroup("g", "RadialSet");
  
  o.optional("radials", "-1", "Radial count, -1 for source");
  o.addGroup("radials", "RadialSet");
  
  o.optional("gates", "-1", "Gate count, -1 for source");
  o.addGroup("gates", "RadialSet");
  
  o.boolean("ground", "Project to ground..");
  o.addGroup("ground", "RadialSet");
}

void
Remap::declareAdvancedHelp(RAPIOOptions& o)
{
  // Hook this up to ArrayPipeline::introduceHelp() if implemented
}

void
Remap::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);
  
  myMode = o.getString("mode");
  mySize = o.getInteger("size");
  if (mySize < 1) {
    mySize = 1;
  }
  if (mySize > 40) {
    mySize = 40;
  }
  
  myGateWidthMeters = o.getInteger("g");
  myNumRadials      = o.getInteger("radials");
  myNumGates        = o.getInteger("gates");
  myProjectGround   = o.getBoolean("ground");
}

void Remap::remap(std::shared_ptr<LatLonGrid> llg) {
    fLogInfo("Remapping LatLonGrid using ArrayPipeline...");
    
    std::string typeName = llg->getTypeName();
    std::string units    = llg->getUnits();
    Time time = llg->getTime();
    
    auto out = LatLonGrid::Create(typeName, units,
        LLH(myFullGrid.getNWLat(), myFullGrid.getNWLon(), 0), time,
        myFullGrid.getLatSpacing(), myFullGrid.getLonSpacing(),
        myFullGrid.getNumY(), myFullGrid.getNumX());
        
    out->getFloat2D()->fill(Constants::DataUnavailable);
    
    std::string pipelineConfig = myMode;
    if (myMode == "cressman" || myMode == "bilinear") {
        pipelineConfig += ":" + std::to_string(mySize) + ":" + std::to_string(mySize);
    }
    
    // Use our new LatLonGridMapper
    LatLonGridMapper geoMapper(*llg, *out);
    auto pipeline = ArrayPipeline::create(pipelineConfig);
    
    if (pipeline) {
        pipeline->remap(llg->getFloat2D(), out->getFloat2D(), geoMapper);
        writeOutputProduct(out->getTypeName(), out);
    } else {
        fLogSevere("Failed to create ArrayPipeline from config '{}'", pipelineConfig);
    }
}

void
Remap::remap(std::shared_ptr<RadialSet> rs)
{
  fLogInfo("Remapping RadialSet using ArrayPipeline...");
  
  auto gateWidthMeters = myGateWidthMeters;
  if (myGateWidthMeters < 0) {
    gateWidthMeters = rs->getGateWidthKMs() * 1000.0;
  }
  
  auto numRadials = myNumRadials;
  if (myNumRadials < 0) {
    numRadials = rs->getNumRadials();
  }
  
  auto numGates = myNumGates;
  if (myNumGates < 0) {
    numGates = rs->getNumGates();
  }
  
  // Create the empty destination RadialSet
  auto out = RadialSet::Create(rs->getTypeName(), rs->getUnits(),
      rs->getRadarLocation(), rs->getTime(), rs->getElevationDegs(),
      rs->getDistanceToFirstGateM(), gateWidthMeters,
      numRadials, numGates);
      
  out->getFloat2D()->fill(Constants::DataUnavailable);
  
  std::string pipelineConfig = myMode;
  if (myMode == "cressman" || myMode == "bilinear") {
      pipelineConfig += ":" + std::to_string(mySize) + ":" + std::to_string(mySize);
  }
  
  RadialSetMapper geoMapper(*rs, *out, myProjectGround);
  auto pipeline = ArrayPipeline::create(pipelineConfig);
  
  if (pipeline) {
      pipeline->remap(rs->getFloat2D(), out->getFloat2D(), geoMapper);
      writeOutputProduct(out->getTypeName(), out);
  } else {
      fLogSevere("Failed to create ArrayPipeline from config '{}'", pipelineConfig);
  }
}

void
Remap::processNewData(rapio::RAPIOData& d)
{
  fLogInfo("Data received: {}", d.getDescription());
  
  auto LLG = d.datatype<rapio::LatLonGrid>();
  if (LLG != nullptr) {
    remap(LLG);
  } else {
    auto R = d.datatype<rapio::RadialSet>();
    if (R != nullptr) {
      remap(R);
    } else {
      fLogSevere("Unsupported remap class, can't remap at moment.");
    }
  }
}

int
main(int argc, char * argv[])
{
  Remap alg = Remap();
  alg.executeFromArgs(argc, argv);
}
