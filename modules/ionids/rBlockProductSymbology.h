#pragma once

#include <rIONIDS.h>
#include <rBinaryIO.h>

namespace rapio {
/** Message Header Block */
class BlockProductSymbology : public NIDSBlock {
public:

  /** Read ourselves from buffer */
  virtual void
  read(StreamBuffer& s) override;

  /** Debug dump */
  void
  dump();

  // protected:
public: // temp for writing alpha

  short myBlockID;   ///<  Block ID
  int myBlockLength; ///< Block length
  short myNumLayers; ///< Number of layers we store
};
}
