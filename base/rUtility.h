#pragma once

#include <rRapio.h>

namespace rapio {
/** Base class for all utility objects.
 *
 * The primary purpose of a utility subclass is
 * to provide static utility functions for some purpose
 *
 * This is more of an organizational empty class.  Makes
 * the doxygen graphs more organized.  Zero cost as long
 * as we don't stick a bunch of storage/functions in here.
 * Easier than moving things around namespaces
 *
 * @author Robert Toomey
 */

class Utility : public Rapio { };
}
