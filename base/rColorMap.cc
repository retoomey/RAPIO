#include "rColorMap.h"
#include "rColor.h"
#include "rError.h"
#include "rURL.h"
#include "rIODataType.h"
#include "rIOURL.h"
#include "rConfig.h"
#include "rStrings.h"
#include "rIOJSON.h"
#include "rIOXML.h"
#include "rConfigColorMap.h"

using namespace rapio;

void
ColorMap::setSpecialColorStrings(const std::string& missing, const std::string& unavailable)
{
  unsigned char r, g, b, a;

  if (!Color::RGBStringToColor(missing, r, g, b, a)) {
    // Set the missing color to what the bins currently return
    getDataColor(Constants::MissingData, r, g, b, a);
  }
  setMissingColor(r, g, b, a);

  if (!Color::RGBStringToColor(unavailable, r, g, b, a)) {
    // Set the unavailable color to what the bins currently return
    getDataColor(Constants::DataUnavailable, r, g, b, a);
  }
  setUnavailableColor(r, g, b, a);
}

std::shared_ptr<ColorMap>
ColorMap::getColorMap(const std::string& key)
{
  // Delegate to the Config
  return ConfigColorMap::getColorMap(key);
} // ColorMap::getColorMap

void
NullColorMap::getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // a test grid
  if (v == Constants::MissingData) {
    r = 255;
    g = 0;
    b = 0;
    a = 255;
  } else if (v == Constants::DataUnavailable) {
    r = 0;
    g = 0;
    b = 255;
    a = 255;
  } else if (v == Constants::RangeFolded) {
    r = 255;
    g = 255;
    b = 0;
    a = 255;
  } else {
    r = 0;
    g = 255;
    b = 0;
    a = 255;
  }
}

void
NullColorMap::getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // a test grid
  if (v == Constants::MissingData) {
    r = 255;
    g = 0;
    b = 0;
    a = 255;
  } else if (v == Constants::DataUnavailable) {
    r = 0;
    g = 0;
    b = 255;
    a = 255;
  } else if (v == Constants::RangeFolded) {
    r = 255;
    g = 255;
    b = 0;
    a = 255;
  } else {
    r = 0;
    g = 255;
    b = 0;
    a = 255;
  }
}

void
DefaultColorMap::toJSON(std::ostream& o)
{
  std::shared_ptr<PTreeData> theJson = std::make_shared<PTreeData>();
  auto tree = theJson->getTree();

  tree->put("Name", myName);

  // Create the array of upperbounds
  PTreeNode upperBounds;

  #if 0
  // I don't seem to need this.  The bins hold it..so in
  // theory we're ok if the color map is formatted correctly.
  for (size_t i = 0; i < myUpperBounds.size(); ++i) {
    auto& u = myUpperBounds[i];
    PTreeNode item; // Dirty just for simple arrays I think

    item.put("", u); // c++ will do inf, -inf, nan

    /*
     * if (std::isinf(u)){
     * if (u > 0){
     * item.put("", "inf");
     * }else{
     * item.put("", "-inf");
     * }
     * }else if (std::isnan(u)){
     * item.put("", "nan");
     * }else{
     * item.put("", u);
     * }
     */
    upperBounds.addArrayNode(item);
  }
  tree->addNode("UpperBounds", upperBounds);
  #endif // if 0

  // Create array of colors
  PTreeNode colors;

  for (size_t i = 0; i < myColorInfo.size(); ++i) {
    auto& c = myColorInfo[i];
    PTreeNode item;
    item.put("label", c.myLabel);
    item.put("linear", c.myIsLinear);
    item.put("r", c.r);
    item.put("g", c.g);
    item.put("b", c.b);
    item.put("a", c.a);
    item.put("r2", c.r2);
    item.put("g2", c.g2);
    item.put("b2", c.b2);
    item.put("a2", c.a2);
    item.put("lower", c.l);
    item.put("upper", c.u);
    item.put("delta", c.d);
    colors.addArrayNode(item);
  }
  tree->addNode("Colors", colors);

  // FIXME: We could add a stream option to IOJSON, IOXML.  API could get better
  //   size_t aLength = IODataType::writeBuffer(theJson, buffer, "json");
  std::vector<char> buffer;

  IOJSON::writePTreeDataBuffer(theJson, buffer); // better or not? More direct

  if (buffer.size() > 1) { // Skip end 0 marker for stream
    for (size_t i = 0; i < buffer.size() - 1; ++i) {
      o << buffer[i];
    }
  }
} // DefaultColorMap::toJSON

void
DefaultColorMap::toSVG(std::ostream& o, const std::string& units, const size_t width, const size_t height)
{
  // FIXME: flag for flipping ordering or vertical/horizontal maybe
  const int s = myColorInfo.size();

  // Create a PTree and use XML?  It might work, right?
  //
  std::shared_ptr<PTreeData> theSVG = std::make_shared<PTreeData>();
  auto tree = theSVG->getTree();
  constexpr size_t BoxWidth     = 22; // 50
  constexpr size_t BoxHeight    = 22;
  constexpr size_t BoxOffsetX   = 45; // 10;
  constexpr size_t LabelOffsetX = 23; // BoxOffsetX+(BoxWidth/2);
  constexpr bool drawTicks      = true;

  PTreeNode svg;

  svg.putAttr("id", "svg_legend");
  // Seems to mess with viewBox dynamic sizing
  // svg.putAttr("width", width);
  // svg.putAttr("height", height);
  svg.putAttr("xmlns", "http://www.w3.org/2000/svg");

  // The 'defs' let us define gradient colors
  PTreeNode defs;

  for (size_t i = 0; i < s; ++i) {
    auto& c = myColorInfo[i];

    PTreeNode item;

    if (c.myIsLinear) {
      // linearGradient item
      item.putAttr("id", i);
      item.putAttr("x1", "0%");
      item.putAttr("y1", "0%");
      item.putAttr("x2", "0%");
      item.putAttr("y2", "100%");

      // stop offset 0%
      PTreeNode start;
      start.putAttr("offset", "0%");
      std::stringstream s1;
      s1 << "stop-color:rgb("
         << static_cast<unsigned int>(c.r2) << ","
         << static_cast<unsigned int>(c.g2) << ","
         << static_cast<unsigned int>(c.b2) << ");stop-opacity:1";
      start.putAttr("style", s1.str());
      item.addNode("stop", start);

      // stop offset 100%
      PTreeNode stop;
      stop.putAttr("offset", "100%");
      std::stringstream s2;
      s2 << "stop-color:rgb("
         << static_cast<unsigned int>(c.r) << ","
         << static_cast<unsigned int>(c.g) << ","
         << static_cast<unsigned int>(c.b) << ");stop-opacity:1";
      stop.putAttr("style", s2.str());
      stop.putAttr("marker", "test");
      item.addNode("stop", stop);

      defs.addNode("linearGradient", item);
    }
  }
  svg.addNode("defs", defs);

  // Add the units
  PTreeNode text;

  text.putAttr("text-anchor", "middle");
  text.putAttr("x", "40"); // ??? Some guess on centering values or width?
  text.putAttr("y", "28");
  text.putAttr("font-family", "Arial");
  text.putAttr("font-weight", "bold");
  text.putAttr("font-size", "16");
  if (units == "MetersPerSecond") {
    text.put("", "m/s");
  } else {
    text.put("", units);
  }
  svg.addNode("text", text);

  const auto startY = 50.0;

  float y = startY;

  // for (size_t i = 0; i < s; ++i) { // Direction
  for (size_t i = s; i-- > 0;) {
    auto& c = myColorInfo[i];

    PTreeNode rect;

    rect.putAttr("x", BoxOffsetX);
    rect.putAttr("y", y);
    rect.putAttr("width", BoxWidth);
    rect.putAttr("height", BoxHeight);
    if (c.myIsLinear) {
      rect.putAttr("fill", "url(#" + std::to_string(i) + ")");
    } else {
      std::stringstream s3;
      s3 << "rgb("
         << static_cast<unsigned int>(c.r) << ","
         << static_cast<unsigned int>(c.g) << ","
         << static_cast<unsigned int>(c.b) << ")";
      rect.putAttr("fill", s3.str());
    }

    svg.addNode("rect", rect);
    y += BoxHeight;
  }

  // Tick marks
  if (drawTicks) {
    y  = startY + .5;   // rect y start + .5
    y += BoxHeight / 2; // To center matching wg labels
    // for (int i = s; i-- > 0;) {
    for (size_t i = 0; i < s; ++i) {
      PTreeNode line;
      // Alternating like original web page, but wg labels are middle of box
      // line.putAttr("x1", i%2? BoxOffsetX-5:BoxOffsetX-10); // shift left
      line.putAttr("x1", BoxOffsetX - 10);
      line.putAttr("y1", y);
      line.putAttr("x2", BoxOffsetX); // left of the box
      line.putAttr("y2", y);
      line.putAttr("stroke", "black");
      svg.addNode("line", line);
      y += BoxHeight;
    }
  }

  // Outline the entire color bar, I think this looks better
  PTreeNode rect;

  rect.putAttr("x", BoxOffsetX);
  rect.putAttr("y", startY);
  rect.putAttr("width", BoxWidth);
  rect.putAttr("height", BoxHeight * s);
  rect.putAttr("stroke", "black");
  rect.putAttr("fill", "none");
  svg.addNode("rect", rect);

  // Text labels
  y = startY + 5.0;
  y = y + (BoxHeight / 2); // shift to middle of cell like W2
  // for (size_t i = 0; i < s; ++i) {
  for (size_t i = s; i-- > 0;) {
    auto& c = myColorInfo[i];
    PTreeNode text;
    text.putAttr("text-anchor", "middle");
    text.putAttr("x", LabelOffsetX);
    text.putAttr("y", y);
    text.putAttr("font-family", "Arial");
    text.putAttr("font-weight", "bold");
    text.putAttr("font-size", "13");
    // The black outline over white text we use in wg
    // text.putAttr("stroke", "black");
    // text.putAttr("fill", "white");
    // text.putAttr("stroke-width", "");
    text.put("", c.myLabel);

    svg.addNode("text", text);

    y += BoxHeight;
  }

  // Final viewBox setting (allow sizing effects)
  std::stringstream vb;

  vb << 0 << " " << 0 << " " << width << " " << y + 10;
  svg.putAttr("viewBox", vb.str());

  tree->addNode("svg", svg);
  std::vector<char> buffer;

  IOXML::writePTreeDataBuffer(theSVG, buffer); // better or not? More direct

  if (buffer.size() > 1) { // Skip end 0 marker for stream
    for (size_t i = 0; i < buffer.size() - 1; ++i) {
      o << buffer[i];
    }
  }
} // DefaultColorMap::toSVG

void
DefaultColorMap::addBin(bool linear, const std::string& label, double u, double l,
  unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_,
  unsigned char r2_, unsigned char g2_, unsigned char b2_, unsigned char a2_)
{
  // Instead of classes, use vectors which should be faster
  // vector or struct of vectors?
  myUpperBounds.push_back(u);

  myColorInfo.push_back(ColorBin(linear, label, l, u, r_, g_, b_, a_, r2_, g2_, b2_, a2_));
}

void
DefaultColorMap::getDataColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // FIXME:
  // On creation:
  // 1. Make sure myUpperBounds sorted.
  // 2. Make sure infinity is the last bin so value always falls into a bin, right?
  //     This will make wt always < 1.0?

  // This assumes sorted
  auto i = std::upper_bound(myUpperBounds.begin(), myUpperBounds.end(), v);

  const size_t index = i != myUpperBounds.end() ? (i - myUpperBounds.begin()) : (myColorInfo.size() - 1);

  auto& z = myColorInfo[index];

  if (z.myIsLinear) {
    // Linear bin, interpolated color
    // v has to be >= z.l and we force z.d to be positive
    const double wt = (v - z.l) / z.d;

    // wt = 1 and nwt = 0 (special case);
    // I'm not seeing this actually happen ever? If the color map
    // covers the full value space, it won't happen, so maybe
    // we ensure this on creation and toss this > check
    // if (wt > 1) {
    //     r = (unsigned char) (0.5 + z.r2); // flooring double up
    //     g = (unsigned char) (0.5 + z.g2);
    //     b = (unsigned char) (0.5 + z.b2);
    //     a = (unsigned char) (0.5 + z.a2);
    //  }else{
    const double nwt = (1.0 - wt);

    r = (unsigned char) (0.5 + nwt * z.r + wt * z.r2);
    g = (unsigned char) (0.5 + nwt * z.g + wt * z.g2);
    b = (unsigned char) (0.5 + nwt * z.b + wt * z.b2);
    a = (unsigned char) (0.5 + nwt * z.a + wt * z.a2);
    //  }
  } else {
    // Single bin, direct color
    r = z.r;
    g = z.g;
    b = z.b;
    a = z.a;
  }
} // DefaultColorMap::getDataColor

void
DefaultColorMap::getColor(double v, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char& a) const
{
  // Use a custom missing value here. Typically missing is the most called
  if (v == Constants::MissingData) {
    myMissingColor.get(r, g, b, a);
    return;
  }
  if (v == Constants::DataUnavailable) {
    myUnavailableColor.get(r, g, b, a);
    return;
  }
  return getDataColor(v, r, g, b, a);
}
