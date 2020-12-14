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
class JSONNode : public Data {
  friend class JSONData;

protected:

  /** Stored in a node */
  boost::property_tree::ptree node;

public:
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
  addNode(const std::string& path, const JSONNode& child)
  {
    node.add_child(path, child.node);
  }

  /** Add as array item to the node */
  void
  addArrayNode(const JSONNode& child)
  {
    node.push_back(std::make_pair("", child.node));
  }

  /** Read child of node */
  JSONNode
  getChild(const std::string& path) const
  {
    JSONNode child;

    child.node = node.get_child(path);
    return child;
  }

  /** Read child of node */
  JSONNode
  getChildOptional(const std::string& path)
  {
    JSONNode child;
    auto thing = node.get_child_optional(path);

    if (thing != boost::none) {
      child.node = *thing;
    }
    return child;
  }

  /** BOOST is a dom so we just copy into our wrapper.
   * a SAX based XML would probably want pull iterator */
  std::vector<JSONNode>
  getChildren(const std::string& filter)
  {
    std::vector<JSONNode> list;
    for (auto r: node.get_child("")) {
      if (r.first == filter) {
        JSONNode child;
        child.node = r.second;
        list.push_back(child);
      }
    }
    return list;
  }
};

/** A wrapper to a JSON tree.
 * Currently using BOOST, though internally
 * we could add/swap rapidjson or another JSON
 * library under the hood if needed.  This probably
 * could happen, since BOOST doesn't seem to support
 * full types and there are debates over the speed.
 *
 * @author Robert Toomey
 * @see IOJSON
 */

class JSONData : public DataType {
public:

  /** Construct JSON tree */
  JSONData();

  /** Get root of document tree, valid only while XMLTree is. */
  JSONNode *
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

  /** JSON boost property tree */
  JSONNode myRoot;
};
}
