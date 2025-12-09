#pragma once

#include <rIONIDS.h>
#include <rBinaryIO.h>

namespace rapio {
/** Message Header Block */
class BlockMessageHeader : public NIDSBlock {
public:

  /** Read ourselves from buffer */
  virtual void
  read(StreamBuffer& s) override;

  /** Write ourselves to buffer */
  virtual void
  write(StreamBuffer& s) override;

  /** Debug dump */
  void
  dump();

  // protected:
public: // temp for writer

  /** NEXRAD message code */
  short int myMsgCode;

  /** Modified Julian date at time of transmission. Number of days
   *  since 1 Jan 1970 where 1 Jan. 1970 = 1
   */
  short int myMsgDate;

  /** Number of seconds after midnight in GMT */
  uint32_t myMsgTime;

  /** Number of bytes in message including header*/
  uint32_t myMsgLength;

  /** Source ID of message */
  short int myMsgSrcID;

  /** Destination ID of message */
  short int myMsgDstID;

  /** Header block + description blocks in msg */
  short int myNumBlocks;
};
}
