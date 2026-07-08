#pragma once

#include <rRAPIOProgram.h>

namespace rapio {
class RAPIOWebGUI : public RAPIOProgram {
public:

  /** Create tile algorithm */
  RAPIOWebGUI(){ };

  /** Declare extra command line plugins */
  virtual void
  declarePlugins() override;

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(RAPIOData& d) override;

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
  serveTile(WebMessage& w, std::shared_ptr<DataType> targetData, std::string& pathout, std::map<std::string,
    std::string>& settings);

  /** Process a "/UI" message */
  void
  handlePathUI(WebMessage& w, std::vector<std::string>& pieces);

  /** Process a "/WMS" message */
  void
  handlePathWMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  /** Process a "/TMS" message */
  void
  handlePathTMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  /** Process a "/vector" message */
  void
  handlePathVectorTMS(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

  void
  handlePathMVT(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);
  void
  handlePathGeoJSON(WebMessage& w, std::vector<std::string>& pieces, std::map<std::string, std::string>& settings);

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

  /** A proxy request from our webassembly client. Every will go though us to avoid CORS, though
   * this will require our own security measures */
  void
  handleProxy(WebMessage& w, std::vector<std::string>& pieces);

protected:

  /** Helper to fetch from memory or lazy-load from disk */
  std::shared_ptr<DataType>
  getOrLoadDataset(const std::string& layerId);

  /** Override output params for image output (global) */
  std::map<std::string, std::string> myOverride;

  /** Most recent datas for creating tiles */
  std::unordered_map<std::string, std::shared_ptr<DataType> > myDataCache;

  /** Web requests are concurrent; protect the cache */
  std::mutex myCacheMutex;

  /** Start up file name, if any */
  std::string myStartUpFile;

  /** Root of all web files */
  std::string myRoot;

  /** Data directory restriction.  Root folder to look for requested dataset.
   * Note: we'll want to chroot basically to this folder and not allow
   * relative moves. */
  std::string myDataDir;

  /** Resolve ID for fall back data */
  std::string
  resolveDatasetId(const WebMessage& w);
};
}
