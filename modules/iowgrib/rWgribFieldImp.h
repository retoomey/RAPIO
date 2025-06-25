#pragma once

#include <rTime.h>
#include <rGribDataType.h>

#include <vector>

extern "C" {
#include <grib2.h>
}

namespace rapio {
class WgribMessageImp;

/** Field representation.  A wrapper for the gribfield.
 * Some fields overlap with GribMessage, I'm not sure if this is different data or if the
 * grib library is just copying the info from the message.  */
class WgribFieldImp : public GribField {
public:
  /** Create an empty field */
  WgribFieldImp() : GribField(1, 1){ };

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
};
}
