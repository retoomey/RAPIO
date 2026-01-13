#include "rBlockMessageHeader.h"
#include <rError.h>

using namespace rapio;

void
BlockMessageHeader::read(StreamBuffer& b)
{
  fLogInfo("Reading Message Header block");

  // Header doesn't have divider since it's the first block
  // checkDivider(b);

  myMsgCode   = b.readShort();
  myMsgDate   = b.readShort();
  myMsgTime   = b.readInt();
  myMsgLength = b.readInt();
  myMsgSrcID  = b.readShort();
  myMsgDstID  = b.readShort();
  myNumBlocks = b.readShort();
}

void
BlockMessageHeader::write(StreamBuffer& b)
{
  fLogInfo("Writing Message Header block");
  // Header doesn't have divider since it's the first block
  // b.writeDivider();

  b.writeShort(myMsgCode);
  b.writeShort(myMsgDate);
  b.writeInt(myMsgTime);
  b.writeInt(myMsgLength);
  b.writeShort(myMsgSrcID);
  b.writeShort(myMsgDstID);
  b.writeShort(myNumBlocks);
}

void
BlockMessageHeader::dump()
{
  fLogInfo("code {}", myMsgCode);
  fLogInfo("Julian {}", myMsgDate);             // Julian Date
  fLogInfo("Time seconds {}", myMsgTime);       // Time of Messages (seconds)
  fLogInfo("Length {}", myMsgLength);           // Length of Message
  fLogInfo("Source {}", myMsgSrcID);            // Source (originators) ID of the sender
  fLogInfo("Destination {}", myMsgDstID);       // Destination ID (receivers) for message transmission
  fLogInfo("Number of blocks {}", myNumBlocks); // Number of Blocks
}
