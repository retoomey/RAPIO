#include "rVelResolver.h"

using namespace rapio;

void
VelResolver::introduceSelf()
{
  std::shared_ptr<VelResolver> newOne = std::make_shared<VelResolver>();
  Factory<VolumeValueResolver>::introduce("vel", newOne);
}

std::string
VelResolver::getHelpString(const std::string& fkey)
{
  return "Experimental velocity gatherer.";
}

std::shared_ptr<VolumeValueResolver>
VelResolver::create(const std::string & params)
{
  return std::make_shared<VelResolver>();
}

void
VelResolver::calc(VolumeValue * vvp)
{
  auto& vv = *(VolumeValueVelGatherer *) (vvp);

  bool haveLower = queryLower(vv);
  bool haveUpper = queryUpper(vv);

  // Will we need any other tilts?  I'm thinking not for velocity
  // bool haveLLower = query2ndLower(vv);
  // bool haveUUpper = query2ndUpper(vv);

  // We're going to try doing a cressmen in the 2D of the radial sets.
  // Fun fun fun.
  vv.dataValue = 2;  // root class and stage1 'grid' output.  What to show here, if any?
  vv.test      = 10; // FIXME: Our custom outputs will go here
} // calc
