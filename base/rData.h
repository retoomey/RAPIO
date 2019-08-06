#pragma once

#include <rRapio.h>

namespace rapio {
/** Base class for all objects that store data.
 *
 * The primary purpose of a Data subclass is to store
 * and interact with a single thing holding some data .
 *
 * This is more of an organizational empty class.  Makes
 * the doxygen graphs more organized.  Zero cost as long
 * as we don't stick a bunch of storage/functions in here.
 * Easier than moving things around namespaces
 *
 * @author Robert Toomey
 */
class Data : public Rapio { };
}
