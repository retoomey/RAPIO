#pragma once
#include <rBinaryTable.h>
#include <string>
#include <vector>

namespace rapio {
class WindBinaryTable : public BinaryTable
{
public:
  WindBinaryTable();
  ~WindBinaryTable();

  inline void
  add(float m11, float m22, float m12, float p1, float p2, short x, short y, char z)
  {
    myM11.push_back(m11);
    myM22.push_back(m22);
    myM12.push_back(m12);
    myP1.push_back(p1);
    myP2.push_back(p2);

    myXs.push_back(x);
    myYs.push_back(y);
    myZs.push_back(z);
    myValueSize++;
  }

  inline void
  addMissing(size_t x, size_t y, size_t z, size_t l)
  {
    myXMissings.push_back(x);
    myYMissings.push_back(y);
    myZMissings.push_back(z);
    myLMissings.push_back(l);
    myMissingSize++;
  }

  virtual bool
  readBlock(const std::string& path, FILE * fp) override;
  virtual bool
  writeBlock(FILE * fp) override;

  static size_t BLOCK_LEVEL;
  virtual void
  getBlockLevels(std::vector<std::string>& levels) const override;

  // Introspection Overrides (Replaces dumpToText)
  virtual std::vector<TableInfo>
  getTableInfo() override;
  virtual std::vector<float>
  getFloatVector(const std::string& name) override;
  virtual std::vector<short>
  getShortVector(const std::string& name) override;
  virtual std::vector<char>
  getCharVector(const std::string& name) override;

  size_t
  getValueSize() const { return myValueSize; }

  size_t
  getMissingSize() const { return myMissingSize; }

  virtual bool
  getUseMissingAsUnavailable() override { return (myMissingMode == 1); }

  void setUseMissingAsUnavailable(){ myMissingMode = 1; }

protected:
  static constexpr size_t Version = 1;
  size_t myVersionID;
  char myMissingMode;
  size_t myValueSize;
  size_t myMissingSize;

public:
  // Matrix/Vector components
  std::vector<float> myM11;
  std::vector<float> myM22;
  std::vector<float> myM12;
  std::vector<float> myP1;
  std::vector<float> myP2;

  // Coordinates
  std::vector<short> myXs;
  std::vector<short> myYs;
  std::vector<char> myZs;

  // Missing RLE
  std::vector<short> myXMissings;
  std::vector<short> myYMissings;
  std::vector<char> myZMissings;
  std::vector<short> myLMissings;
};
} // namespace rapio
