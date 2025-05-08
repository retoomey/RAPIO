#pragma once

#include <iosfwd>

namespace rapio {
/** Base class for all rapio objects.
 * Like Java, making everything subclass 'object' basically.
 *
 * Should be zero cost as long as we don't place
 * any virtual functions, etc. in here.
 *
 * I might remove these 'organizational' classes later
 * and just use namespaces or something else later,
 * if someone shows me a cooler trick.  This allows me
 * to create a nice full tree graph with doxygen for
 * displaying etc.  Pure c++ programmers will probably
 * chastise me on it, lol.
 *
 * @author Robert Toomey
 */

class Rapio { };
}
