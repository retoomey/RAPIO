#pragma once

#include <rConfig.h>
#include <string>
#include <vector>

namespace rapio {
/** Shared code, info for param groups.  These
 * are passed on command line and involve string spliting and parsing
 * to store information for other classes. It's possible we'll
 * process these from XML as well at some point, so those functions will be added
 * here. This abstracts the string parsing processing from rest of the code.
 * @author Robert Toomey
 */
class ConfigParamGroup : public ConfigType
{
public:
  ConfigParamGroup(){ };

  /** Process a single part of param string */
  virtual void
  process1(const std::string& param){ };

  /** Process a full param string */
  virtual void
  process(const std::string& param);

  /** We don't read from XML, algorithm will send us a string from command line */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> ) override { return true; }

  /** Read configuration information from a command line string */
  virtual bool
  readString(const std::string& param) override { process(param); return true; };
};

/** The -i param and information stored from it */
class ConfigParamGroupi : public ConfigParamGroup
{
public:
  class indexInputInfo : public Data {
public:
    /* Constructor to make sure no fields missed */
    indexInputInfo(const std::string& p, const std::string& i) : protocol(p), indexparams(i){ }

    std::string protocol;
    std::string indexparams;
  };

  ConfigParamGroupi(){ };

  /** Process a single part of param string */
  virtual void
  process1(const std::string& param) override;

  /** Get index info database */
  static const std::vector<indexInputInfo>&
  getIndexInputInfo(){ return myIndexInputInfo; }

protected:
  /** Database of added indexes we look for products in */
  static std::vector<indexInputInfo> myIndexInputInfo;
};

/** The -I param and information stored from it */
class ConfigParamGroupI : public ConfigParamGroup
{
public:

  /** Store database of product information */
  class productInputInfo : public Data {
public:
    /* Constructor to make sure no fields missed */
    productInputInfo(const std::string& n, const std::string& s)
      : name(n), subtype(s){ }

    std::string name;
    std::string subtype;
  };

  ConfigParamGroupI(){ };

  /** Get product info database */
  static const std::vector<productInputInfo>&
  getProductInputInfo(){ return myProductInputInfo; }

  /** Process a single part of param string */
  virtual void
  process1(const std::string& param) override;

protected:
  /** Database of added input products we want from any index */
  static std::vector<productInputInfo> myProductInputInfo;
};

/** The -o param and information stored from it */
class ConfigParamGroupo : public ConfigParamGroup
{
public:

  /** Store -o output information for the writers */
  class outputInfo : public Data {
public:
    /* Constructor to make sure no fields missed */
    outputInfo(const std::string& f, const std::string& o)
      : factory(f), outputinfo(o){ }

    std::string factory;
    std::string outputinfo;
  };

  ConfigParamGroupo(){ };

  /** Process a single part of param string */
  virtual void
  process1(const std::string& param) override;

  /** Get output info database */
  static const std::vector<outputInfo>&
  getWriteOutputInfo(){ return myWriters; }

protected:
  /** The list of writers to attempt */
  static std::vector<outputInfo> myWriters;
};

/** The -O param and information stored from it */
class ConfigParamGroupO : public ConfigParamGroup
{
public:

  /** Store database of output information */
  class productOutputInfo : public Data {
public:
    /* Constructor to make sure no fields missed */
    productOutputInfo(const std::string& p, const std::string& s,
      const std::string& toP, const std::string& toS)
      : product(p), subtype(s), toProduct(toP), toSubtype(toS){ }

    /** The product key matcher such as '*', "Reflectivity*" */
    std::string product;

    /** The subtype key matcher, if any */
    std::string subtype;

    /** The product key to translate into */
    std::string toProduct;

    /** The subtype key to translate into, if any */
    std::string toSubtype;
  };

  ConfigParamGroupO(){ };

  /** Get product info database */
  static const std::vector<productOutputInfo>&
  getProductOutputInfo(){ return myProductOutputInfo; }

  /** Process a single part of param string */
  virtual void
  process1(const std::string& param) override;
protected:
  /** Database of added output products we can generate */
  static std::vector<productOutputInfo> myProductOutputInfo;
};

/** The -n param and information stored from it */
class ConfigParamGroupn : public ConfigParamGroup
{
public:

  /** Store -n output information for the record notifier */
  class notifierInfo : public Data {
public:
    /* Constructor to make sure no fields missed */
    notifierInfo(const std::string& proto, const std::string& p)
      : protocol(proto), params(p){ }

    std::string protocol;
    std::string params;
  };

  ConfigParamGroupn() : myDisabled(false){ };

  /** Get product info database */
  static const std::vector<notifierInfo>&
  getNotifierInfo(){ return myNotifierInfo; }

  /** Process a single part of param string */
  virtual void
  process1(const std::string& param) override;

  /** Process a full param string */
  virtual void
  process(const std::string& param) override;

protected:
  /** Database of notifier info */
  static std::vector<notifierInfo> myNotifierInfo;

  /** Disabled or not */
  bool myDisabled;
};
}
