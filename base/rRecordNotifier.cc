#include "rRecordNotifier.h"

#include "rStaticMethodFactory.h"
#include "rStrings.h"
#include "rColorTerm.h"

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

void
RecordNotifier::introduceHelp(std::string& help)
{
  // Bleh first attempt, so horrible.  Need to spend some
  // time on dynamic module help, etc. For now just need to
  // document in the help.
  // FIXME: thinking about writing a custom html style stream class
  help +=
    "If blank, set to {OutputDir}/code_index.fam, while if set to 'disable' then turned off (could speed up archive processing.)\n";
  help += " " + ColorTerm::fRed + "fml" + ColorTerm::fNormal + " : " + FMLRecordNotifier::getHelpString("fml") + "\n";
  help += " " + ColorTerm::fRed + "exe" + ColorTerm::fNormal + " : " + EXERecordNotifier::getHelpString("exe") + "\n";
}

bool
RecordNotifier::createNotifiers(const std::string  & nstring,
  const std::string                                & outputdir,
  std::vector<std::shared_ptr<RecordNotifierType> >& v)
{
  if (nstring == "disable") {
    LogInfo("Notifiers disabled\n");
    return true;
  }

  // Split n param by spaces
  std::vector<std::string> pieces;
  Strings::splitWithoutEnds(nstring, ' ', &pieces);

  // For each split piece
  for (auto& s: pieces) {
    std::string protocol;
    std::string params;

    // FIXME: dup code, The whole ' ' and then '=' code matches the index stuff
    // maybe can combine it...
    std::vector<std::string> pair;
    Strings::splitWithoutEnds(s, '=', &pair);
    const size_t aSize = pair.size();
    std::string path   = "";
    if (aSize == 1) {
      // Something like 'fml=' or 'test=', a type missing a parameter string
      if (Strings::endsWith(s, "=")) {
        protocol = pair[0];
        params   = "";
      } else {
        // Something like "/test", etc..try to read as location
        protocol = "fml";
        params   = pair[0];
      }
    } else if (aSize == 2) {
      protocol = pair[0];
      params   = pair[1];
    } else {
      LogSevere("Format of passed notifier '" << s << "' is wrong, see help n\n");
      exit(1);
    }

    auto n = getNotifier(params, outputdir, protocol);
    if (n != nullptr) {
      LogDebug("Added notifier: " << protocol << ":" << params << "\n");
      v.push_back(n);
    } else {
      LogSevere("Format of passed notifier '" << protocol << " = " << params << "' is wrong, see help n\n");
      exit(1);
    }
  }

  return true;
} // RecordNotifier::createNotifiers

std::shared_ptr<RecordNotifierType>
RecordNotifier::getNotifier(const std::string& params, const std::string& outputdir, const std::string& type)
{
  auto p = StaticMethodFactory<RecordNotifierType>::get(type, "Notifier");

  if (p != nullptr) {
    std::shared_ptr<RecordNotifierType> z = (*p)(); // Call the static method
    if (z != nullptr) {
      z->initialize(params, outputdir);
      return (z);
    }
  }
  return (nullptr);
}

void
RecordNotifierType::
writeRecords(const std::string& outputinfo, const std::vector<Record>& rec)
{
  for (auto r:rec) {
    writeRecord(outputinfo, r);
  }
}
