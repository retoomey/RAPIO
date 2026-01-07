addRAPIOModule(rapionids SHARED
  rIONIDS.cc
  rNIDSRadialSet.cc
  rNIDSUtil.cc
  # NIDS product table (required)
  rConfigNIDSInfo.cc

  # Blocks of Level III
  rBlockMessageHeader.cc
  rBlockProductDesc.cc
  rBlockProductSymbology.cc
  rBlockRadialSet.cc
)

addRAPIONote("      Found: NIDS Level III")
