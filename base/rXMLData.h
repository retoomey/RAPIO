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
  friend class XMLData;

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

  /** Get tag from node. */
  template <class Type>
  Type
  get(const Type & value) const
  {
    return (node.get_value<Type>());
  }

  /** Get attr from node, or default if missing */
  template <class Type>
  Type
  getAttr(const std::string& attr, const Type & value) const
  {
    auto attrs = node.get_child_optional("<xmlattr>");

    if (attrs != boost::none) {
      return (attrs->get(attr, value));
      // return (*attrs.get(attr, value));
    }

    return value;
    // return (node.get("<xmlattr>", value));
  }

  /** Put data into node.  Uses the BOOST style . path  */
  template <typename Type>
  void
  put(const std::string& path, const Type & value)
  {
    node.put(path, value);
  }

  /** Put data into attributes.*/
  template <typename Type>
  void
  putAttr(const std::string& name, const Type & value)
  {
    node.put("<xmlattr>." + name, value);
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
  std::shared_ptr<XMLNode>
  getChildOptional(const std::string& path)
  {
    // XMLNode child;
    auto thing = node.get_child_optional(path);

    if (thing != boost::none) {
      std::shared_ptr<XMLNode> child = std::make_shared<XMLNode>();
      child->node = *thing;
      return child;
    }
    return nullptr;
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

  std::vector<std::shared_ptr<XMLNode> >
  getChildrenPtr(const std::string& filter)
  {
    std::vector<std::shared_ptr<XMLNode> > list;
    for (auto r: node.get_child("")) {
      if (r.first == filter) {
        // Copy to a shared_ptr
        std::shared_ptr<XMLNode> child = std::make_shared<XMLNode>();
        child->node = r.second;
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
