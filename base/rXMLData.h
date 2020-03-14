#pragma once

#include <rDataType.h>
#include <rTime.h>
#include <rLLH.h>

#include <boost/property_tree/ptree.hpp>

namespace rapio {
// NOTE: with BOOST at least, this code
// duplicates with XMLDataType.  Normally you'd
// make a common superclass, but I want to keep it
// separate in case we end up using separate XML/JSON
// libraries

/** Node in a tree, hides implementation */
class XMLNode : public Data {
public:

  /** Stored in a node */
  boost::property_tree::ptree node;

  /** Get data from node.  Uses the BOOST style . path  */
  template <class Type>
  Type
  get(const std::string& path, const Type & value) const
  {
    return (node.get(path, value));
  }

  /** Put data into node.  Uses the BOOST style . path  */
  template <typename Type>
  void
  put(const std::string& path, const Type & value)
  {
    node.put(path, value);
  }

  /** Add a child to the node */
  void
  addNode(const std::string& path, const XMLNode& child)
  {
    node.add_child(path, child.node);
  }

  /** Add as array item to the node */
  void
  addArrayNode(const XMLNode& child)
  {
    node.push_back(std::make_pair("", child.node));
  }

  /** Read child of node */
  XMLNode
  getChild(const std::string& path) const
  {
    XMLNode child;

    child.node = node.get_child(path);
    return child;
  }

  /** Read child of node */
  XMLNode
  getChildOptional(const std::string& path)
  {
    XMLNode child;
    auto thing = node.get_child_optional(path);

    if (thing != boost::none) {
      child.node = *thing;
    }
    return child;
  }

  /** BOOST is a dom so we just copy into our wrapper.
   * a SAX based XML would probably want pull iterator */
  std::vector<XMLNode>
  getChildren(const std::string& filter)
  {
    std::vector<XMLNode> list;
    for (auto r: node.get_child("")) {
      if (r.first == filter) {
        XMLNode child;
        child.node = r.second;
        list.push_back(child);
      }
    }
    return list;
  }
};

/** A wrapper to a XML tree.
 * Currently using BOOST, though internally
 * we could add/swap rapidjson or another XML
 * library under the hood if needed.  This probably
 * could happen, since BOOST doesn't seem to support
 * full types and there are debates over the speed.
 *
 * @author Robert Toomey
 * @see IOXML
 */

class XMLData : public DataType {
public:

  /** Construct XML tree */
  XMLData();

  // Need to rethink location/time data type stuff
  virtual LLH
  getLocation() const override;

  virtual Time
  getTime() const override;

  /** Get root of document tree, valid only while XMLTree is. */
  XMLNode *
  getTree()
  {
    return &myRoot;
  }

  /** Write property tree to URL */
  bool
  writeURL(
    const URL & path,
    bool      shouldIndent = true,
    bool      console = false);

  // These 'could' go to node for finer control

  /** Read from given buffer of text */
  bool
  readBuffer(std::vector<char>& buffer);

  /** Write text out to buffer, return size */
  size_t
  writeBuffer(std::vector<char>& buffer);

protected:

  /** XML boost property tree */
  XMLNode myRoot;
};
}
