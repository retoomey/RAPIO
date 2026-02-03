#include "rRecordNotifier.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rColorTerm.h"
#include "rConfigParamGroup.h"

// Subclasses we introduce
#include "rFMLRecordNotifier.h"
#include "rEXERecordNotifier.h"

using namespace rapio;
using namespace std;

void
RecordNotifier::introduceSelf()
{
  FMLRecordNotifier::introduceSelf();
  EXERecordNotifier::introduceSelf();
}

std::string
RecordNotifier::introduceHelp()
{
  std::string help;

  help +=
    "If blank, set to {OutputDir}/code_index.fam, while if set to 'disable' then turned off (could speed up archive processing.)\n";
  help += " " + ColorTerm::red() + "fml" + ColorTerm::reset() + " : " + FMLRecordNotifier::getHelpString("fml") + "\n";
  help += " " + ColorTerm::red() + "exe" + ColorTerm::reset() + " : " + EXERecordNotifier::getHelpString("exe") + "\n";
  return help;
}

std::vector<std::shared_ptr<RecordNotifierType> >
RecordNotifier::createNotifiers()
{
  std::vector<std::shared_ptr<RecordNotifierType> > v;
  const auto& notifiers = ConfigParamGroupn::getNotifierInfo();

  for (auto& ni:notifiers) {
    auto n = createNotifier(ni.protocol, ni.params);
    if (n != nullptr) {
      fLogDebug("Added notifier: {}:{}", ni.protocol, ni.params);
      v.push_back(n);
    } else {
      fLogSevere("Format of passed notifier '{} = {}' is wrong, see help n", ni.protocol, ni.params);
      exit(1);
    }
  }
  return v;
} // RecordNotifier::createNotifiers

std::shared_ptr<RecordNotifierType>
RecordNotifier::createNotifier(const std::string& type, const std::string& params)
{
  auto p = Factory<RecordNotifierCreator>::get(type, "Notifier");

  if (p != nullptr) {
    // Letting subclasses control how it's created/initialized
    auto newOne = p->create(params);
    return newOne;
  }

  return (nullptr);
}

void
RecordNotifierType::
writeRecords(std::map<std::string, std::string>& outputParams, const std::vector<Record>& rec)
{
  for (auto r:rec) {
    writeRecord(outputParams, r);
  }
}
