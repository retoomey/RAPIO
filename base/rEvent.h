#pragma once

#include <rRapio.h>

namespace rapio {
/** Base class for all objects that deal with
 * callbacks, listeners, sources, etc.
 *
 * The primary purpose of an Event subclass is do
 * callbacks and handle events, etc.
 *
 * This is more of an organizational empty class.  Makes
 * the doxygen graphs more organized.  Zero cost as long
 * as we don't stick a bunch of storage/functions in here.
 * Easier than moving things around namespaces
 *
 * @author Robert Toomey
 */

class Event : public Rapio { };
}
