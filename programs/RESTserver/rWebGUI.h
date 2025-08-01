#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
class RAPIOWebGUI : public rapio::RAPIOProgram {
public:

  /** Create tile algorithm */
  RAPIOWebGUI(){ };

  /** Declare extra command line plugins */
  virtual void
  declarePlugins() override;

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process a web message */
  virtual void
  processWebMessage(std::shared_ptr<WebMessage> message) override;

  /** Execute the server */
  virtual void
  execute() override;

protected:

  /** Debug server log write */
  void
  logWebMessage(const WebMessage& w);

  /** Process web GET params pairs and update params to image writer */
  void
  handleOverrides(const std::map<std::string, std::string>& params, std::map<std::string, std::string>& settings);

  /** Serve a web tile image from a cache */
  void
  serveTile(WebMessage& w, std::string& pathout, std::map<std::string, std::string>& settings);

  /** Process a "/UI" message */
  void
  handlePathUI(WebMessage& w, std::vector<std::string>& pieces);

  /** Process a "/WMS" message */
  void
  handlePathWMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  /** Process a "/TMS" message */
  void
  handlePathTMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  /** Process a "/DATA" message */
  void
  handlePathData(WebMessage& w, std::vector<std::string>& pieces);

  /** Process a "/" message */
  void
  handlePathDefault(WebMessage& w);

  /** Request a color map */
  void
  handleColorMap(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  /** Request a SVG */
  void
  handleSVG(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  /** Override output params for image output (global) */
  std::map<std::string, std::string> myOverride;

  /** Most recent data for creating tiles */
  std::shared_ptr<DataType> myTileData;

  /** Start up file name, if any */
  std::string myStartUpFile;

  /** Root of all web files */
  std::string myRoot;
};
}
