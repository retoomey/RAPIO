#include "rRecordNotifier.h"

#include "rStaticMethodFactory.h"
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
  help += " " + ColorTerm::fRed + "fml" + ColorTerm::fNormal + " : " + FMLRecordNotifier::getHelpString("fml") + "\n";
  help += " " + ColorTerm::fRed + "exe" + ColorTerm::fNormal + " : " + EXERecordNotifier::getHelpString("exe") + "\n";
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
      LogDebug("Added notifier: " << ni.protocol << ":" << ni.params << "\n");
      v.push_back(n);
    } else {
      LogSevere("Format of passed notifier '" << ni.protocol << " = " << ni.params << "' is wrong, see help n\n");
      exit(1);
    }
  }
  return v;
} // RecordNotifier::createNotifiers

std::shared_ptr<RecordNotifierType>
RecordNotifier::createNotifier(const std::string& type, const std::string& params)
{
  auto p = StaticMethodFactory<RecordNotifierType>::get(type, "Notifier");

  if (p != nullptr) {
    std::shared_ptr<RecordNotifierType> z = (*p)(); // Call the static method
    if (z != nullptr) {
      z->initialize(params);
      return (z);
    }
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
