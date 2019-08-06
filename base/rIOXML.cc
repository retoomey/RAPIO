#include "rIOXML.h"

#include "rStrings.h"
#include "rBuffer.h"
#include "rError.h"
#include "rIOURL.h"
#include "rProcessTimer.h"
#include "rConfig.h"

#include <algorithm> // remove_if()
#include <limits>    // numeric_limits
#include <cstdio>
#include <cstdlib> // strtol()
#include <cassert>

using namespace rapio;

void
XMLElement::setAttribute(const std::string& name,
  int                                     val)
{
  char value[64];

  snprintf(value, sizeof(value), "%d", val);
  setAttribute(name, value);
}

void
XMLElement::setAttribute(const std::string& name,
  size_t                                  val)
{
  char value[64];

  snprintf(value, sizeof(value), "%lu", (unsigned long) val);
  setAttribute(name, value);
}

void
XMLElement::setAttribute(const std::string& name,
  double                                  val)
{
  char value[64];

  snprintf(value, sizeof(value), "%f", val);
  setAttribute(name, value);
}

size_t
XMLElement::getChildren(const std::string& name, children_t * result) const
{
  assert(result != nullptr);
  const size_t oldsize(result->size());

  for (auto it:children) {
    if (it->getTagName() == name) {
      result->push_back(it);
    }
  }
  return (result->size() - oldsize);
}

std::shared_ptr<XMLElement>
XMLElement::getFirstNamedChild(const std::string& name) const
{
  for (auto it:children) {
    if (it->getTagName() == name) {
      return (it);
    }
  }
  return (0);
}

const std::string&
XMLElement::getAttribute(const std::string& name) const
{
  static const std::string empty;

  return (getAttributeValue(name, empty));
}

bool
XMLElement::readValidAttribute(const std::string& attribute,
  char                                          & value) const
{
  const std::string& attr(getAttribute(attribute));

  if (!attr.empty()) { value = attr[0]; return (true); }
  return (false);
}

bool
XMLElement::readValidAttribute(const std::string& attribute,
  int                                           & value) const
{
  const std::string& attr(getAttribute(attribute));

  if (!attr.empty()) { value = strtol(attr.c_str(), NULL, 0); return (true); }
  return (false);
}

bool
XMLElement::readValidAttribute(const std::string& attribute,
  float                                         & value) const
{
  const std::string& attr(getAttribute(attribute));

  if (!attr.empty()) { value = atof(attr.c_str()); return (true); }
  return (false);
}

bool
XMLElement::readValidAttribute(const std::string& attribute,
  double                                        & value) const
{
  const std::string& attr(getAttribute(attribute));

  if (!attr.empty()) { value = atof(attr.c_str()); return (true); }
  return (false);
}

bool
XMLElement::readValidAttribute(const std::string& attribute,
  bool                                          & value) const
{
  if (!hasAttribute(attribute)) { return false; }

  const std::string& attr(getAttribute(attribute));

  if (!attr.empty()) { value = attr == "true" || attr == "1" || attr[0] == 'y'; }
  return true;
}

bool
XMLElement::readValidAttribute(const std::string& attribute,
  std::string                                   & value) const
{
  const std::string& attr(getAttribute(attribute));

  if (!attr.empty()) { value = attr; return (true); }
  return (false);
}

void
IOXML::write(std::ostream& os, const XMLElement& e, int depth, bool shouldIndent)
{
  // indentation
  std::string indent, linefeed;

  if (shouldIndent) {
    indent.append(depth, ' ');
    linefeed = "\n";
  }

  // start
  os << linefeed << indent << "<" << e.getTagName();

  // attributes
  for (auto& iter: e.getAttributes()) {
    os << " " << iter.first << "=\""
       << IOXML::encodeXML(iter.second)
       << "\"";
  }

  // EMPTY?
  if (e.getText().empty() && e.getChildren().empty()) {
    os << "/>";
  } else {
    os << ">" << IOXML::encodeXML(e.getText());

    const XMLElementList& children(e.getChildren());

    // foreach_const(XMLElementList, children, it){
    for (auto it:children) {
      // write(os, **it, depth + 1);
      write(os, *it, depth + 1);
    }

    if (!children.empty()) {
      os << linefeed << indent;
    }
    os << "</" << e.getTagName() << ">";
  }
} // IOXML::write

void
IOXML::write(std::ostream& os, const XMLDocument& doc, bool shouldIndent)
{
  os << "<?xml version=\"1.0\"?>\n";
  os << "<!DOCTYPE " << doc.getRootElement().getTagName() << ">\n";
  write(os, doc.getRootElement(), 0);
  os << "\n";
}

const std::string&
XMLReader::getCurrentTag() const
{
  if (!parseStack.empty()) { return (parseStack.top()->getTagName()); }

  LogSevere("XML: parse stack is empty!\n");
  static const std::string err("ERROR");
  return (err);
}

std::shared_ptr<XMLDocument>
XMLReader::parse(Buffer& b)
{
  b.data().push_back('\0');
  std::istringstream is(&b.data().front());
  return (parse(is));
}

std::shared_ptr<XMLDocument>
XMLReader::parse(std::istream& is)
{
  XMLReader parser(is);

  const bool parseOK = parser.parse();

  if (!parseOK) { return (0); }

  if (!parser.parseStack.empty()) {
    LogSevere("IOXML: unclosed tag \"" << parser.getCurrentTag() << "\"\n");
    return (0);
  }

  return (parser.doc);
}

bool
XMLReader::parse()
{
  static const std::streamsize maxStreamLen =
    std::numeric_limits<std::streamsize>::max();

  bool peeked(false);
  bool r;

  do {
    if (!peeked) {
      is->ignore(maxStreamLen, '<'); // jump forward past the next
    }

    // '<'
    peeked = false;

    if (!*is) { // eof before next element,
                // which is fine
      return (true);
    }

    // read the next section
    std::string text;
    std::getline(*is, text, '>'); // read everything until '>' or eof.

    if (text.empty()) { // eof before next element, which is fine
      return (true);
    }

    if (!*is) {
      LogSevere("XML: spurious text at end: [" << text << "]\n");
      return (false);
    }

    // delegate to appropriate handler method
    const char front(text[0]);

    if (front == '?') {
      r = handleProcessingInstruction(text);
    } else if (front == '!') {
      if ((text.size() > 2) && (text[1] == '-') &&
        (text[2] == '-')) { r = handleComment(text); } else { r = handleDTDEntity(text); }
    } else if (front == '/') {
      r = handleEndElement(text);
    } else if (*text.rbegin() == '/') {
      r = handleEmptyElement(text);
    } else { r = peeked = handleStartElement(text); }
  } while (r == true);

  // failed
  return (false);
} // XMLReader::parse

bool
XMLReader::handleStartElement(const std::string& startTag)
{
  createElement(startTag);

  std::string text;
  std::getline(*is, text, '<'); // read until '<' or eof. (no CDATA)

  if (!*is) {
    LogSevere("IOXML: Element " << getCurrentTag() << " is incomplete.\n");
    return (false);
  }

  // set the text -- trim for ignorable white space
  parseStack.top()->setText(IOXML::decodeXML(Strings::trimText(text)));
  return (true);
}

bool
XMLReader::handleEmptyElement(const std::string& s)
{
  createElement(s.substr(0, s.size() - 1)); // trim out '/' from "/>"
  // no text -- ends immediately
  parseStack.pop();
  return (true);
}

bool
XMLReader::handleEndElement(const std::string& s)
{
  // compare our current tag to `s', but ignore the leading slash in 's'
  if (s.compare(1, std::string::npos, getCurrentTag())) {
    LogSevere(
      "IOXML: tag \"" << getCurrentTag() << "\" is closed by \"" << s << "\"\n");
    return (false);
  }

  parseStack.pop();

  return (true);
}

bool
XMLReader::
handleProcessingInstruction(const std::string& chars)
{
  return (true);
}

bool
XMLReader::
handleDTDEntity(const std::string& chars)
{
  return (true);
}

bool
XMLReader::handleComment(const std::string& chars)
{
  // std::string s(chars);
  auto s(chars);

  for (;;) {
    // did we reach the end of the comments, i.e., a "-->"
    // const std::string::size_type len(s.size());
    const auto len(s.size());

    if ((len > 2) && (s[len - 1] == '-') && (s[len - 2] == '-')) { return (true); }

    std::getline(*is, s, '>'); // read until '>' or eof.

    if (!*is) { return (false); }
  }
}

namespace {
std::vector<std::string> fromlist;
std::vector<std::string> tolist;

void
ensureEntitiesDefined()
{
  if (fromlist.empty()) {
    fromlist.push_back("&");
    tolist.push_back("&amp;"); // first encode
    fromlist.push_back("<");
    tolist.push_back("&lt;");
    fromlist.push_back(">");
    tolist.push_back("&gt;");
    fromlist.push_back("'");
    tolist.push_back("&apos;");
    fromlist.push_back("\"");
    tolist.push_back("&quot;");
  }
}
}

std::string
IOXML::decodeXML(const std::string& input)
{
  if (input.find('&') == std::string::npos) { return (input); }

  ensureEntitiesDefined();

  std::string result(input);

  const size_t s = fromlist.size();
  for (size_t i = s - 1; i >= 0; --i) {
    Strings::replace(result, tolist[i], fromlist[i]);
  }
  return (result);
}

std::string
IOXML::encodeXML(const std::string& input)
{
  ensureEntitiesDefined();

  std::string result(input);

  const size_t s = fromlist.size();
  for (size_t i = 0; i < s; ++i) {
    Strings::replace(result, fromlist[i], tolist[i]);
  }
  return (result);
}

bool
XMLReader::createElement(const std::string& s)
{
  std::string::size_type namePos(0), nameLen(0);

  if (!Strings::findNextWord(s, namePos, nameLen)) { return (false); }

  // build an element and parent it properly...
  std::shared_ptr<XMLElement> e(new XMLElement(s.substr(namePos, nameLen)));

  if (parseStack.empty()) {
    // These tags in doc don't have parent tags
    if (doc == nullptr) {
      std::shared_ptr<XMLDocument> newOne(new XMLDocument(e));
      doc = newOne;
    } else {
      doc->addChild(e);
    }
  } else {
    parseStack.top()->addChild(e);
  }
  parseStack.push(e);

  return (createAttributes(s, namePos + nameLen));
}

bool
XMLReader::createAttributes(const std::string & s,
  std::string::size_type                      pos)
{
  const Strings::WhiteSpacePred ws;

  for (;;) {
    // find name beginning
    std::string::size_type unused;

    if (!Strings::findNextWord(s, pos, unused, pos)) {
      return (true); // nothing
    }

    // left to
    // read...

    // find name end
    std::string::size_type equalPos = s.find('=', pos);

    if (pos == std::string::npos) {
      LogSevere(
        "XML: incomplete attribute at position " << pos << " of [" << s << "]\n");
      return (false);
    }

    // build name string -- remove whitespace
    std::string name(s, pos, equalPos - pos);
    name.erase(std::remove_if(name.begin(), name.end(), ws), name.end());

    // find value beginning
    if (!Strings::findNextWord(s, pos, unused, ++equalPos)) {
      LogSevere(
        "XML: incomplete attribute at position " << equalPos << " of [" << s
                                                 << "]\n");
      return (false);
    }

    // find value end
    const char delimiter = s[pos];
    ++pos; // walk past delimiter
    const std::string::size_type closePos = s.find(delimiter, pos);

    if (closePos == std::string::npos) {
      LogSevere(
        "XML: incomplete attribute value at position " << pos << " of [" << s
                                                       << "]\n");
      return (false);
    }

    // build value string
    std::string value(s.substr(pos, closePos - pos));
    value = IOXML::decodeXML(value);
    pos   = closePos + 1;

    // set the attribute
    parseStack.top()->setAttribute(name, value);
  }
} // XMLReader::createAttributes

std::shared_ptr<XMLDocument>
IOXML::readXMLDocument(const URL& url)
{
  std::shared_ptr<XMLDocument> xml;
  Buffer buf;

  if (IOURL::read(url, buf) > 0) {
    //  ProcessTimer("Parsing file...\n");
    xml = XMLReader::parse(buf);
  }
  return (xml);
}

std::shared_ptr<XMLDocument>
IOXML::readXMLDocument(std::istream& is)
{
  std::shared_ptr<XMLDocument> doc = XMLReader::parse(is);
  return doc;
}
