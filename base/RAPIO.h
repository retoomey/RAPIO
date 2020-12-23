#pragma once

// Main headers for making algorithm
#include <rRAPIOAlgorithm.h>
#include <rProcessTimer.h>

// Allow access to direct readers?
// Thinking gdal's way of passing a key
// Since modules are dynamically loaded, we'll
// probably need to pass xml/json and pointers
// generically through the main algorithm
// Or possibly I make the IO* classes stubs
// #include <rIOGrib.h>
#include <rIOJSON.h>

// Module loaded types (these are stubs)
// FIXME: API not 100% final here, goal is
// dynamic ondemand usage while keeping
// interface simple
#include <rGribDataType.h>
#include <rNetcdfDataType.h>
#include <rImageDataType.h>

// Allow access to standard datatypes
#include <rRadialSet.h>
#include <rLatLonGrid.h>
#include <rJSONData.h>
#include <rXMLData.h>

// Experimental AWS calls..
#include <rAWS.h>
#include <rProject.h>
// #include <rCurlConnection.h>
#include <rArray.h>
