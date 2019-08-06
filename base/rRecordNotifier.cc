#include "rRecordNotifier.h"

#include "rFactory.h"
#include "rStrings.h"
#include "rIOXML.h"

#include "rFMLRecordNotifier.h"

using namespace rapio;
using namespace std;

void
RecordNotifier::introduce(const string & protocol,
  std::shared_ptr<RecordNotifier>      factory)
{
  Factory<RecordNotifier>::introduce(protocol, factory);
}

void
RecordNotifier::introduceSelf()
{
  FMLRecordNotifier::introduceSelf();
}

std::shared_ptr<RecordNotifier>
RecordNotifier::getNotifier(const URL& notifyLoc, const URL& dataLoc, const std::string& type)
{
  // We only have one use of each notifier.  FmlRecordNotifier keeps the
  // path as internal variable.  Would need to change that for multiple paths...
  std::shared_ptr<RecordNotifier> d = Factory<RecordNotifier>::get(type);
  d->setURL(notifyLoc, dataLoc);
  return (d);
}

std::string
RecordNotifier::getDefaultBaseName()
{
  return ("code_index.fam");
}

void
RecordNotifier::
writeRecords(const std::vector<Record>& rec)
{
  for (auto& i:rec) {
    writeRecord(i);
  }
}
