#pragma once

#include <rRapio.h>

namespace rapio {
/** Base class for all objects that deal with
 * reading/writing data.
 *
 * The primary purpose of an IO subclass is do
 * reading and writing of any data, from multiple sources
 *
 * This is more of an organizational empty class.  Makes
 * the doxygen graphs more organized.  Zero cost as long
 * as we don't stick a bunch of storage/functions in here.
 * Easier than moving things around namespaces
 *
 * @author Robert Toomey
 */

class IO : public Rapio { };
}
