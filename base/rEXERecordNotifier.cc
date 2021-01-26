#include "rEXERecordNotifier.h"

#include "rOS.h"
#include "rError.h"
#include "rStaticMethodFactory.h"

using namespace rapio;

/** Static factory method */
std::shared_ptr<RecordNotifierType>
EXERecordNotifier::create()
{
  std::shared_ptr<EXERecordNotifier> result = std::make_shared<EXERecordNotifier>();
  return (result);
}

std::string
EXERecordNotifier::getHelpString(const std::string& fkey)
{
  return
    "Call script/program for each new record.\n  Example: exe=/test.exe to call test.exe with the record param list.\n  Combination: 'fml= exe=/test.exe fml=/copy' Two sets of fml records written, one to default and one to /copy.  Call test.exe with info on written data files.";
}

void
EXERecordNotifier::introduceSelf()
{
  // No need to store a special object here, could if wanted
  //  std::shared_ptr<EXERecordNotifier> newOne = std::make_shared<EXERecordNotifier>();
  //  Factory<RecordNotifierType>::introduce("fml", newOne);
  StaticMethodFactory<RecordNotifierType>::introduce("exe", &EXERecordNotifier::create);
}

void
EXERecordNotifier::initialize(const std::string& params, const std::string& outputdir)
{
  std::string checkExe = OS::validateExe(params);
  if (checkExe.empty()) {
    LogSevere("Given exe at '" << params << "' does not exist or needs permissions.\n");
    exit(1); // Abort or ignore?
  }

  myExe = checkExe;
  LogInfo("Using EXE at " << checkExe << "\n");
}

EXERecordNotifier::~EXERecordNotifier()
{ }

void
EXERecordNotifier::writeRecord(const std::string& outputinfo, const Record& rec)
{
  if (!rec.isValid()) { return; }

  // FIXME: More advanced ability at some point
  // I'm just calling system and background at moment
  auto params = rec.getBuilderParams();
  if (params.size() > 1) {
    std::string command = myExe;
    for (auto p: params) { // Let the script/exe have all the params
      command += ' ' + p;  // FIXME: Harden..what if param has a shell character in it.
    }
    command += " &"; // shell background
    system(command.c_str());
  }
} // EXERecordNotifier::writeRecord
