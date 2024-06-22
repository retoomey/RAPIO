#pragma once

#include <rTime.h>
#include <rGribDataType.h>
#include "rGribPointerHolder.h"

#include <vector>

extern "C" {
#include <grib2.h>
}

namespace rapio {
class GribMessageImp;
/** Field representation.  A wrapper for the gribfield.
 * Some fields overlap with GribMessage, I'm not sure if this is different data or if the
 * grib library is just copying the info from the message.  */
class GribFieldImp : public GribField {
public:
  /** Create an empty field */
  GribFieldImp(unsigned char * data, size_t messageNumber, size_t fieldNumber, std::weak_ptr<GribPointerHolder> valid) : GribField(messageNumber, fieldNumber),
    myBufferPtr(data), myGribField(nullptr), myUnpacked(false), myExpanded(false), myDataTypeValid(valid){ }

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
  ~GribFieldImp();

  /** Print this message and N fields.  Called by base operator << virtually */
  virtual std::ostream& print(std::ostream& os) override;

protected:

  /** Does this field need to reload before using? */
  bool
  needsToReload(bool unpacked = false, bool expanded = false) const;

  /** Load, reload or cache the field, return true if loaded and valid.
   * This is lazy called and modifies, but we treat it as const */
  bool
  fieldLoaded(bool unpacked = false, bool expanded = false);

  /** My buffer pointer which may point to myLocalBuffer, or basically a weak reference to a global buffer.
   * This requires what is handling the true buffer to stay in scope for us to remain valid.
   * This will be the start of the message (multiple fields could be sharing this) */
  unsigned char * myBufferPtr;

  /** Gribfield pointer if any */
  gribfield * myGribField;

  /** Is our grib pointer unpacked? (was g2_getfld called with unpack?) */
  bool myUnpacked;

  /** Is our grib pointer expanded? (was g2_getfld called with expand?) */
  bool myExpanded;

  /** Is my DataType still in scope, because we need it to be */
  std::weak_ptr<GribPointerHolder> myDataTypeValid;
};
}
