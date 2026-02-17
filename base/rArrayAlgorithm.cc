#include <rArrayAlgorithm.h>
#include <rFactory.h>
#include <rColorTerm.h>

// Various samplers/filters
#include <rNearestNeighbor.h>
#include <rBilinear.h>
#include <rCressman.h>
#include <rThresholdFilter.h>

#include <rStrings.h>
#include <rError.h>

using namespace rapio;

void
ArrayAlgorithm::introduceSelf()
{
  static bool first = true;

  if (first) {
    // Samplers (leafs)
    Bilinear::introduceSelf();
    Cressman::introduceSelf();
    NearestNeighbor::introduceSelf();

    // Filters (pipelines)
    ThresholdFilter::introduceSelf();
    first = false;
  }
}

std::string
ArrayAlgorithm::introduceHelp()
{
  introduceSelf();
  std::string help;

  help += "Samplers and filters can be created in a pipeline for processing/remapping arrays.\n";
  help += "For example, 'cressman:3:3,threshold:18,50' pipeline does a valid threshold after";
  help += "cressman interpolation of the data field. The default on a missing sampler is nearest neighbor.\n";
  help += "The difference from WDSS2 is the addition of samplers for handling different size arrays, ";
  help += " as well as chaining N number of filters. Chain order is left to right.\n";

  // Samplers are leaf nodes in the pipeline, interpolating data values.
  auto samplers = Factory<ArraySampler>::getAll();

  help += "  Samplers (start of pipeline):\n";
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

std::shared_ptr<ArrayAlgorithm>
ArrayAlgorithm::create(const std::string& config)
{
  introduceSelf();

  fLogInfo("Factory request ArrayAlgorithm '{}'", config);
  std::vector<std::string> stages;

  // Split pipeline stages by comma: "bilinear:3:3,threshold:0:50"
  Strings::splitWithoutEnds(config, ',', &stages);

  std::shared_ptr<ArrayAlgorithm> pipeline = nullptr;

  bool firstInPipe = true;

  for (auto& stage : stages) {
    /** First break up say bilinear:3:3 into three parts */
    Strings::trim(stage);
    std::vector<std::string> parts;
    Strings::splitWithoutEnds(stage, ':', &parts);

    if (parts.empty()) {
      continue;
    }

    std::string type = Strings::makeLower(parts[0]);
    std::shared_ptr<ArrayAlgorithm> current = nullptr;

    // First in pipe is currently only allowed sampler
    // I tried to chain samplers but it's even more confusing,
    // I think forcing one sampler max is more than enough ability
    // at least for now.
    bool handleAsFilter = true;
    if (firstInPipe) {
      auto sampler = Factory<ArraySampler>::get(type, "ArraySampler: " + type);

      // First is a sampler...
      if (sampler) {
        pipeline       = sampler;
        handleAsFilter = false; // handled
        fLogInfo("Created sampler '{}' with '{}'", type, stage);
        // ... or if not, make a NN and handle as a filter instead
      } else {
        pipeline = std::make_shared<NearestNeighbor>();
        fLogInfo("Created sampler nearest neighbor'", type, stage);
      }
      if (!pipeline->parseOptions(parts)) {
        fLogSevere("Failed to parse sampler options {}", stage);
      }
      firstInPipe = false;
    }

    // Try to load a filter
    if (handleAsFilter) {
      auto filter = Factory<ArrayFilter>::get(type, "ArrayFilter: " + type);
      if (filter) {
        fLogInfo("Created filter '{}' with '{}'", type, stage);
        filter->setUpstream(pipeline);
        if (!filter->parseOptions(parts)) {
          fLogSevere("Failed to parse options for algorithm stage: {}", stage);
        }
        pipeline = filter;
      } else {
        fLogSevere("Failed to make filter from {}", type);
      }
    }
  }

  if (!pipeline) {
    fLogInfo("No valid pipeline created from '{}', using Nearest Neighbor.", config);
    pipeline = std::make_shared<NearestNeighbor>();
  }

  return pipeline;
} // ArrayAlgorithm::create

void
ArrayAlgorithm::process(std::shared_ptr<Array<float, 2> > source,
  std::shared_ptr<Array<float, 2> >                       dest,
  CoordMapper *                                           mapper)
{
  if (!source || !dest) { return; }
  setSource(source);

  auto srcSize  = source->getSizes();
  auto dstSize  = dest->getSizes();
  auto& destRef = dest->ref();

  // FAST PATH: Identical sizes, no mapper provided
  // Think simple 2D array to 2D array here
  if (!mapper && (srcSize == dstSize) ) {
    for (int i = 0; i < (int) dstSize[0]; ++i) {
      for (int j = 0; j < (int) dstSize[1]; ++j) {
        float out;
        if (sampleAtIndex(i, j, out)) {
          destRef[i][j] = out;
        }
      }
    }
  }
  // MAPPED PATH: Use helper or default linear stretching
  // Think one LatLonGrid geo transforming to another LatLonGrid
  // or linear stretching of one flat 2D array to another larger one
  else {
    for (int i = 0; i < (int) dstSize[0]; ++i) {
      for (int j = 0; j < (int) dstSize[1]; ++j) {
        float u, v, out;
        bool valid;

        if (mapper) {
          valid = mapper->map(i, j, u, v);
        } else {
          // Default Linear Stretching: Map [0, dst] to [0, src]
          u     = i * ((float) srcSize[0] / dstSize[0]);
          v     = j * ((float) srcSize[1] / dstSize[1]);
          valid = true;
        }

        if (valid && sampleAt(u, v, out)) {
          destRef[i][j] = out;
        } else {
          destRef[i][j] = Constants::DataUnavailable;
        }
      }
    }
  }
} // process
