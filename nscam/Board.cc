#include "Board.hh"
#include "LLNLv1.hh"
#include "LLNLv4.hh"
#include "Error.hh"
#include "Logger.hh"

#include <iostream>
#include <iomanip>
#include <chrono>

using namespace Pds::NsCam;

std::shared_ptr<Board> Board::create(BoardType btype, SensorType stype, std::shared_ptr<Comm> comm)
{
  switch (btype) {
  case BoardType::LLNL_V1:
    return std::make_shared<LLNLv1>(stype, comm);
  case BoardType::LLNL_V4:
    return std::make_shared<LLNLv4>(stype, comm);
  default:
    return std::shared_ptr<Board>{};
  }
}

Board::Board(BoardType btype,
             SensorType stype,
             std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames,
             const std::map<std::string, uint32_t>& defaults,
             const std::map<std::string, std::string>& aliases,
             const std::map<std::string, std::string>& monitors) :
  Device(comm, regnames, subregnames, defaults, aliases, monitors),
  btype_(btype),
  stype_(stype),
  vref_(0.),
  adc5_mult_(0),
  adc5_bipolar_(false),
  armed_(false),
  abort_{false}
{
  fpgaNum_ = getRegister("FPGA_NUM");
  fpgaRev_ = getRegister("FPGA_REV");

  // check that board type is as expected
  if (fpgaNum_ & (0x80000000)) {
    uint32_t type_byte = (fpgaNum_ >> 24) & 0xf;
    if ((type_byte == 1) || (type_byte == 4)) {
      BoardType fpga_btype = type_byte == 1 ? BoardType::LLNL_V1 : BoardType::LLNL_V4;
      if (btype_ != fpga_btype) {
        LOG_EXCEPTION(DeviceError, name(), LOG_STR("Inconsistent board type (fpga, config): " << fpga_btype << ", " << type()));
      }
    } else {
      LOG_EXCEPTION(DeviceError, name(), LOG_STR("Unsupported board type: " << type_byte));
    }
  } else {
    LOG_EXCEPTION(DeviceError, name(), "Unsupported board type: SNLrevC");
  }

  // check if the board is rad tolerant
  if ((fpgaNum_ >> 4) & 1) {
    fpgaRad_ = true;
  } else {
    fpgaRad_ = false;
  }

  // check that sensor type is as expected
  uint32_t sensor_byte = fpgaNum_ & 0xf;
  if ((sensor_byte == 1) || (sensor_byte == 2)) {
    SensorType fpga_stype = sensor_byte == 1 ? (stype_ == SensorType::ICARUS2 ? SensorType::ICARUS2 : SensorType::ICARUS) : SensorType::DAEDALUS;
    if (stype_ != fpga_stype) {
      LOG_EXCEPTION(DeviceError, name(), LOG_STR("Inconsistent sensor type (fpga, config): " << fpga_stype << ", " << stype_));
    }
  } else {
    LOG_EXCEPTION(DeviceError, name(), LOG_STR("Unsupported sensor type: " << sensor_byte));
  }

  // check supported interface types
  uint32_t interface_byte = (fpgaNum_ >> 8) & 0xf;
  if (interface_byte & 1) {
    fpgaInterfaces_.push_back(CommType::RS422);
  }
  if (interface_byte & 2) {
    fpgaInterfaces_.push_back(CommType::GIGE);
  }
}

void Board::info() const
{
  // save the i/o formatting before changing...
  FormatBackup fmt(std::cout);
  std::cout << "Board Info:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Type:             " << name() << std::endl;
  std::cout << std::hex << std::setfill('0') << std::setw(8);
  std::cout << " Number:           " << fpgaNum() << std::endl;
  std::cout << " Revision:         " << fpgaRev() << std::endl;
  std::cout << " Rad-Tolerant:     " << fpgaRad() << std::endl;
  std::cout << " Interfaces:       " << std::endl;
  for (auto ctype : fpgaInterfaces()) {
    std::cout << "   " << toString(ctype);
    if (ctype == commType()) {
      std::cout << " (active)";
    }
    std::cout << std::endl;
  }
  // remove hex formatting
  fmt.restore();
  std::cout << " Vref:             " << vref() << std::endl;
  std::cout << " ADC5 multiplier:  " << adc5_mult() << std::endl;
  std::cout << " ADC5 bipolar:     " << adc5_bipolar() << std::endl;
  std::cout << std::endl;
}

void Board::status() const
{
  uint32_t statusbits = checkStatus();
  uint32_t statusbits2 = checkStatus2();
  // save the i/o formatting before changing...
  FormatBackup fmt(std::cout);
  std::cout << "Board Status:" << std::endl;
  std::cout << "=========================" << std::endl;
  std::cout << " Sensor Read:      " << getBit(statusbits, 0) << std::endl;
  std::cout << " Coarse Trigger:   " << getBit(statusbits, 1) << std::endl;
  std::cout << " Fine Trigger:     " << getBit(statusbits, 2) << std::endl;
  std::cout << " Readout Started:  " << getBit(statusbits, 5) << std::endl;
  std::cout << " Reaout Complete:  " << getBit(statusbits, 6) << std::endl;
  std::cout << " SRAM Started:     " << getBit(statusbits, 7) << std::endl;
  std::cout << " SRAM Complete:    " << getBit(statusbits, 8) << std::endl;
  std::cout << " TimingCfg Active: " << getBit(statusbits, 9) << std::endl;
  std::cout << " ADCs Configured:  " << getBit(statusbits, 10) << std::endl;
  std::cout << " DACs Configured:  " << getBit(statusbits, 11) << std::endl;
  std::cout << " Timer Reset:      " << getBit(statusbits, 13) << std::endl;
  std::cout << " Camera Armed:     " << getBit(statusbits, 14) << std::endl;
  std::cout << " TimingCfg Done:   " << getBit(statusbits, 16) << std::endl;
  std::cout << " FPA_IF_TO:        " << getBit(statusbits2, 0) << std::endl;
  std::cout << " SRAM_RO_TO:       " << getBit(statusbits2, 1) << std::endl;
  std::cout << " PixelRd Timeout:  " << getBit(statusbits2, 2) << std::endl;
  std::cout << " UART_TX_TO_RST:   " << getBit(statusbits2, 3) << std::endl;
  std::cout << " UART_RX_TO_RST:   " << getBit(statusbits2, 4) << std::endl;
  std::cout << std::endl;
}

uint32_t Board::fpgaNum() const
{
  return fpgaNum_;
}

uint32_t Board::fpgaRev() const
{
  return fpgaRev_;
}

bool Board::fpgaRad() const
{
  return fpgaRad_;
}

const std::vector<CommType>& Board::fpgaInterfaces() const
{
  return fpgaInterfaces_;
}

BoardType Board::type() const
{
  return btype_;
}

std::string Board::name() const
{
  return toString(btype_);
}

double Board::vref() const
{
  return vref_;
}

uint32_t Board::adc5_mult() const
{
  return adc5_mult_;
}

bool Board::adc5_bipolar() const
{
  return adc5_bipolar_;
}

uint32_t Board::getTimer() const
{
  LOG_DEBUG << __func__;
  return getRegister("TIMER_VALUE");
}

void Board::resetTimer()
{
  LOG_DEBUG << __func__;
  setSubRegister("RESET_TIMER", 1);
  setSubRegister("RESET_TIMER", 0);
}

double Board::getPressure(double offset, double sensitivity, PressureType scale) const
{
  LOG_WARN << __func__ << " is not implement on the " << name() << " board.";
  return 0.0;
}

double Board::getPressure(PressureType scale) const
{
  LOG_WARN << __func__ << " is not implement on the " << name() << " board.";
  return 0.0;
}

double Board::getPressure() const
{
  LOG_WARN << __func__ << " is not implement on the " << name() << " board.";
  return 0.0;
}

void Board::clearStatus()
{
  LOG_DEBUG << __func__;
  getRegister("STAT_REG_SRC");
  getRegister("STAT_REG2_SRC");
}

uint32_t Board::checkStatus() const
{
  LOG_DEBUG << __func__;
  return getRegister("STAT_REG");
}

uint32_t Board::checkStatus2() const
{
  LOG_DEBUG << __func__;
  return getRegister("STAT_REG2");
}

bool Board::armed() const
{
  return armed_;
}

void Board::arm(TriggerType mode)
{
  LOG_DEBUG << __func__ << " mode = " << mode;
  clearStatus();
  latchPots();
  startCapture(mode);
  armed_ = true;
}

void Board::disarm()
{
  LOG_DEBUG << __func__;
  clearStatus();
  armed_ = false;
  setSubRegister("HW_TRIG_EN", 0);
  setSubRegister("SW_TRIG_EN", 0);
}

void Board::startCapture(TriggerType mode)
{
  LOG_DEBUG << __func__;
  setRegister("ADC_CTL", 0x0000001F);
  if (mode == TriggerType::SOFTWARE) {
    setSubRegister("HW_TRIG_EN", 0);
    setSubRegister("SW_TRIG_EN", 1);
    setSubRegister("SW_TRIG_START", 1);
  } else {
    setSubRegister("SW_TRIG_EN", 0);
    setSubRegister("HW_TRIG_EN", 1);
  }
}

bool Board::waitForSRAM(uint32_t timeout_ms)
{
  LOG_DEBUG << __func__;
  bool waiting = true;
  bool ready = false;
  auto start = std::chrono::system_clock::now();

  while (waiting) {
    uint32_t sram = getSubRegister("SRAM_READY");
    if (sram) {
      waiting = false;
      ready = true;
      LOG_DEBUG << __func__ << " SRAM ready";
    }

    // check if acquisition has been aborted
    if (abort_.exchange(false)) {
      waiting = false;
      LOG_DEBUG << __func__ << " readoff aborted by user";
    }

    // if timeout is enabled then check
    if (!ready && (timeout_ms > 0)) {
      auto current = std::chrono::system_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start);
      if (elapsed.count() > timeout_ms) {
        waiting = false;
        LOG_ERROR << __func__ << " SRAM timeout";
      }
    }
  }

  return ready;
}

bool Board::abortReadoff(bool flag)
{
  LOG_DEBUG << __func__;
  abort_.store(flag);
  return flag;
}

double Board::convertMonV(double fraction) const
{
  if (adc5_bipolar_) {
    if (fraction >= 0.5) {
      fraction -= 1.;
    }
    return 2 * adc5_mult_ * fraction * vref_;
  } else {
    return adc5_mult_ * fraction * vref_;
  }
}
