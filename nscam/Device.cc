#include "Device.hh"
#include "Comm.hh"
#include "Error.hh"
#include "Logger.hh"

#include <cmath>
#include <iostream>
#include <iomanip>

static void msleep(long ms)
{
  timespec sleep_time = {ms / 1000, (ms % 1000) * 1000000};
  nanosleep(&sleep_time, NULL);
}

using namespace Pds::NsCam;

SubRegister::SubRegister(std::string r,
                         uint32_t sb,
                         uint32_t w,
                         bool wr) :
  regname(std::move(r)),
  start_bit(sb),
  width(w),
  writable(wr),
  minV(0.),
  maxV(5.)
{}

SubRegister::SubRegister(std::string r,
                         uint32_t sb,
                         uint32_t w,
                         bool wr,
                         double minv,
                         double maxv) :
  regname(std::move(r)),
  start_bit(sb),
  width(w),
  writable(wr),
  minV(minv),
  maxV(maxv)
{}

uint32_t SubRegister::minValue() const
{
  return 0;
}

uint32_t SubRegister::maxValue() const
{
  uint64_t value = 1;
  return ((value<<width) - 1);
}

double SubRegister::resolution() const
{
  return (maxV - minV) / maxValue();
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

uint32_t Device::getRegister(const std::string& regname) const
{
  return Device::getRegister(lookupRegister(regname));
}

uint32_t Device::getRegister(uint16_t address) const
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

uint32_t Device::getSubRegister(const std::string& subregname) const
{
  return getSubRegister(lookupSubRegister(subregname));
}

uint32_t Device::getSubRegister(const SubRegister& subreg) const
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

std::unique_ptr<uint8_t[]> Device::getDataRegisterAsUInt8(const std::string& regname, uint32_t value, size_t len)
{
  return getDataRegisterAsUInt8(lookupRegister(regname), value, len);
}

std::unique_ptr<uint8_t[]> Device::getDataRegisterAsUInt8(uint16_t address, uint32_t value, size_t len)
{
  return comm_->readDataAsUInt8(address, value, len);
}

std::unique_ptr<uint8_t[]> Device::getDataSubRegisterAsUInt8(const std::string& subregname, uint32_t value, size_t len)
{
  return getDataSubRegisterAsUInt8(lookupSubRegister(subregname), value, len);
}

std::unique_ptr<uint8_t[]> Device::getDataSubRegisterAsUInt8(const SubRegister& subreg, uint32_t value, size_t len)
{
  if (subreg.writable) {
    uint32_t regval = getRegister(subreg.regname);
    regval &= (~(subreg.maxValue() << subreg.start_bit));
    regval |= ((value & subreg.maxValue()) << subreg.start_bit);
    return getDataRegisterAsUInt8(subreg.regname, regval, len);
  } else {
    throw ReadOnlyRegister(subreg.regname);
  }
}

std::unique_ptr<uint16_t[]> Device::getDataRegisterAsUInt16(const std::string& regname, uint32_t value, size_t len)
{
  return getDataRegisterAsUInt16(lookupRegister(regname), value, len);
}

std::unique_ptr<uint16_t[]> Device::getDataRegisterAsUInt16(uint16_t address, uint32_t value, size_t len)
{
  return comm_->readDataAsUInt16(address, value, len);
}

std::unique_ptr<uint16_t[]> Device::getDataSubRegisterAsUInt16(const std::string& subregname, uint32_t value, size_t len)
{
  return getDataSubRegisterAsUInt16(lookupSubRegister(subregname), value, len);
}

std::unique_ptr<uint16_t[]> Device::getDataSubRegisterAsUInt16(const SubRegister& subreg, uint32_t value, size_t len)
{
  if (subreg.writable) {
    uint32_t regval = getRegister(subreg.regname);
    regval &= (~(subreg.maxValue() << subreg.start_bit));
    regval |= ((value & subreg.maxValue()) << subreg.start_bit);
    return getDataRegisterAsUInt16(subreg.regname, regval, len);
  } else {
    throw ReadOnlyRegister(subreg.regname);
  }
}

std::unique_ptr<uint32_t[]> Device::getDataRegisterAsUInt32(const std::string& regname, uint32_t value, size_t len)
{
  return getDataRegisterAsUInt32(lookupRegister(regname), value, len);
}

std::unique_ptr<uint32_t[]> Device::getDataRegisterAsUInt32(uint16_t address, uint32_t value, size_t len)
{
  return comm_->readDataAsUInt32(address, value, len);
}

std::unique_ptr<uint32_t[]> Device::getDataSubRegisterAsUInt32(const std::string& subregname, uint32_t value, size_t len)
{
  return getDataSubRegisterAsUInt32(lookupSubRegister(subregname), value, len);
}

std::unique_ptr<uint32_t[]> Device::getDataSubRegisterAsUInt32(const SubRegister& subreg, uint32_t value, size_t len)
{
  if (subreg.writable) {
    uint32_t regval = getRegister(subreg.regname);
    regval &= (~(subreg.maxValue() << subreg.start_bit));
    regval |= ((value & subreg.maxValue()) << subreg.start_bit);
    return getDataRegisterAsUInt32(subreg.regname, regval, len);
  } else {
    throw ReadOnlyRegister(subreg.regname);
  }
}

double Device::getPot(const std::string& potname) const
{
  const SubRegister& subreg = lookupSubRegister(potname);
  uint32_t potval = getSubRegister(subreg);
  return ((double) (potval - subreg.minValue())) / (subreg.maxValue() - subreg.minValue());
}

double Device::getPotV(const std::string& potname) const
{
  const SubRegister& subreg = lookupSubRegister(potname);
  double fraction = getPot(potname);
  return fraction * (subreg.maxV - subreg.minV);
}

void Device::setPot(const std::string& potname, double fraction)
{
  std::string name = resolveRegisterName(potname);
  const SubRegister& subreg = lookupSubRegister(potname);
  if (!subreg.writable) {
    throw ReadOnlyRegister(subreg.regname);
  }

  // this keeps compatablity with python nscamera
  if (fraction < 0.) {
    fraction = 0.0;
  }
  if (fraction > 1.) {
    fraction = 1.0;
  }

  uint32_t potval = std::round(fraction * subreg.maxValue());
  setSubRegister(subreg, potval);

  if (name.rfind("POT", 0) == 0) {
    uint32_t potnumlatch = std::stoi(name.substr(3)) * 2 + 1;
    setRegister("POT_CTL", potnumlatch);
  } else if ((name.rfind("DAC", 0) == 0) && (name.length() == 4)) {
    uint32_t potnumlatch = int(name[3] - 'A') * 2 + 1;
    setRegister("DAC_CTL", potnumlatch);
  } else {
    LOG_EXCEPTION(PotError, name, "is not a valid pot/dac");
  }
}

void Device::setPotV(const std::string& potname, double voltage, bool tune, double accuracy, int iterations, double approach, bool tuneErr)
{
  const SubRegister& subreg = lookupSubRegister(potname);

  // this keeps compatablity with python nscamera
  if (voltage < subreg.minV) {
    voltage = subreg.minV;
  }
  if (voltage > subreg.maxV) {
    voltage = subreg.maxV;
  }
  double fraction = (voltage - subreg.minV) / (subreg.maxV - subreg.minV);
  setPot(potname, fraction);

  msleep(100);

  if (tune) {
    if (!hasMonitor(potname)) {
      LOG_EXCEPTION(PotError, potname, "does not have a monitor - cannot tune");
    }

    setPot(potname, 0.65);
    msleep(200);
    double mon65 = getMonV(potname);
    setPot(potname, 0.35);
    msleep(200);
    double mon35 = getMonV(potname);

    // theoretical voltage range assuming linearity
    double potrange = (mon65 - mon35) / 0.3;
    double stepsize = potrange / (subreg.maxValue() + 1);

    if (potrange < 1.) {
      // restore the nominal value
      setPot(potname, fraction);
      if (tuneErr) {
        LOG_EXCEPTION(PotError, potname, LOG_STR("monitor shows insufficient change with pot variation: " << potrange));
      } else {
        LOG_WARN << potname << " monitor shows insufficient change with pot variation: " << potrange;
      }
      return;
    }

    double potzero = 0.35 - (mon35 / potrange);
    double potone = 1.65 - (mon65 / potrange);
    if (potzero < 0.) {
      potzero = 0.;
    }
    if (potone > 1.) {
      potone = 1.;
    }

    double mindiff = accuracy > stepsize ? accuracy : stepsize;
    double tuned = potzero + (voltage / potone);
    setPot(potname, tuned);

    int smalladjust = 0;
    double lastdiff = 0.0;
    for (int i=0; i<iterations; i++) {
      double measured = getMonV(potname);
      double diff = voltage - measured;
      if (std::abs(diff - lastdiff) < (stepsize / 2)) {
        if (smalladjust > 12) {
          // magic number for now; if it doesn't converge after several
          //   tries, it never will, usually because the setting is pinned
          //   to 0 or 1 and adjust can't change it
          break;
        }
        smalladjust++;
      }
      if (std::abs(diff * 2) < stepsize) {
        break;
      }
      double adjust = approach * (diff / potrange);
      tuned += adjust;
      if (tuned > 1.) {
        tuned = 1.;
      } else if (tuned < 0.) {
        tuned = 0.;
      }
      setPot(potname, tuned);
      lastdiff = diff;
      msleep(200);
    }
    double measured = getMonV(potname);
    double diff = voltage - measured;
    if (diff > mindiff) {
      if (tuneErr) {
        LOG_EXCEPTION(PotError, potname, LOG_STR("tuning failed: diff (" << diff << ") is greater than mindiff (" << mindiff << ")"));
      } else {
        LOG_WARN << potname << " tuning failed: diff (" << diff << ") is greater than mindiff (" << mindiff << ")";
      }
    }
  }
}

double Device::getMonV(const std::string& potname) const
{
  return convertMonV(getPot(lookupMonitor(potname)));
}

void Device::potInfo() const
{
  // save the i/o formatting before changing...
  FormatBackup fmt(std::cout);
  std::cout << "Pot Status:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << std::fixed << std::setprecision(3);
  for (const auto& kv : aliases_) {
    if ((kv.second.rfind("POT", 0) == 0) || (kv.second.rfind("DAC", 0) == 0)) {
      std::cout << " " << std::left << std::setw(18) << kv.first + ":" << getPotV(kv.first) << " V";
      if (hasMonitor(kv.first)) {
        std::cout << ", mon = " << getMonV(kv.first) << " V" << std::endl;
      } else {
        std::cout << std::endl;
      }
    }
  }
  std::cout << std::endl;
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

double Device::convertMonV(double fraction) const
{
  return fraction;
}

void Device::initDefaults()
{
  for (const auto& kv : defaults_) {
    setRegister(kv.first, kv.second);
  }
}

std::string Device::resolveRegisterName(const std::string& regname) const
{
  auto alias = aliases_.find(regname);
  return alias == aliases_.end() ? regname : alias->second;
}

uint16_t Device::lookupRegister(const std::string& regname) const
{
  auto it = regnames_.find(resolveRegisterName(regname));
  if (it == regnames_.end()) {
    throw InvalidRegister(regname);
  }

  return it->second;
}

const SubRegister& Device::lookupSubRegister(const std::string& subregname) const
{
  auto it = subregnames_.find(resolveRegisterName(subregname));
  if (it == subregnames_.end()) {
    throw InvalidRegister(subregname);
  }

  return it->second;
}

std::string Device::lookupMonitor(const std::string& potname) const
{
  auto it = monitors_.find(resolveRegisterName(potname));
  if (it == monitors_.end()) {
    // check if a raw monitor was passed instead of a potname
    if (potname.rfind("MON_", 0) == 0) {
      return potname;
    } else {
      throw InvalidRegister(potname);
    }
  }

  return it->second;
}

bool Device::hasMonitor(const std::string& potname) const
{
  auto it = monitors_.find(resolveRegisterName(potname));
  return it != monitors_.end();
}

uint32_t Device::getBit(uint32_t regval, uint32_t bit)
{
  if (bit < 32) {
    return (regval >> bit) & 0x1;
  } else {
    return 0x0;
  }
}

uint64_t Device::combineRegisters(uint32_t lowregval, uint32_t highregval)
{
  return (((uint64_t) highregval)<<32) | lowregval;
}
