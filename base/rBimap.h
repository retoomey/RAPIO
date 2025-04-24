#pragma once

#include <rData.h>
#include <rBOOST.h>

#include <map>

namespace rapio {
/* (AI) Bimap allows two direction key to value pair lookup
 *      in various ways.
 *
 * @author Robert Toomey
 */
template <typename Key, typename Value>
class BimapInterface : public Data {
public:
  /** Insert a key/value pair into the bimap */
  virtual void
  insert(const Key& key, const Value& value) = 0;
  /** Get a key given a value */
  virtual Key
  getKey(const Value& value) const = 0;
  /** Get a value given a key */
  virtual Value
  getValue(const Key& key) const = 0;
};

/** Bimap implementation using Boost.Bimap.  This should be
 * a reasonably fast two way general Key <--> Value lookup. */
template <typename Key, typename Value>
class BimapBoost : public BimapInterface<Key, Value> {
public:
  using BimapType = boost::bimap<boost::bimaps::set_of<Key>, boost::bimaps::set_of<Value> >;

  /** Insert a key/value pair into the bimap */
  void
  insert(const Key& key, const Value& value) override
  {
    myBimap.insert({ key, value });
  }

  /** Get a key given a value */
  Key
  getKey(const Value& value) const override
  {
    auto it = myBimap.right.find(value);

    return it != myBimap.right.end() ? it->second : Key{ };
  }

  /** Get a value given a key */
  Value
  getValue(const Key& key) const override
  {
    auto it = myBimap.left.find(key);

    return it != myBimap.left.end() ? it->second : Value{ };
  }

private:
  /** The simple boost bimap we wrap here */
  BimapType myBimap;
};

/** Bimap implementation using two std::map.  The boost 'should' be quicker,
 * but this is another option to try.
 * Estimated speed:
 *   Key to Value O(LogN)
 *   Value to Key O(LogN)
 **/
template <typename Key, typename Value>
class BimapStdMap : public BimapInterface<Key, Value> {
public:
  /** Insert a key/value pair into the bimap */
  void
  insert(const Key& key, const Value& value) override
  {
    myForwardMap[key]   = value;
    myReverseMap[value] = key;
  }

  /** Get a key given a value */
  Key
  getKey(const Value& value) const override
  {
    auto it = myReverseMap.find(value);

    return it != myReverseMap.end() ? it->second : Key{ };
  }

  /** Get a value given a key */
  Value
  getValue(const Key& key) const override
  {
    auto it = myForwardMap.find(key);

    return it != myForwardMap.end() ? it->second : Value{ };
  }

private:
  /** Map forward from key to value */
  std::map<Key, Value> myForwardMap;

  /** Map reverse from value to key */
  std::map<Value, Key> myReverseMap;
};
}
