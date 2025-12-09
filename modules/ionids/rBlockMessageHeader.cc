#include "rBlockMessageHeader.h"
#include <rError.h>

using namespace rapio;

void
BlockMessageHeader::read(StreamBuffer& b)
{
  LogInfo("Reading Message Header block\n");

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
  LogInfo("Writing Message Header block\n");
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
  LogInfo("code " << myMsgCode << "\n");
  LogInfo("Julian " << myMsgDate << "\n");             // Julian Date
  LogInfo("Time seconds " << myMsgTime << "\n");       // Time of Messages (seconds)
  LogInfo("Length " << myMsgLength << "\n");           // Length of Message
  LogInfo("Source " << myMsgSrcID << "\n");            // Source (originators) ID of the sender
  LogInfo("Destination " << myMsgDstID << "\n");       // Destination ID (receivers) for message transmission
  LogInfo("Number of blocks " << myNumBlocks << "\n"); // Number of Blocks
}
