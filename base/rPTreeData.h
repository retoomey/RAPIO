#pragma once

#include <rDataType.h>
#include <rTime.h>
#include <rLLH.h>

#include <boost/property_tree/ptree.hpp>

namespace rapio {
/** Property Tree holder class.  This is typically used
 * for XML, JSON, YAML, etc.. We wrap BOOST here currently*/
class PTreeNode : public Data {
  friend class PTreeData;
  friend class IOXML;
  friend class IOJSON;

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

  /** Simple get all attributes, if any as a string map */
  std::map<std::string, std::string>
  getAttrMap()
  {
    std::map<std::string, std::string> attrList;
    auto attrs = node.get_child_optional("<xmlattr>");
    if (attrs != boost::none) {
      for (auto& c: *attrs) {
        attrList[c.first] = c.second.get_value<std::string>();
      }
    }
    return attrList;
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
  addNode(const std::string& path, const PTreeNode& child)
  {
    node.add_child(path, child.node);
  }

  /** Add as array item to the node */
  void
  addArrayNode(const PTreeNode& child)
  {
    node.push_back(std::make_pair("", child.node));
  }

  /** Read child of node */
  PTreeNode
  getChild(const std::string& path) const
  {
    PTreeNode child;

    child.node = node.get_child(path);
    return child;
  }

  /** Read child of node */
  std::shared_ptr<PTreeNode>
  getChildOptional(const std::string& path)
  {
    // PTreeNode child;
    auto thing = node.get_child_optional(path);

    if (thing != boost::none) {
      std::shared_ptr<PTreeNode> child = std::make_shared<PTreeNode>();
      child->node = *thing;
      return child;
    }
    return nullptr;
  }

  /** Get the name of first real tag, used for factories based off
   * tag identification.  Ignore comments */
  std::string
  getFirstChildName()
  {
    for (auto r: node.get_child("")) {
      if (r.first == "<xmlcomment>") {
        continue;
      }
      return r.first;
    }
    return "";
  }

  /** BOOST is a dom so we just copy into our wrapper.
   * a SAX based XML would probably want pull iterator */
  std::vector<PTreeNode>
  getChildren(const std::string& filter)
  {
    std::vector<PTreeNode> list;
    for (auto r: node.get_child("")) {
      if (r.first == filter) {
        PTreeNode child;
        child.node = r.second;
        list.push_back(child);
      }
    }
    return list;
  }

  std::vector<std::shared_ptr<PTreeNode> >
  getChildrenPtr(const std::string& filter)
  {
    std::vector<std::shared_ptr<PTreeNode> > list;
    for (auto r: node.get_child("")) {
      if (r.first == filter) {
        // Copy to a shared_ptr
        std::shared_ptr<PTreeNode> child = std::make_shared<PTreeNode>();
        child->node = r.second;
        list.push_back(child);
      }
    }
    return list;
  }
};

/** A wrapper to a property tree.
 * Currently using BOOST, though internally
 * we could add/swap rapidjson or another tree
 * library under the hood if needed.  This probably
 * could happen, since BOOST doesn't seem to support
 * full types and there are debates over the speed.
 *
 * @author Robert Toomey
 * @see IOXML IOJSON
 */

class PTreeData : public DataType {
public:

  /** Construct property tree */
  PTreeData();

  /** Get root of document tree, valid only while PTree is. */
  std::shared_ptr<PTreeNode>
  getTree()
  {
    return myRoot;
  }

  /** Move this datatype contents to another. */
  void
  Move(std::shared_ptr<PTreeData> to)
  {
    LogSevere("MOVE IN PTREE CALLED\n");
    DataType::Move(to);
    to->myRoot = myRoot;
  }

protected:

  /** Root of the property tree */
  std::shared_ptr<PTreeNode> myRoot;
};
}
