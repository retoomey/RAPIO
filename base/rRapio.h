#pragma once

#include <iosfwd>

//
// Doxygen groups
// Using these will allow us to 'group' in doxygen without
// using old Java style God classes.  This way is better
// because it speeds compilation time.
//

/**
 * @defgroup rapio
 * @brief rapio core engine
 * @{
 */

/**
 * @defgroup rapio_data
 * @brief Used for simple data storage.
 */

/**
 * @defgroup rapio_io
 * @brief Used for input/output of data.
 */

/**
 * @defgroup rapio_datatype
 * @brief Used for more advanced/collected data storage that is typically IO from storage.
 */

/**
 * @defgroup rapio_utility
 * @brief Used as a utility.
 */

/**
 * @defgroup rapio_algorithm
 * @brief Used for controlling the running algorithm.
 */

/**
 * @defgroup rapio_event
 * @brief Used for callbacks, listeners, sources, etc.
 */

/**
 * @defgroup rapio_image
 * @brief Used for array/image processing effects.
 */

/** @} */ // End of RAPIO

#if 0
namespace rapio {
class Rapio { };
}
#endif
