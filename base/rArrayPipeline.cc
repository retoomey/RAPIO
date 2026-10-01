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
#include <rOutlierFilter.h>
#include <rSavitzkyGolayFilter.h>

using namespace rapio;

void
ArrayPipeline::introduceSelf()
{
  static bool first = true;

  if (first) {
    // Samplers
    Bilinear::introduceSelf();
    Cressman::introduceSelf();
    NearestNeighbor::introduceSelf();

    // 2D filters
    ThresholdFilter::introduceSelf();
    PercentFilter::introduceSelf();
    DespeckleFilter::introduceSelf();
    DilateFilter::introduceSelf();

    // 1D filters
    OutlierFilter::introduceSelf();
    SavitzkyGolayFilter::introduceSelf();
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

  auto samplers = Factory<ArraySampler>::getAll();

  help += "  Samplers (start of pipeline when remapping):\n";
  for (auto a : samplers) {
    help += "  " + ColorTerm::red() + a.first + ColorTerm::reset() + " : " + a.second->getHelpString() + "\n";
  }

  help += "  Filters (pipeline):\n";
  auto full = Factory<ArrayFilter>::getAll();

  for (auto a : full) {
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

    char delimiter = ':';
    size_t pos     = stage.find(delimiter);
    if (pos != std::string::npos) {
      type   = stage.substr(0, pos);
      params = stage.substr(pos + 1);
    } else {
      type   = stage;
      params = "";
    }
    type = Strings::makeLower(type);

    if (firstStage) {
      firstStage = false;
      auto sampler = Factory<ArraySampler>::get(type, "ArraySampler: " + type);
      if (sampler) {
        if (sampler->parseOptions(params)) {
          fLogInfo("Pipeline sampler {}", type);
          pipeline->mySampler     = sampler;
          pipeline->mySamplerType = type;
        } else {
          fLogSevere("Failed to parse options for ArraySampler: {}", stage);
          return nullptr;
        }
        continue;
      }
    }

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
ArrayPipeline::remap(const std::shared_ptr<ArrayBase>& src,
  const std::shared_ptr<ArrayBase>                   & dst,
  const ArrayMapper                                  & mapper) const
{
  if (!src || !dst) { return; }
  size_t dims = src->getNumDimensions();

  std::shared_ptr<ArraySampler> activeSampler = mySampler;

  if (!activeSampler) {
    activeSampler = std::make_shared<NearestNeighbor>(); // Default fallback
  }

  if (!activeSampler->supportsDimensions(dims)) {
    fLogSevere("Sampler '{}' does not support {}D data.", mySamplerType.empty() ? "nearest" : mySamplerType, dims);
    return;
  }

  if (dims == 1) {
    auto src1D = std::dynamic_pointer_cast<Array<float, 1> >(src);
    auto dst1D = std::dynamic_pointer_cast<Array<float, 1> >(dst);
    activeSampler->remap1D(src1D, dst1D, mapper);
    executeFiltersPingPong1D(dst1D);
  } else if (dims == 2) {
    auto src2D = std::dynamic_pointer_cast<Array<float, 2> >(src);
    auto dst2D = std::dynamic_pointer_cast<Array<float, 2> >(dst);
    activeSampler->remap2D(src2D, dst2D, mapper);
    executeFiltersPingPong2D(dst2D);
  } else if (dims == 3) {
    auto src3D = std::dynamic_pointer_cast<Array<float, 3> >(src);
    auto dst3D = std::dynamic_pointer_cast<Array<float, 3> >(dst);
    activeSampler->remap3D(src3D, dst3D, mapper);
    executeFiltersPingPong3D(dst3D);
  }
}

void
ArrayPipeline::process(const std::shared_ptr<ArrayBase>& src,
  const std::shared_ptr<ArrayBase>                     & dst) const
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

  size_t dims = src->getNumDimensions();

  for (auto& filter : myFilters) {
    if (!filter->supportsDimensions(dims)) {
      fLogSevere("Pipeline aborted: A filter in the chain does not support {}D data.", dims);
      return;
    }
  }

  if (dims == 1) {
    auto src1D = std::dynamic_pointer_cast<const Array<float, 1> >(src);
    auto dst1D = std::dynamic_pointer_cast<Array<float, 1> >(dst);

    if (src1D != dst1D) {
      auto s = src1D->refAs1D();
      auto d = dst1D->refAs1D();
      std::copy(s.begin(), s.end(), d.begin());
    }
    executeFiltersPingPong1D(dst1D);
  } else if (dims == 2) {
    auto src2D = std::dynamic_pointer_cast<Array<float, 2> >(src);
    auto dst2D = std::dynamic_pointer_cast<Array<float, 2> >(dst);
    if (src2D != dst2D) {
      auto s = src2D->refAs1D();
      auto d = dst2D->refAs1D();
      std::copy(s.begin(), s.end(), d.begin());
    }
    executeFiltersPingPong2D(dst2D);
  } else if (dims == 3) {
    auto src3D = std::dynamic_pointer_cast<Array<float, 3> >(src);
    auto dst3D = std::dynamic_pointer_cast<Array<float, 3> >(dst);
    if (src3D != dst3D) {
      auto s = src3D->refAs1D();
      auto d = dst3D->refAs1D();
      std::copy(s.begin(), s.end(), d.begin());
    }
    executeFiltersPingPong3D(dst3D);
  }
} // ArrayPipeline::process

void
ArrayPipeline::processInPlace(const std::shared_ptr<ArrayBase>& data) const
{
  process(data, data);
}

void
ArrayPipeline::executeFiltersPingPong1D(const std::shared_ptr<Array<float, 1> >& target) const
{
  if (myFilters.empty()) { return; }
  auto temp = std::make_shared<Array<float, 1> >(target->getSizes());

  temp->fill(0);

  // Take raw pointers to the shared_ptr objects.
  // No atomic reference counting is triggered here!
  const std::shared_ptr<Array<float, 1> > * currentSrc = &target;
  const std::shared_ptr<Array<float, 1> > * currentDst = &temp;

  for (auto& filter : myFilters) {
    // Dereference the raw pointer to pass the const std::shared_ptr&
    filter->process1D(*currentSrc, *currentDst);
    // Swap the raw pointers, not the shared_ptrs. Zero overhead.
    std::swap(currentSrc, currentDst);
  }

  // If the final result ended up in the temp buffer, copy it back
  if (currentSrc == &temp) {
    auto s = (*currentSrc)->refAs1D();
    auto d = target->refAs1D();
    std::copy(s.begin(), s.end(), d.begin());
  }
}

void
ArrayPipeline::executeFiltersPingPong2D(const std::shared_ptr<Array<float, 2> >& target) const
{
  if (myFilters.empty()) { return; }
  auto temp = std::make_shared<Array<float, 2> >(target->getSizes());

  temp->fill(0);

  const std::shared_ptr<Array<float, 2> > * currentSrc = &target;
  const std::shared_ptr<Array<float, 2> > * currentDst = &temp;

  for (auto& filter : myFilters) {
    filter->process2D(*currentSrc, *currentDst);

    std::swap(currentSrc, currentDst);
  }

  if (currentSrc == &temp) {
    auto s = (*currentSrc)->refAs1D();
    auto d = target->refAs1D();
    std::copy(s.begin(), s.end(), d.begin());
  }
}

void
ArrayPipeline::executeFiltersPingPong3D(const std::shared_ptr<Array<float, 3> >& target) const
{
  if (myFilters.empty()) { return; }
  auto temp = std::make_shared<Array<float, 3> >(target->getSizes());

  temp->fill(0);

  const std::shared_ptr<Array<float, 3> > * currentSrc = &target;
  const std::shared_ptr<Array<float, 3> > * currentDst = &temp;

  for (auto& filter : myFilters) {
    filter->process3D(*currentSrc, *currentDst);
    std::swap(currentSrc, currentDst);
  }

  if (currentSrc != &temp) {
    auto s = (*currentSrc)->refAs1D();
    auto d = target->refAs1D();
    std::copy(s.begin(), s.end(), d.begin());
  }
}

void
ArrayPipeline::process(const std::vector<float>& src, std::vector<float>& dst) const
{
  if (myFilters.empty()) { return; }

  if (dst.size() < src.size()) {
    dst.resize(src.size());
  }

  // Ping-pong buffers
  std::vector<float> temp(src.size());

  const std::vector<float> * currentSrc = &src;
  std::vector<float> * currentDst       = (&src == &dst) ? &temp : &dst;

  for (auto& filter : myFilters) {
    filter->process1D(*currentSrc, *currentDst);

    // Swap pointers for the next pass
    currentSrc = currentDst;
    currentDst = (currentDst == &dst) ? &temp : &dst;
  }

  // If the final result ended up in the temp buffer, copy it to dst
  if (currentSrc == &temp) {
    std::copy(temp.begin(), temp.end(), dst.begin());
  }
}
