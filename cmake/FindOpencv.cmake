##################################################################################################
# Toomey OpenCV finder
#

INCLUDE(FindThirdParty)
find_third_party(
  Opencv
  HEADER opencv2/opencv.hpp
  LIBRARY opencv_core
  HEADER_PATHS ${Opencv_INCLUDEDIR} ${CUSTOM_THIRDDIR}/include /usr/include/opencv4 /usr/include
  LIBRARY_PATHS ${Opencv_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib
)

# The image processing routines (blur, morphology, etc.) live in opencv_imgproc.
# The generic finder above can only resolve a single library, so pull it in here.
if (Opencv_FOUND)
  find_library(Opencv_IMGPROC_LIBRARY
    NAMES opencv_imgproc
    PATHS ${Opencv_LIBRARYDIR} ${CUSTOM_THIRDDIR}/lib /usr/lib64 /usr/lib
  )
  if (Opencv_IMGPROC_LIBRARY)
    set(Opencv_LIBRARIES ${Opencv_LIBRARIES} ${Opencv_IMGPROC_LIBRARY})
  else()
    message(WARNING "Found OpenCV core but not opencv_imgproc. OpenCV image filters will fail to link.")
  endif()
endif()
