#include "Detector.hh"
#include "Error.hh"
#include "Logger.hh"

#include <iostream>

using namespace Pds::NsCam;

void Detector::listDevices()
{
  Comm::listDevices();
}

Detector::Detector(const std::string& host,
                   unsigned short port,
                   CommType ctype,
                   BoardType btype,
                   SensorType stype,
                   bool init) :
  initialized_(false),
  comm_(Comm::create(host, port, ctype)),
  board_(Board::create(btype, stype, comm_)),
  sensor_(Sensor::create(stype, board_))
{
  if (init) {
    initialize();
  }
}

Detector::~Detector()
{}

bool Detector::isInitialized() const
{
  return initialized_;
}

void Detector::initialize()
{
  board_->initBoard();
  board_->initPots();
  sensor_->initSensor();
  initPowerCheck();
  initialized_ = true;
}

void Detector::reinitialize()
{
  LOG_INFO << __func__;
  initialized_ = false;
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
    LOG_DEBUG << __func__ << " " << err.what();
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
  FormatBackup fmt(std::cout);
  std::cout << INFO_HEADER("Env Status", 1) << std::endl;
  std::cout << INFO_PAD("Temp") << getTemp(TempType::C) << " " << toString(TempType::C) << std::endl;
  if (board_->type() != BoardType::LLNL_V1) {
    std::cout << INFO_PAD("Pressure") << getPressure(PressureType::torr) << " " << toString(PressureType::torr) << std::endl;
  }
  std::cout << std::endl;
  // restore i/o formatting
  fmt.restore();

  board_->status();
}

void Detector::potInfo() const
{
  board_->potInfo();
}

BoardType Detector::boardType() const
{
  return board_->type();
}

SensorType Detector::sensorType() const
{
  return sensor_->type();
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
  LOG_DEBUG << __func__ << " elapsed time = " << elapsed.count() << ", difference = " << difference;

  return difference < delta;
}

uint32_t Detector::getTimer() const
{
  return board_->getTimer();
}

std::string Detector::host() const
{
  return comm_->host();
}

unsigned short Detector::port() const
{
  return comm_->port();
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

uint32_t Detector::minframe() const
{
  return sensor_->minframe();
}

uint32_t Detector::maxframe() const
{
  return sensor_->maxframe();
}

uint32_t Detector::maxwidth() const
{
  return sensor_->maxwidth();
}

uint32_t Detector::maxheight() const
{
  return sensor_->maxheight();
}

uint32_t Detector::bytesperpixel() const
{
  return sensor_->bytesperpixel();
}

uint32_t Detector::nframes() const
{
  return sensor_->nframes();
}

uint32_t Detector::firstframe() const
{
  return sensor_->firstframe();
}

uint32_t Detector::lastframe() const
{
  return sensor_->lastframe();
}

uint32_t Detector::firstrow() const
{
  return sensor_->firstrow();
}

uint32_t Detector::lastrow() const
{
  return sensor_->lastrow();
}
size_t Detector::width() const
{
  return sensor_->width();
}

size_t Detector::height() const
{
  return sensor_->height();
}

size_t Detector::npixels() const
{
  return sensor_->npixels();
}

size_t Detector::payloadSize() const
{
  return sensor_->payloadSize();
}

void Detector::setRows()
{
  sensor_->setRows();
}

void Detector::setRows(uint32_t minrow, uint32_t maxrow)
{
  sensor_->setRows(minrow, maxrow);
}

void Detector::setFrames()
{
  sensor_->setFrames();
}

void Detector::setFrames(uint32_t minframe, uint32_t maxframe)
{
  sensor_->setFrames(minframe, maxframe);
}

bool Detector::validRegister(const std::string& regname) const
{
  return board_->validRegister(regname);
}

bool Detector::validSubRegister(const std::string& subregname) const
{
  return board_->validSubRegister(subregname);
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

Sequence Detector::getActualTiming(SideType side) const
{
  return sensor_->getActualTiming(side);
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

void Detector::setOscillator(OscillatorType osc)
{
  sensor_->setOscillator(osc);
}

OscillatorType Detector::getOscillator() const
{
  return sensor_->getOscillator();
}

void Detector::setInterlacing(SideType side)
{
  return sensor_->setInterlacing(side);
}

void Detector::setInterlacing(uint32_t ifactor, SideType side)
{
  return sensor_->setInterlacing(ifactor, side);
}

uint32_t Detector::getInterlacing(SideType side) const
{
  return sensor_->getInterlacing(side);
}

void Detector::setHighFullWell(bool flag)
{
  return sensor_->setHighFullWell(flag);
}

bool Detector::getHighFullWell() const
{
  return sensor_->getHighFullWell();
}

void Detector::setZeroDeadTime(bool flag, SideType side)
{
  return sensor_->setZeroDeadTime(flag, side);
}

bool Detector::getZeroDeadTime(SideType side) const
{
  return sensor_->getZeroDeadTime(side);
}

void Detector::setTriggerDelay()
{
  return sensor_->setTriggerDelay();
}

void Detector::setTriggerDelay(double delay)
{
  return sensor_->setTriggerDelay(delay);
}

double Detector::getTriggerDelay() const
{
  return sensor_->getTriggerDelay();
}

void Detector::setPhiDelay(SideType side)
{
  return sensor_->setPhiDelay(side);
}

void Detector::setPhiDelay(double delay, SideType side)
{
  return sensor_->setPhiDelay(delay, side);
}

double Detector::getPhiDelay(SideType side)
{
  return sensor_->getPhiDelay(side);
}

void Detector::setExtClk()
{
  return sensor_->setExtClk();
}

void Detector::setExtClk(uint32_t dilation, double frequency)
{
  return sensor_->setExtClk(dilation, frequency);
}

bool Detector::armed() const
{
  return board_->armed();
}

void Detector::arm(TriggerType mode)
{
  board_->arm(mode, sensor_->manualTiming());
}

void Detector::disarm()
{
  board_->disarm();
}

void Detector::startCapture(TriggerType mode)
{
  board_->startCapture(mode, sensor_->manualTiming());
}

bool Detector::waitForSRAM(uint32_t timeout_ms)
{
  return board_->waitForSRAM(timeout_ms);
}

std::unique_ptr<uint8_t[]> Detector::readFrame8()
{
  return sensor_->readFrame8();
}

std::unique_ptr<uint16_t[]> Detector::readFrame16()
{
  return sensor_->readFrame16();
}

std::unique_ptr<uint32_t[]> Detector::readFrame32()
{
  return sensor_->readFrame32();
}

std::unique_ptr<uint8_t[]> Detector::waitFrame8(TriggerType mode, uint32_t timeout_ms)
{
  arm(mode);
  if (waitForSRAM(timeout_ms)) {
    return readFrame8();
  } else {
    return {};
  }
}

std::unique_ptr<uint16_t[]> Detector::waitFrame16(TriggerType mode, uint32_t timeout_ms)
{
  arm(mode);
  if (waitForSRAM(timeout_ms)) {
    return readFrame16();
  } else {
    return {};
  }
}

std::unique_ptr<uint32_t[]> Detector::waitFrame32(TriggerType mode, uint32_t timeout_ms)
{
  arm(mode);
  if (waitForSRAM(timeout_ms)) {
    return readFrame32();
  } else {
    return {};
  }
}

bool Detector::abortReadoff(bool flag)
{
  return board_->abortReadoff(flag);
}

void Detector::initPowerCheck()
{
  LOG_DEBUG << __func__ << " resetting timer for power check function";
  inittime_ = std::chrono::system_clock::now();
  resetTimer();
}
