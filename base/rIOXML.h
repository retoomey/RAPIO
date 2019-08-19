#pragma once

#include <rIO.h>
#include <rData.h>
#include <iosfwd>
#include <string>
#include <map>
#include <vector>
#include <stack>
#include <memory>

namespace rapio {
class Buffer;
class URL;

typedef std::map<std::string, std::string> AttributeMap;

/**
 *  An XMLElement.
 */
class XMLElement : public Data {
public:

  typedef std::vector<std::shared_ptr<XMLElement> > children_t;

  XMLElement(const std::string& tagName_in) : tagName(tagName_in){ }

  /** For STL convenience.
   *  You have to call setTagName for this to be a valid element. */
  XMLElement(){ }

  /**
   *  Change the  text value between an element's start and end
   *  tags. It is recommended that you avoid mixed
   *  content (having both children and text) as
   *  all embedded elements will be moved to the front.
   *
   *  Entities in text_str will be replaced when writing out.
   */
  void
  setText(const std::string& text_str)
  {
    text = text_str;
  }

  /**
   * Append text to the existing value.
   * @see setText
   */
  void
  appendText(const std::string& text_str)
  {
    text += text_str;
  }

  /**
   * Get the  text value between an element's start and end
   * tags. It is recommended that you avoid mixed
   * content (having both children and text) as
   * all embedded elements will be moved to the front.
   *
   * Entities have already been replaced.
   */
  const std::string&
  getText() const
  {
    return (text);
  }

  /**
   *  A name-value mapping for the attributes.
   *  The value string has no entities.
   */
  const AttributeMap&
  getAttributes() const
  {
    return (attributes);
  }

  bool
  hasAttribute(const std::string& name) const
  {
    return (attributes.count(name) != 0);
  }

  /** entitites in the value string will be replaced on write. */
  void
  setAttribute(const std::string& name, const std::string& value)
  {
    attributes[name] = value;
  }

  void
  setAttribute(const std::string& name,
    size_t                      value);
  void
  setAttribute(const std::string& name,
    int                         value);
  void
  setAttribute(const std::string& name,
    double                      value);

  /** Preferred way to use the get attribute method */
  const std::string&
  getAttributeValue(const std::string& name,
    const std::string                & fallback) const
  {
    AttributeMap::const_iterator iter = attributes.find(name);
    return (iter == attributes.end() ? fallback : iter->second);
  }

  /**
   *  get the attribute value. @return empty string if the attribute
   *  was not specified. @see getAttributeValue for a better interface.
   */
  const std::string&
  getAttribute(const std::string& name) const;

  /**
   *  get the attribute value. @return empty string if the attribute
   *  was not specified. @see getAttributeValue for a better interface.
   */
  const std::string&
  attribute(const std::string& name) const
  {
    return (getAttribute(name));
  }

  /** high level convenience methods for standard types that get a
   *  converted value and check existance at same time. @return true
   *  only if attribute found and value successfully changed to it.
   *  @see getAttributeValue for dealing with general types.
   */
  bool
  readValidAttribute(const std::string& attribute,
    char                              & value) const;
  bool
  readValidAttribute(const std::string& attribute,
    int                               & value) const;
  bool
  readValidAttribute(const std::string& attribute,
    float                             & value) const;
  bool
  readValidAttribute(const std::string& attribute,
    double                            & value) const;
  bool
  readValidAttribute(const std::string& attribute,
    bool                              & value) const;
  bool
  readValidAttribute(const std::string& attribute,
    std::string                       & value) const;

  const std::string&
  getTagName() const
  {
    return (tagName);
  }

  void
  setTagName(const std::string& tagName_in)
  {
    tagName = tagName_in;
  }

  /**
   * fills in pointers to immediate children
   * that bear this tag name (unlike the DOM method getElementsByTagName
   * this does not recurse and is therefore, pretty fast.)
   * @return number of children matching tagName
   */
  size_t
  getChildren(const std::string& tagName,
    children_t *               result) const;

  /** returns the first child element. To get the text, use
   *  getText */
  const XMLElement&
  getFirstChild() const
  {
    return (*children.front());
  }

  /** returns the first child element that matches the
   * tag name given.  This replaces Qt's namedNode function */
  std::shared_ptr<XMLElement>
  getFirstNamedChild(const std::string& name) const;

  const children_t      &
  getChildren() const
  {
    return (children);
  }

  void
  addChild(std::shared_ptr<XMLElement> e)
  {
    children.push_back(e);
  }

  void
  appendChild(const XMLElement& e)
  {
    std::shared_ptr<XMLElement> newOne = std::make_shared<XMLElement>(e);
    children.push_back(newOne);
  }

protected:

  AttributeMap attributes;

  /** Child elements. Only elements -- no comments, etc. */
  children_t children;

  /**
   * text value between an element's start and end
   * tags. It is recommended that you avoid mixed
   * content (having both children and text) as
   * all embedded elements will be moved to the front.
   *
   * This is the true text; entities will have to be substituted
   * when writing.
   */
  std::string text;

  std::string tagName;
};

/**
 *  An XML Document is an XMLElement with extra information.
 *  Well it could have extra information later
 */
class XMLDocument : public XMLElement {
public:

  const XMLElement&
  getRootElement() const
  {
    return (*children.front());
  }

  /** Create a blank document */
  XMLDocument()
  { }

  XMLDocument(std::shared_ptr<XMLElement>& e)
  {
    addChild(e);
  }
};

class XMLReader : public IO {
public:

  static std::shared_ptr<XMLDocument>
  parse(std::istream& is);
  static std::shared_ptr<XMLDocument>
  parse(Buffer& b);

protected:

  XMLReader(std::istream& input) : is(&input){ }

  bool
  parse();
  bool
  handleComment(const std::string& chars);
  bool
  handleDTDEntity(const std::string& chars);
  bool
  handleProcessingInstruction(const std::string& chars);
  bool
  handleEndElement(const std::string& chars);
  bool
  handleEmptyElement(const std::string& chars);
  bool
  handleStartElement(const std::string& chars);
  bool
  createElement(const std::string& startTag);
  bool
  createAttributes(const std::string & attrStr,
    std::string::size_type           pos);
  const std::string&
  getCurrentTag() const;

  std::istream * is;
  std::stack<std::shared_ptr<XMLElement> > parseStack;
  std::shared_ptr<XMLDocument> doc;
};

/** Simple routines for reading/writing XML documents */
class IOXML : public IO {
public:

  /** returns "a<b" for "a&lt;b" */
  static std::string
  decodeXML(const std::string& input);

  /** returns "a&lt;b" for "a<b" */
  static std::string
  encodeXML(const std::string& input);

public:

  // Writing
  // --------------------------------------------------------------------
  static void
  write(std::ostream & os,
    const XMLDocument& doc,
    bool             shouldIndent = true);

  /** the depth is the number of spaces
   * to indent this element. */
  static void
  write(std::ostream & os,
    const XMLElement & e,
    int              depth,
    bool             shouldIndent = true);

  // Reading
  // --------------------------------------------------------------------

  /**
   * Returns an XML::Document created by passing the contents of `url' to
   * XML::Reader::parse().
   */
  static std::shared_ptr<XMLDocument>
  readXMLDocument(const URL& url);

  /* Read from a stream */
  static std::shared_ptr<XMLDocument>
  readXMLDocument(std::istream& is);
};

/** list of elements such as that filled
 * in by Element::getChildren */
typedef XMLElement::children_t XMLElementList;
}
