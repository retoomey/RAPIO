#include <rNamedAny.h>

using namespace rapio;

std::shared_ptr<NamedAnyList>
NamedAnyList::Clone()
{
  std::shared_ptr<NamedAnyList> clonedList = std::make_shared<NamedAnyList>();

  for (const auto& a : myAttributes) {
    const std::string& name = a.getName();
    NamedAny c(name);
    c.set(a.getData());
    clonedList->myAttributes.push_back(std::move(c));
  }
  return clonedList;
};

int
NamedAnyList::index(const std::string& name) const
{
  size_t count = 0;

  for (auto i:myAttributes) {
    if (i.getName() == name) {
      return count; // always +
    }
    count++;
  }
  return -1;
}
