#pragma once
// Precompiled header for rapio.
// If cmake is 3.16 or greater we can speed up the compile
// by using a precompiled header.
// Note:  We only want 3rd party here, not our private headers

// 1. Standard Library (No wrapping needed)
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

// 2. Formatting
#include <fmt/format.h>

// 3. Heavyweight Boost (Wrapped!)
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/multi_array.hpp>
#include <boost/optional.hpp>
#include <boost/any.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/asio.hpp>
BOOST_WRAP_POP

// (Optional) Other heavy C-libs
#include <zlib.h>
