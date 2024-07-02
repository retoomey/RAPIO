#include <rRemap.h>

#include <iostream>

using namespace rapio;

void
Remap::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "Remap is used to remap LatLonArea based classes to another grid specification.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid("nw(37, -100) se(30.5, -93) h(0.5,0.5,1) s(0.01, 0.01)");

  // Mode choice of remap technique...
  // FIXME: For cressmen we 'might' want a N parameter for size of it.
  o.optional("mode", "nearest", "Convert mode for remapping, such as nearest neighbor.");
  o.addSuboption("mode", "nearest", "Use nearest neighbor sampling (size is 1).");
  o.addSuboption("mode", "cressman", "Use cressman sampling.");
  o.addSuboption("mode", "bilinear", "Use bilinear sampling.");

  o.optional("size", "3", "Size of matrix if not nearest.  For example, Cressman and bilinear");
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
}

void
Remap::remap(std::shared_ptr<LatLonGrid> llg)
{
  LogInfo("Remapping an incoming LatLonGrid to new grid...\n");

  // ----------------------------------------------------------------
  // Make a new LatLonGrid for output.
  //
  // FIXME: Need more create API...this is messy
  // FIXME: We could reuse this LLG/cache each call (if this is being
  // used in real time)
  // I'll use this alpha code once working to improve the API
  LLCoverageArea outg  = myFullGrid;
  std::string typeName = llg->getTypeName();
  std::string units    = llg->getUnits();
  Time time = llg->getTime();
  std::shared_ptr<LatLonGrid> out;

  out = LatLonGrid::Create(typeName, units,
      LLH(myFullGrid.getNWLat(), myFullGrid.getNWLon(), 0), time,
      myFullGrid.getLatSpacing(), myFullGrid.getLonSpacing(),
      myFullGrid.getNumY(), myFullGrid.getNumX());

  // Fill unavailable in the new grid
  auto fullgrid = out->getFloat2D();

  fullgrid->fill(Constants::DataUnavailable);

  // FIXME: Maybe we should have a getNWLat, etc.
  LLH newCorner = llg->getTopLeftLocationAt(0, 0); // not centered
  AngleDegs orgNWLatDegs      = newCorner.getLatitudeDeg();
  AngleDegs orgNWLonDegs      = newCorner.getLongitudeDeg();
  AngleDegs orgLatSpacingDegs = llg->getLatSpacing();
  AngleDegs orgLonSpacingDegs = llg->getLonSpacing();
  size_t orgNumLats = llg->getNumLats();
  size_t orgNumLons = llg->getNumLons();

  // ----------------------------------------------------------------
  // Project from new to old and handle value
  //
  // We're only dealing with the primary data array.  Multi raster
  // we'd need more work, right?  We'd have to add flags to specify the
  // fields to handle then in some way.
  auto& refOut = out->getFloat2DRef();
  auto& refIn  = llg->getFloat2DRef();

  // ----------------------------------------------------------------
  // Alpha: Remap attempt.
  //
  // Basically we have to march though the lat/lon of the new grid
  // and use that to calculate the indexes into the old grid.  It's similar
  // to projection and fusion marching.  Feel like this could be made an iterator
  const size_t numY = outg.getNumY();
  const size_t numX = outg.getNumX();

  // ----------------------------------------------------------------
  // Make the remapper wanted
  //
  std::shared_ptr<ArrayAlgorithm> Remap;

  if (myMode == "nearest") {
    Remap = std::make_shared<ArrayAlgorithm>(NearestNeighbor(llg->getFloat2D(), out->getFloat2D()));
  } else if (myMode == "cressman") {
    Remap = std::make_shared<ArrayAlgorithm>(Cressman(llg->getFloat2D(), out->getFloat2D()));
  } else if (myMode == "bilinear") {
    Remap = std::make_shared<ArrayAlgorithm>(Bilinear(llg->getFloat2D(), out->getFloat2D()));
  } else {
    LogSevere("Can't create remapper '" << myMode << "'\n");
    exit(1);
  }
  LogInfo("Created Array Algorithm '" << myMode << "' to process LatLonGrid primary array\n");
  auto& r = *Remap;

  // Pixel centering stuff
  AngleDegs startLat = outg.getNWLat() - (outg.getLatSpacing() / 2.0); // move south (lat decreasing)
  AngleDegs startLon = outg.getNWLon() + (outg.getLonSpacing() / 2.0); // move east (lon increasing)
  AngleDegs atLat    = startLat;

  size_t counter = 0;

  // FIXME: Could we do different directions?  N by N2?  Why not?
  LogInfo("Using matrix size of " << mySize << " by " << mySize << "\n");

  // Cell hits yof and xof
  // Note the cell is allowed to be fractional and out of range,
  // since we're doing a matrix 'some' cells might be in the range

  for (size_t y = 0; y < numY; ++y, atLat -= outg.getLatSpacing()) {
    const float yof = (orgNWLatDegs - atLat) / orgLatSpacingDegs;

    AngleDegs atLon = startLon;
    for (size_t x = 0; x < numX; ++x, atLon += outg.getLonSpacing()) {
      const float xof = (atLon - orgNWLonDegs ) / orgLonSpacingDegs;
      if (!r.remap(yof, xof, y, x)) {
        continue;
      }
      counter++;
    } // endX
  }   // endY

  if (counter > 0) {
    LogInfo("Good sample hit counter is " << counter << "\n");
  }
  // ----------------------------------------------------------------
  // Write the new output
  //
  std::map<std::string, std::string> myOverrides;

  writeOutputProduct(out->getTypeName(), out, myOverrides); // Typename will be replaced by -O filters
} // Remap::remap

void
Remap::processNewData(rapio::RAPIOData& d)
{
  LogInfo("Data received: " << d.getDescription() << "\n");

  // We only handle the area classes
  auto data = d.datatype<rapio::LatLonArea>();

  if (data != nullptr) {
    // Reproject LLG
    auto LLG = d.datatype<rapio::LatLonGrid>();
    if (LLG != nullptr) {
      remap(LLG);
    }
  } else {
    LogSevere("Not a LatLonArea class, can't remap at moment.\n");
  }
} // Remap::processNewData

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  Remap alg = Remap();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
