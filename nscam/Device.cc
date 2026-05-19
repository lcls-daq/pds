#include "Device.hh"
#include "Comm.hh"
#include "Error.hh"

using namespace Pds::NsCam;

SubRegister::SubRegister(std::string r,
                         uint32_t sb,
                         uint32_t w,
                         bool wr) :
  regname(std::move(r)),
  start_bit(sb),
  width(w),
  writable(wr),
  minV(0),
  maxV(5)
{}

/*SubRegister::SubRegister(SubRegister& other) :
  regname(other.regname),
  start_bit(other.start_bit),
  width(other.width),
  writable(other.writable),
  minV(other.minV),
  maxV(other.maxV)
{}*/

uint32_t SubRegister::maxValue() const
{
  uint64_t value = 1;
  return ((value<<width) - 1);
}

double SubRegister::resolution() const
{
  return ((double) (maxV - minV)) / maxValue();
}

Device::Device(std::shared_ptr<Comm> comm,
               const std::map<std::string, uint16_t>& regnames,
               const std::map<std::string, SubRegister>& subregnames,
               const std::map<std::string, uint32_t>& defaults) :
  regnames_(regnames),
  subregnames_(subregnames),
  defaults_(defaults),
  comm_(comm)
{}

Device::Device(std::shared_ptr<Comm> comm,
               const std::map<std::string, uint16_t>& regnames,
               const std::map<std::string, SubRegister>& subregnames,
               const std::map<std::string, uint32_t>& defaults,
               const std::map<std::string, std::string>& aliases,
               const std::map<std::string, std::string>& monitors) :
  regnames_(regnames),
  subregnames_(subregnames),
  defaults_(defaults),
  aliases_(aliases),
  monitors_(monitors),
  comm_(comm)
{}

uint32_t Device::getRegister(const std::string& regname)
{
  return Device::getRegister(lookupRegister(regname));
}

uint32_t Device::getRegister(uint16_t address)
{
  return comm_->sendCmd(1, address);
}

void Device::setRegister(const std::string& regname, uint32_t value)
{
  setRegister(lookupRegister(regname), value);
}

void Device::setRegister(uint16_t address, uint32_t value)
{
  comm_->sendCmd(0, address, value);
}

uint32_t Device::getSubRegister(const std::string& subregname)
{
  return getSubRegister(lookupSubRegister(subregname));
}

uint32_t Device::getSubRegister(const SubRegister& subreg)
{
  uint32_t regval = getRegister(subreg.regname);
  return (regval >> subreg.start_bit) & subreg.maxValue();
}

void Device::setSubRegister(const std::string& subregname, uint32_t value)
{
  setSubRegister(lookupSubRegister(subregname), value);
}

void Device::setSubRegister(const SubRegister& subreg, uint32_t value)
{
  if (subreg.writable) {
    uint32_t regval = getRegister(subreg.regname);
    regval &= (~(subreg.maxValue() << subreg.start_bit));
    regval |= ((value & subreg.maxValue()) << subreg.start_bit);
    setRegister(subreg.regname, regval);
  } else {
    throw ReadOnlyRegister(subreg.regname);
  }
}

CommType Device::commType() const
{
  return comm_->type();
}

std::string Device::commName() const
{
  return comm_->name();
}

void Device::addRegister(const std::string& regname, uint16_t address)
{
  regnames_.emplace(regname, address);
}

void Device::addSubRegister(const std::string& regname, const SubRegister& subreg)
{
  subregnames_.emplace(regname, subreg);
}

void Device::addDefault(const std::string& regname, uint32_t value)
{
  defaults_.emplace(regname, value);
}

uint16_t Device::lookupRegister(const std::string& regname) const
{
  auto alias = aliases_.find(regname);
  auto it = regnames_.find(alias == aliases_.end() ? regname : alias->second);
  if (it == regnames_.end()) {
    throw InvalidRegister(regname);
  }

  return it->second;
}

SubRegister& Device::lookupSubRegister(const std::string& subregname)
{
  auto alias = aliases_.find(subregname);
  auto it = subregnames_.find(alias == aliases_.end() ? subregname : alias->second);
  if (it == subregnames_.end()) {
    throw InvalidRegister(subregname);
  }

  return it->second;
}
