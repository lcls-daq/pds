#include "Board.hh"
#include "LLNLv1.hh"
#include "LLNLv4.hh"

#include <iostream>
#include <iomanip>

using namespace Pds::NsCam;

std::shared_ptr<Board> Board::create(BoardType btype, std::shared_ptr<Comm> comm)
{
  switch (btype) {
  case BoardType::LLNL_V1:
    return std::make_shared<LLNLv1>(comm);
  case BoardType::LLNL_V4:
    return std::make_shared<LLNLv4>(comm);
  default:
    return std::shared_ptr<Board>{};
  }
}

Board::Board(BoardType btype,
             std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames) :
  Device(comm, regnames, subregnames),
  btype_(btype),
  vref_(0.),
  adc5_mult_(0)
{
  fpgaNum_ = getRegister("FPGA_NUM");
  fpgaRev_ = getRegister("FPGA_REV"); 
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
  // remove hex formatting
  std::cout.flags(fmt);
  std::cout << " Vref:             " << vref() << std::endl;
  std::cout << " ADC5 multiplier:  " << adc5_mult() << std::endl;
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
