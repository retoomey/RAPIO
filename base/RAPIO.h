#pragma once

// Main headers for making algorithm
#include <rRAPIOAlgorithm.h>
#include <rProcessTimer.h>

// Allow access to direct readers?
// Thinking gdal's way of passing a key
// #include <rIOGrib.h>
#include <rIOJSON.h>

// Allow access to standard datatypes
#include <rGribDataType.h>
#include <rNetcdfDataType.h>
#include <rRadialSet.h>
#include <rLatLonGrid.h>
#include <rJSONData.h>
#include <rXMLData.h>

// Experimental AWS calls..
#include <rAWS.h>
#include <rProject.h>
// #include <rCurlConnection.h>
