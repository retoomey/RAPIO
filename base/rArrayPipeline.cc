#include <rArrayPipeline.h>
#include <rStrings.h>
#include <rError.h>
#include <rFactory.h>
#include <rColorTerm.h>

// Include the concrete implementations so we can register them
#include <rNearestNeighbor.h>
#include <rBilinear.h>
#include <rCressman.h>
#include <rThresholdFilter.h>
#include <rPercentFilter.h>
#include <rDilateFilter.h>
#include <rDespeckleFilter.h>

using namespace rapio;

void
ArrayPipeline::introduceSelf()
{
  static bool first = true;

  // FIXME: Wondering if we could go dynamic here
  if (first) {
    Bilinear::introduceSelf();
    Cressman::introduceSelf();
    NearestNeighbor::introduceSelf();
    ThresholdFilter::introduceSelf();
    PercentFilter::introduceSelf();
    DespeckleFilter::introduceSelf();
    DilateFilter::introduceSelf();
    first = false;
  }
}

std::string
ArrayPipeline::introduceHelp()
{
  introduceSelf();
  std::string help;

  help += "Samplers and filters can be created in a pipeline for processing/remapping arrays.\n";
  help += "For example, 'cressman:3:3,threshold:18,50' pipeline does a valid threshold after";
  help += " cressman interpolation of the data field. The default on a missing sampler is nearest neighbor.\n";
  help += "The difference from WDSS2 is the addition of samplers for handling different size arrays, ";
  help += " as well as chaining N number of filters. Chain order is left to right.\n";

  // Samplers are leaf nodes in the pipeline, interpolating data values.
  auto samplers = Factory<ArraySampler>::getAll();

  help += "  Samplers (start of pipeline when remapping):\n";
  for (auto a:samplers) {
    help += "  " + ColorTerm::red() + a.first + ColorTerm::reset() + " : " + a.second->getHelpString() + "\n";
  }

  // Filters perform actions on data values.
  help += "  Filters (pipeline):\n";
  auto full = Factory<ArrayFilter>::getAll();

  for (auto a:full) {
    help += "  " + ColorTerm::red() + a.first + ColorTerm::reset() + " : " + a.second->getHelpString() + "\n";
  }
  return help;
}

std::shared_ptr<ArrayPipeline>
ArrayPipeline::create(const std::string& config)
{
  introduceSelf();

  auto pipeline = std::make_shared<ArrayPipeline>();
  std::vector<std::string> stages;

  Strings::splitWithoutEnds(config, ',', &stages);

  bool firstStage = true;

  for (const auto& stage : stages) {
    std::string type, params;

    // First part, say "filtername:params"
    char delimiter = ':';
    size_t pos     = stage.find(delimiter);
    if (pos != std::string::npos) { // If found, split once
      type   = stage.substr(0, pos);
      params = stage.substr(pos + 1);
    } else { // if not found, it's the name only
      type   = stage;
      params = "";
    }
    type = Strings::makeLower(type);

    // Attempt to extract Sampler via Factory (must be stage 1)
    if (firstStage) {
      firstStage = false;
      // Not thread safe, we set internal state on a global item (unless we clone it)
      auto sampler = Factory<ArraySampler>::get(type, "ArraySampler: " + type);
      if (sampler) {
        if (sampler->parseOptions(params)) {
          fLogInfo("Pipeline sampler {}", type);
          pipeline->mySampler     = sampler;
          pipeline->mySamplerType = type;
          //    pipeline->mySamplerArgs = parts; what?
        } else {
          fLogSevere("Failed to parse options for ArraySampler: {}", stage);
          return nullptr;
        }
        continue;
      }
    }

    // Otherwise, attempt to extract as Filter
    auto filter = Factory<ArrayFilter>::get(type, "ArrayFilter: " + type);
    if (filter) {
      if (filter->parseOptions(params)) {
        fLogInfo("Pipeline filter {}", type);
        pipeline->myFilters.push_back(filter);
      } else {
        fLogSevere("Failed to parse options for ArrayFilter: {}", stage);
        return nullptr;
      }
    } else {
      fLogSevere("Unknown pipeline stage or missing filter module: {}", type);
      return nullptr;
    }
  }

  return pipeline;
} // ArrayPipeline::create

void
ArrayPipeline::process(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                      dst)
{
  if (!src || !dst) { return; }

  if (src->getSizes() != dst->getSizes()) {
    fLogSevere("ArrayPipeline::process requires src and dst to have identical dimensions.");
    return;
  }

  if (mySampler) {
    fLogInfo("Warning: Sampler '{}' defined in config but pipeline executed as 1:1 process. Sampler ignored.",
      mySamplerType);
  }

  // 1. Direct copy for 1:1 processing
  if (src != dst) {
    auto srcData = src->refAs1D();
    auto dstData = dst->refAs1D();
    std::copy(srcData.begin(), srcData.end(), dstData.begin());
  }

  // 2. Execute Filters
  executeFiltersPingPong(dst);
}

void
ArrayPipeline::processInPlace(std::shared_ptr<Array<float, 2> > data)
{
  process(data, data);
}

void
ArrayPipeline::executeFiltersPingPong(std::shared_ptr<Array<float, 2> > target)
{
  if (myFilters.empty()) { return; }

  // Ping-pong buffer to support out-of-place filters safely
  auto temp = std::make_shared<Array<float, 2> >(target->getSizes());

  // temp->fill(rapio::Constants::DataUnavailable); // Must initialize temp memory!
  temp->fill(0); // Must initialize temp memory!
  auto currentSrc = target;
  auto currentDst = temp;

  for (auto& filter : myFilters) {
    filter->process(currentSrc, currentDst);
    std::swap(currentSrc, currentDst);
  }

  // If the final filtered result ended up in the temp buffer, copy it back to the target
  if (currentSrc != target) {
    auto srcData = currentSrc->refAs1D();
    auto dstData = target->refAs1D();
    std::copy(srcData.begin(), srcData.end(), dstData.begin());
  }
}
