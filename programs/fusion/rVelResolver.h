#pragma once

#include "rVolumeValueResolver.h"
#include "rRAPIOAlgorithm.h"

namespace rapio {
/** Grid cell output storage for our resolver */
class VolumeValueVelGatherer : public VolumeValue
{
public:
  /* Set inline function to make sure all fields are set */
  inline void
  set(float v, float ux_, float uy_, float uz_, float latDegs_, float lonDegs_, float heightKMs_)
  {
    dataValue = v;
    ux        = ux_;
    uy        = uy_;
    uz        = uz_;
    latDegs   = latDegs_;
    lonDegs   = lonDegs_;
    heightKMs = heightKMs_;
  }

  float ux;
  float uy;
  float uz;
  float latDegs;
  float lonDegs;
  float heightKMs;
};

/** Data handler for stage1 to stage2 */
class VelVolumeValueIO : public VolumeValueIO {
public:

  /** Empty creation for stl */
  VelVolumeValueIO(){ }

  // -----------------------------------------------------------
  // Stage one adding and sending or 'finalizing' values...
  // FIXME: Currently three methods for sending...I'm leaning towards
  // the design hiding everything in the resolver to be a
  // 'multi radar' algorithm

  /** Initialize a volume value IO for sending data.  Typically called by stage1 to
   * prepare to send the grid resolved values */
  virtual bool
  initForSend(
    const std::string   & radarName,
    const std::string   & typeName,
    const std::string   & units,
    const LLH           & center,
    size_t              xBase,
    size_t              yBase,
    std::vector<size_t> dims
  ) override
  {
    mySourceName = radarName;
    myUnits      = units;
    myLocation   = center;
    // We'll clear for now..though it should be unique per pass
    // Would be nice to do a std::vector<VolumeValueVelGatherer>, but
    // we need each field separate for netcdf/table columns.
    myValues.clear();
    myUXs.clear();
    myUYs.clear();
    myUZs.clear();
    myLatDegs.clear();
    myLonDegs.clear();
    myHeightKMs.clear();
    return true;
  }

  /** Add data to us for sending to stage2, only used by stage one.
   * FIXME: Debating the separate VolumeValue output vs directly storing,
   * since doing things this way requires a copy.  But if I want to ever
   * 'change out' a stage2 to say using an AWS S3 bucket or something,
   * having it logically separated is cleaner vs having the resolver
   * directly store the values.*/
  virtual void
  add(VolumeValue * vvp, short x, short y, short z) override
  {
    // Copy grid cell output init final vectors for output...
    auto& vv = *(VolumeValueVelGatherer *) (vvp);

    // Ok, ignore missing or unavailable values for the table,
    // those are just for the 2D grid to visualize stuff...
    if (Constants::isGood(vv.dataValue)) {
      myValues.push_back(vv.dataValue);
      myUXs.push_back(vv.ux);
      myUYs.push_back(vv.uy);
      myUZs.push_back(vv.uz);
      myLatDegs.push_back(vv.latDegs);
      myLonDegs.push_back(vv.lonDegs);
      myHeightKMs.push_back(vv.heightKMs);
    }
  }

  /** Send/write stage2 data.  Give an algorithm pointer so we call do alg things if needed. */
  virtual void
  send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName) override
  {
    // At moment just send a netcdf file always
    if (alg->isProductWanted("S2Netcdf")) {
      const size_t finalSize = myValues.size();

      auto stage2 =
        DataGrid::Create(asName, myUnits, myLocation, aTime, { finalSize }, { "Values" });

      // This is general settings for pathing...
      stage2->setString("Sourcename", mySourceName);
      stage2->setDataType("PointTable"); // DataType attribute in the file not filename
      stage2->setSubType("S2");

      // Values to netcdf arrays
      auto& values = stage2->addFloat1DRef(Constants::PrimaryDataName, myUnits, { 0 });
      auto& ux     = stage2->addShort1DRef("ux", "dimensionless", { 0 });
      auto& uy     = stage2->addShort1DRef("uy", "dimensionless", { 0 });
      auto& uz     = stage2->addShort1DRef("uz", "dimensionless", { 0 });
      auto& lat    = stage2->addShort1DRef("latitude", "Degrees", { 0 });
      auto& lon    = stage2->addShort1DRef("longitude", "Degrees", { 0 });
      auto& ht     = stage2->addShort1DRef("height", "KM", { 0 });

      // Copy into netcdf output
      std::copy(myValues.begin(), myValues.end(), values.data());
      std::copy(myUXs.begin(), myUXs.end(), ux.data());
      std::copy(myUYs.begin(), myUYs.end(), uy.data());
      std::copy(myUZs.begin(), myUZs.end(), uz.data());
      std::copy(myLatDegs.begin(), myLatDegs.end(), lat.data());
      std::copy(myLonDegs.begin(), myLonDegs.end(), lon.data());
      std::copy(myHeightKMs.begin(), myHeightKMs.end(), ht.data());

      std::map<std::string, std::string> extraParams;
      extraParams["showfilesize"] = "yes";
      extraParams["compression"]  = "gz";
      stage2->setTypeName(asName); // should already be set
      alg->writeOutputProduct("S2Netcdf", stage2, extraParams);
    }

    if (alg->isProductWanted("S2")) {
      LogSevere("Can't write raw S2.  Use S2Netcdf option to write netcdf.\n");
    }
  } // send

private:
  /** Name of source */
  std::string mySourceName;
  /** Location of source */
  LLH myLocation;
  /** Units passed in from source */
  std::string myUnits;

  // Output
  std::vector<float> myValues;
  std::vector<float> myUXs;
  std::vector<float> myUYs;
  std::vector<float> myUZs;
  std::vector<float> myLatDegs;
  std::vector<float> myLonDegs;
  std::vector<float> myHeightKMs;
};

/**
 * A resolver attempting to gather data for a velocity collection
 * attempt.
 *
 * @author Robert Toomey
 */
class VelResolver : public VolumeValueResolver
{
public:

  // ---------------------------------------------------
  // Functions for declaring available to command line
  //

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf();

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override;

  // ---------------------------------------------------
  // Functions for declaring how to create/process data
  // for a stage2
  //

  /** We're going to use our custom gatherer */
  virtual std::shared_ptr<VolumeValue>
  getVolumeValue() override
  {
    return std::make_shared<VolumeValueVelGatherer>();
  }

  /** Return the VolumeValueOutputter class used for this resolver.  This allows resolvers to
   * write/read final output as they want. */
  virtual std::shared_ptr<VolumeValueIO>
  getVolumeValueIO() override
  {
    return std::make_shared<VelVolumeValueIO>();
  }

  /** Calculate using VolumeValue inputs, our set of outputs in VolumeValue.*/
  virtual void
  calc(VolumeValue * vvp) override;
};
}
