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
EXERecordNotifier::writeRecord(const Record& rec, const std::string& file)
{
  if (!rec.isValid()) { return; }

  // FIXME: More advanced ability at some point
  // I'm just calling system and background at moment
  std::string command = myExe + ' ' + file;
  command += " &"; // shell background
  system(command.c_str());
} // EXERecordNotifier::writeRecord
