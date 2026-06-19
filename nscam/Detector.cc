#include "Detector.hh"
#include "Error.hh"
#include "Logger.hh"

#include <iostream>
#include <iomanip>

using namespace Pds::NsCam;

void Detector::listDevices()
{
  Comm::listDevices();
}

Detector::Detector(const std::string& host,
                   unsigned short port,
                   CommType ctype,
                   BoardType btype,
                   SensorType stype) :
  comm_(Comm::create(host, port, ctype)),
  board_(Board::create(btype, stype, comm_)),
  sensor_(Sensor::create(stype, board_))
{
  initialize();
}

Detector::~Detector()
{}

void Detector::initialize()
{
  board_->initBoard();
  board_->initPots();
  sensor_->initSensor();
  initPowerCheck();
}

void Detector::reinitialize()
{
  LOG_INFO(__func__);
  comm_->reconnect();
  initialize();

  // restore timing settings works for timing set by setTiming, setArbTiming, or self.setManualShutters
  sensor_->restoreCachedTiming();
}

void Detector::reboot()
{
  try {
    board_->softReboot();
  } catch (const CommException& err) {
    // this is not unexcepted just log the error for debug
    LOG_DEBUG(std::string(__func__) + ": " + err.what());
  }
  reinitialize();
}

void Detector::resetTimer()
{
  board_->resetTimer();
}

void Detector::interfaceInfo() const
{
  comm_->info();
}

void Detector::boardInfo() const
{
  board_->info();
}

void Detector::sensorInfo() const
{
  sensor_->info();
}

void Detector::statusInfo() const
{
  // save the i/o formatting before changing...
  std::ios_base::fmtflags fmt(std::cout.flags());
  std::cout << "Env Status:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Temp:             " << getTemp(TempType::C) << " " << toString(TempType::C) << std::endl;
  if (board_->type() != BoardType::LLNL_V1) {
    std::cout << " Pressure:         " << getPressure(PressureType::torr) << " " << toString(PressureType::torr) << std::endl;
  }
  std::cout << std::endl;
  // restore i/o formatting
  std::cout.flags(fmt);

  board_->status();
}

std::string Detector::boardName() const
{
  return board_->name();
}

std::string Detector::sensorName() const
{
  return sensor_->name();
}

uint32_t Detector::boardFpgaNum() const
{
  return board_->fpgaNum();
}

uint32_t Detector::boardFpgaRev() const
{
  return board_->fpgaRev();
}

bool Detector::boardFpgaRad() const
{
  return board_->fpgaRad();
}

bool Detector::powerCheck(uint32_t delta) const
{
  auto cur_time = std::chrono::system_clock::now();
  uint32_t timer = getTimer();

  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(cur_time - inittime_);
  uint32_t difference = std::abs(elapsed.count() - timer);
  LOG_DEBUG(std::string(__func__) + ": elapsed time = " + std::to_string(elapsed.count()) + ", difference = " + std::to_string(difference));

  return difference < delta;
}

uint32_t Detector::getTimer() const
{
  return board_->getTimer();
}

double Detector::getTemp(TempType scale) const
{
  return board_->getTemp(scale);
}

double Detector::getPressure(double offset, double sensitivity, PressureType scale) const
{
  return board_->getPressure(offset, sensitivity, scale);
}

double Detector::getPressure(PressureType scale) const
{
  return board_->getPressure(scale);
}

double Detector::getPressure() const
{
  return board_->getPressure();
}

uint32_t Detector::getRegister(const std::string& regname) const
{
  return board_->getRegister(regname);
}

void Detector::setRegister(const std::string& regname, uint32_t value)
{
  board_->setRegister(regname, value);
}

uint32_t Detector::getSubRegister(const std::string& subregname) const
{
  return board_->getSubRegister(subregname);
}

void Detector::setSubRegister(const std::string& subregname, uint32_t value)
{
  board_->setSubRegister(subregname, value);
}

double Detector::getPot(const std::string& potname) const
{
  return board_->getPot(potname);
}

double Detector::getPotV(const std::string& potname) const
{
  return board_->getPotV(potname);
}

void Detector::setPot(const std::string& potname, double fraction)
{
  board_->setPot(potname, fraction);
}

void Detector::setPotV(const std::string& potname, double voltage, bool tune, double accuracy, int iterations, double approach, bool tuneErr)
{
  board_->setPotV(potname, voltage, tune, accuracy, iterations, approach, tuneErr);
}

double Detector::getMonV(const std::string& potname) const
{
  return board_->getMonV(potname);
}

bool Detector::manualTiming() const
{
  return sensor_->manualTiming();
}

Sequence Detector::getArbTiming(SideType side) const
{
  return sensor_->getArbTiming(side);
}

Timing Detector::getTiming(SideType side) const
{
  return sensor_->getTiming(side);
}

Sequence Detector::getManualTiming(SideType side) const
{
  return sensor_->getManualTiming(side);
}

void Detector::setArbTiming(SideType side, const Sequence& sequence)
{
  return sensor_->setArbTiming(side, sequence);
}

void Detector::setTiming(SideType side, const Timing& timing)
{
  return sensor_->setTiming(side, timing);
}

void Detector::setManualTiming(SideType side, const Sequence& sequence)
{
  return sensor_->setManualTiming(side, sequence);
}

void Detector::initPowerCheck()
{
  LOG_DEBUG(std::string(__func__) + ": resetting timer for power check function");
  inittime_ = std::chrono::system_clock::now();
  resetTimer();
}
