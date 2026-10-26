#pragma once

namespace rapio {
/** All raw specializers subclass to this */
class RawSpecializer : public IOSpecializer
{
public:
  virtual std::shared_ptr<DataType>
  readRAW(FILE * fp, const std::string& path, IOConfig& keys) = 0;

  virtual bool
  writeRAW(FILE *, std::shared_ptr<DataType>, IOConfig&){ return false; }

  // Fulfill the base IOSpecializer contract (though typically unused for RAW direct file ops)
  virtual std::shared_ptr<DataType>
  read(IOConfig&) override { return nullptr; }

  virtual bool
  write(std::shared_ptr<DataType>, IOConfig&) override { return false; }
};
}
