#include "Board.hh"
#include "LLNLv1.hh"
#include "LLNLv4.hh"
#include "Error.hh"

#include <iostream>
#include <iomanip>

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
             const std::map<std::string, uint32_t>& defaults) :
  Device(comm, regnames, subregnames, defaults),
  btype_(btype),
  stype_(stype),
  vref_(0.),
  adc5_mult_(0)
{
  fpgaNum_ = getRegister("FPGA_NUM");
  fpgaRev_ = getRegister("FPGA_REV");

  // check that board type is as expected
  if (fpgaNum_ & (0x80000000)) {
    uint32_t type_byte = (fpgaNum_ >> 24) & 0xf;
    if ((type_byte == 1) || (type_byte == 4)) {
      BoardType fpga_btype = type_byte == 1 ? BoardType::LLNL_V1 : BoardType::LLNL_V4;
      if (btype_ != fpga_btype) {
        throw BoardError("Inconsistent board type (fpga, config): " + std::string(toString(fpga_btype)) + ", " + name());
      }
    } else {
      throw BoardError("Unsupported board type: " + std::to_string(type_byte));
    }
  } else {
    throw BoardError("Unsupported board type: SNLrevC");
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
      throw BoardError("Inconsistent sensor type (fpga, config): " + std::string(toString(fpga_stype)) + ", " + std::string(toString(stype_)));
    }
  } else {
    throw BoardError("Unsupported sensor type: " + std::to_string(sensor_byte));
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
  std::ios_base::fmtflags fmt(std::cout.flags());
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
  std::cout.flags(fmt);
  std::cout << " Vref:             " << vref() << std::endl;
  std::cout << " ADC5 multiplier:  " << adc5_mult() << std::endl;
  std::cout << std::endl;
  // restore i/o formatting
  std::cout.flags(fmt);
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
