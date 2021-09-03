include_directories(. ../)

# Need this to add the BOOST setting or the rError.cc will complain about v2
#add_definitions(-DBOOST_LOG_DYN_LINK)

add_library(rapiopython SHARED
  rIOPython.cc
)

#target_link_libraries(rapiopython
# 
#)

# Install the module
install(TARGETS rapiopython
   DESTINATION ${RAPIO_INSTALL_LIB}
)
