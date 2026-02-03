#include "rEXERecordNotifier.h"

#include "rOS.h"
#include "rError.h"
#include "rFactory.h"

using namespace rapio;

namespace rapio {
/** Create a new instance of EXERecordNotifier.
 * @ingroup rapio_io
 * @brief Create a new instance of EXERecordNotifier
 * */
class EXERecordNotifierCreator : public RecordNotifierCreator {
public:
  std::shared_ptr<RecordNotifierType>
  create(const std::string& params) override
  {
    auto ptr = std::make_shared<EXERecordNotifier>();

    ptr->initialize(params); // Could pass to constructor now
    return ptr;
  }
};
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
  // Tiny class dedicated to creating new instances, since we can have more than one
  std::shared_ptr<EXERecordNotifierCreator> newOne = std::make_shared<EXERecordNotifierCreator>();
  Factory<RecordNotifierCreator>::introduce("exe", newOne);
}

void
EXERecordNotifier::initialize(const std::string& params)
{
  std::string checkExe = OS::validateExe(params);

  if (checkExe.empty()) {
    fLogSevere("Given exe at '{}' does not exist or needs permissions.", params);
    exit(1); // Abort or ignore?
  }

  myExe = checkExe;
  fLogInfo("Using EXE at {}", checkExe);
}

EXERecordNotifier::~EXERecordNotifier()
{ }

void
EXERecordNotifier::writeMessage(std::map<std::string, std::string>& outputParams, const Message& rec)
{
  fLogDebug("-->EXE TRYING TO WRITE MESSAGE");
}

void
EXERecordNotifier::writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec)
{
  // const std::string outputinfo = outputParams["outputfolder"];

  // FIXME: More advanced ability at some point
  // I'm just calling system and background at moment
  auto params = rec.getParams();

  if (params.size() > 1) {
    std::string command = myExe;
    for (auto p: params) { // Let the script/exe have all the params
      command += ' ' + p;  // FIXME: Harden..what if param has a shell character in it.
    }
    command += " &"; // shell background
    system(command.c_str());
  }
} // EXERecordNotifier::writeRecord
