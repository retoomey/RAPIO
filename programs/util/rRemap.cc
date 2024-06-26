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
  o.addSuboption("mode", "nearest", "Use nearest neighbor sampling.");
  o.addSuboption("mode", "cressman", "Use cressman sampling.");

  o.optional("size", "3", "Size of matrix.  For example, Cressman");
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
  bool nearest      = (myMode == "nearest");

  // Pixel centering stuff
  AngleDegs startLat = outg.getNWLat() - (outg.getLatSpacing() / 2.0); // move south (lat decreasing)
  AngleDegs startLon = outg.getNWLon() + (outg.getLonSpacing() / 2.0); // move east (lon increasing)
  AngleDegs atLat    = startLat;

  size_t counter  = 0;
  const int N     = mySize; // Cressman cell size
  const int halfN = N / 2;

  LogInfo("Using matrix size of " << N << " by " << N << "\n");

  for (size_t y = 0; y < numY; ++y, atLat -= outg.getLatSpacing()) {
    AngleDegs atLon = startLon;
    for (size_t x = 0; x < numX; ++x, atLon += outg.getLonSpacing()) {
      // Calculation of the X/Y index into the source LatLonGrid.
      // Note this if a float since int values are the true cell,
      // while the float is a 'delta' used by say cressman
      // Duplicates with the rDataProjection math.
      // FIXME: yo > orgNumLats should be yo > orgNumLats-1 I think
      const float yo = (orgNWLatDegs - atLat) / orgLatSpacingDegs;
      if ((yo < 0) || (yo >= orgNumLats)) { // not valid
        continue;                           // FIXME: Write unavailable?
        // Constants::DataUnavailable;
      }
      const float xo = (atLon - orgNWLonDegs ) / orgLonSpacingDegs;
      if ((xo < 0) || (xo >= orgNumLons)) {
        continue; // FIXME: Write unavailable?
        // Constants::DataUnavailable;
      }

      // In the loop for now though inefficient
      if (nearest) {
        // ---------------------------------------------------------
        //  Nearest neighbor...
        //

        // For nearest, just round down the values. Though
        // technically we would round up as well at .5.  We'll
        // play with this
        // Also this will pass on mask values such as Missing
        refOut[y][x] = refIn[yo][xo]; // roundx xo,yo
        counter++;
      } else {
        // ---------------------------------------------------------
        // Cressman experiment...
        // FIXME: clean up and put into API
        //

        // Sum up all the weights for each sample
        float tot_wt      = 0;
        float tot_val     = 0;
        float currentMask = Constants::DataUnavailable;
        int n = 0;

        for (int i = -halfN; i <= halfN; ++i) {
          const int yat = static_cast<int>(yo) + i;

          // If the lat is invalid, all the lons in the row will be invalid
          if (!((yat < 0) || (yat >= orgNumLats))) {
            // For the change in lon row...
            for (int j = -halfN; j <= halfN; ++j) {
              int xat = static_cast<int>(xo) + j;

              // ...if lon valid, check if a good value
              if (!((xat < 0) || (xat >= orgNumLons))) {
                float& val = refIn[yat][xat];

                // ...if the data value good, add weight to total...
                if (Constants::isGood(val)) {
                  const float lonDist = (xo - xat) * (xo - xat);
                  const float latDist = (yo - yat) * (yo - yat);
                  const float dist    = std::sqrt(latDist + lonDist);

                  counter++;
                  // If the distance is extremely small, use the cell exact value to avoid division by zero
                  // This also passes on mask when close to a true cell location
                  if (dist < std::numeric_limits<float>::epsilon()) {
                    refOut[y][x] = val;
                    goto getout;
                  }

                  float wt = 1.0 / dist;
                  tot_wt  += wt;
                  tot_val += wt * val;
                  ++n;
                } else {
                  // If any value in our matrix sampling is missing, we'll use that as a mask
                  // if there are no good values to interpolate.  Should work
                  if (val == Constants::MissingData) {
                    currentMask = Constants::MissingData;
                  }
                }
              } // Valid lon
            }   // End lon row
          } // Valid lat
        }   // End lat column

        if (n > 0) {
          refOut[y][x] = tot_val / tot_wt;
        } else {
          refOut[y][x] = currentMask;
        }
getout: ;

        //
        // ---------------------------------------------------------
      }
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
