#pragma once

#include <rTime.h>
#include <rGribDataType.h>

#include <vector>

namespace rapio {
class WgribMessageImp;

/** Field representation */
class WgribFieldImp : public GribField {
public:

  /** Create an empty field.  Field takes same as message, since in the wgrib2 model, the 'message' is just the first field */
  WgribFieldImp(const URL& url, int messageNumber, int fieldNumber, long int filepos, std::array<long, 3>& sec0,
    std::array<long, 13>& sec1);

  // Array methods (assuming grid data)

  /** Get a float 2D array in data projection, assuming we're grid data */
  virtual std::shared_ptr<Array<float, 2> >
  getFloat2D() override;

  /** Append/create a float 3D at a given level.  Used for 3D array ability */
  virtual std::shared_ptr<Array<float, 3> >
  getFloat3D(std::shared_ptr<Array<float, 3> > in, size_t at, size_t max) override;

  // Metadata methods (FIXME: add more as needed)

  /** GRIB Edition Number (currently 2) 'gfld->version' */
  virtual long
  getGRIBEditionNumber() override;

  /** Discipline-GRIB Master Table Number (see Code Table 0.0) 'gfld->discipline' */
  virtual long
  getDisciplineNumber() override;

  /** Get the Significance of Reference Time (see Code Table 1.2) 'gfld->idsect[4]' */
  virtual size_t
  getSigOfRefTime() override;

  /** Get the time stored in this field. (usually just matches message time) 'glfd->idsect[5-10]' */
  virtual Time
  getTime() override;

  /** Return the Grid Definition Template Number (Code Table 3.1) */
  virtual size_t
  getGridDefTemplateNumber() override;

  // GribDatabase methods

  /** Get product name from database */
  virtual std::string
  getProductName() override;

  /** Get product name from database */
  virtual std::string
  getLevelName() override;

  /** Free our internal grib pointer if we have one */
  ~WgribFieldImp();

  /** Print this message and N fields.  Called by base operator << virtually */
  virtual std::ostream&
  print(std::ostream& os) override;

protected:

  /** Store the URL passed to use for filename */
  URL myURL;

  // FIXME: with the iogrib module we used gribfield not the sections,
  // so it's a bit messy here.

  /** [Indicator, Discipline, GRIB Version] */
  std::array<long, 3> mySection0;

  /** [Section1 metadata] */
  std::array<long, 13> mySection1;
};
}
