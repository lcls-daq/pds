#include "Sensor.hh"
#include "Daedalus.hh"
#include "Icarus.hh"
#include "Icarus2.hh"
#include "Error.hh"

#include <iostream>
#include <iomanip>

using namespace Pds::NsCam;

std::shared_ptr<Sensor> Sensor::create(SensorType stype, BoardType btype, std::shared_ptr<Comm> comm)
{
  switch (stype) {
  case SensorType::DAEDALUS:
    return std::make_shared<Daedalus>(btype, comm);
  case SensorType::ICARUS:
    return std::make_shared<Icarus>(btype, comm);
  case SensorType::ICARUS2:
    return std::make_shared<Icarus2>(btype, comm);
  default:
    return std::shared_ptr<Sensor>{};
  }
}

Sensor::Sensor(SensorType stype,
               BoardType btype,
               std::shared_ptr<Comm> comm,
               const std::map<std::string, uint16_t>& regnames,
               const std::map<std::string, SubRegister>& subregnames,
               const std::map<std::string, uint32_t>& defaults) :
  Device(comm, regnames, subregnames, defaults),
  stype_(stype),
  btype_(btype)
{}

void Sensor::info() const
{
  // save the i/o formatting before changing...
  std::ios_base::fmtflags fmt(std::cout.flags());
  std::cout << "Sensor Info:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Type:             " << name() << std::endl;
  std::cout << std::endl;
  // restore i/o formatting
  std::cout.flags(fmt);
}

SensorType Sensor::type() const
{
  return stype_;
}

std::string Sensor::name() const
{
  return toString(stype_);
}

