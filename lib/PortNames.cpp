#include "PortNames.h"
#include <map>

using namespace axe;

struct PortNames {
  std::map<uint32_t, std::string> portNames;
  std::map<std::string, uint32_t> portIds;
  void addPort(const std::string &name, uint32_t id) {
    portNames.insert(std::make_pair(id, name));
    portIds.insert(std::make_pair(name, id));
  }
  PortNames();
  bool getPortName(uint32_t id, std::string &name) {
    auto it = portNames.find(id);
    if (it == portNames.end()) {
      return false;
    }
    name = it->second;
    return true;
  }
  bool getPortId(const std::string &name, uint32_t &id) {
    auto it = portIds.find(name);
    if (it == portIds.end()) {
      return false;
    }
    id = it->second;
    return true;
  }
};

PortNames::PortNames()
{
  addPort("PORT_32A", 0x200000);
  addPort("PORT_32B", 0x200100);

  addPort("PORT_16A", 0x100000);
  addPort("PORT_16B", 0x100100);
  addPort("PORT_16C", 0x100200);
  addPort("PORT_16D", 0x100300);

  addPort("PORT_8A", 0x80000);
  addPort("PORT_8B", 0x80100);
  addPort("PORT_8C", 0x80200);
  addPort("PORT_8D", 0x80300);

  addPort("PORT_4A", 0x40000);
  addPort("PORT_4B", 0x40100);
  addPort("PORT_4C", 0x40200);
  addPort("PORT_4D", 0x40300);
  addPort("PORT_4E", 0x40400);
  addPort("PORT_4F", 0x40500);

  addPort("PORT_1A", 0x10200);
  addPort("PORT_1B", 0x10000);
  addPort("PORT_1C", 0x10100);
  addPort("PORT_1D", 0x10300);
  addPort("PORT_1E", 0x10600);
  addPort("PORT_1F", 0x10400);
  addPort("PORT_1G", 0x10500);
  addPort("PORT_1H", 0x10700);
  addPort("PORT_1I", 0x10a00);
  addPort("PORT_1J", 0x10800);
  addPort("PORT_1K", 0x10900);
  addPort("PORT_1L", 0x10b00);
  addPort("PORT_1M", 0x10c00);
  addPort("PORT_1N", 0x10d00);
  addPort("PORT_1O", 0x10e00);
  addPort("PORT_1P", 0x10f00);
}

static PortNames portNames;

bool axe::getPortName(uint32_t id, std::string &name)
{
  return portNames.getPortName(id, name);
}

bool axe::getPortId(const std::string &name, uint32_t &id)
{
  return portNames.getPortId(name, id);
}
