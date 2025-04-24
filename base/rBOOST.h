#pragma once

/** Boost header inclusion
 * There can be issues with boost macros or defines or
 * deprecation.  By having all our boost references in a
 * single location, this allows some control.
 */

// 'This' fun boost thing...global namespace for _1.
// It seems ptree still wants it.  We define it here
// to force it before any boost header is included.
// #define BOOST_BIND_NO_PLACEHOLDERS
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

// Suppress Clang warning for enum constexpr conversions
// This causes errors and breaks the build.  We don't have
// much control over boost internals
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wenum-constexpr-conversion"
#elif defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic push
#endif

// rArray
#include <boost/multi_array.hpp>

// rBoostConnection
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

// rBimap
#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/multiset_of.hpp>

// rBitset
#include <boost/dynamic_bitset.hpp>

// rDataArray
#include <boost/optional.hpp>

// rDataFilter
#include <boost/iostreams/filtering_stream.hpp>

#include <boost/iostreams/filter/gzip.hpp>  // .gz files
#include <boost/iostreams/filter/bzip2.hpp> // .bz2 files
#include <boost/iostreams/filter/zlib.hpp>  // .z files

#if HAVE_LZMA
# include <boost/iostreams/filter/lzma.hpp> // .lzma files
#endif

// rError
#include <boost/log/expressions.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/filesystem.hpp>
// On alpine at least, need this header to define BOOST_CURRENT_FUNCTION
#include <boost/current_function.hpp>

// rIOJSON
#include <boost/property_tree/json_parser.hpp>

// rIOXML
#include <boost/property_tree/xml_parser.hpp>

// rNamedAny
#include <boost/any.hpp>
#include <boost/optional.hpp>

// rOS
#include <boost/asio.hpp>
#include <boost/dll.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>
// #include <boost/date_time/posix_time/posix_time.hpp>

// rProcessTimer
#include <boost/timer/timer.hpp>

// rPTreeData
#include <boost/property_tree/ptree.hpp>

// rStrings
#include <boost/algorithm/string.hpp>

// rThreadGroup
#include <boost/thread.hpp>

#if defined(__clang__)
# pragma clang diagnostic pop
#elif defined(__GNUC__) && !defined(__clang__)
# pragma GCC diagnostic pop
#endif

// rDataGrid
#include <boost/multi_array.hpp>
#include <boost/variant.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>
