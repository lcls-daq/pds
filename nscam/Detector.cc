#include "Detector.hh"

using namespace Pds::NsCam;

void Detector::listDevices()
{
  Comm::listDevices();
}

Detector::Detector(const std::string& host,
                   unsigned short port,
                   CommType ctype,
                   BoardType btype) :
  comm_(Comm::create(host, port, ctype)),
  board_(Board::create(btype, comm_))
{
  board_->initBoard();
  //board_->initPots();
  //board_->initSensor();
  //self.initPowerCheck()
  //self.getBoardInfo()
  //self.printBoardInfo()
}

Detector::~Detector()
{}

void Detector::interfaceInfo() const
{
  comm_->info();
}

void Detector::boardInfo() const
{
  board_->info();
}

std::string Detector::boardName() const
{
  return board_->name();
}

uint32_t Detector::boardFpgaNum() const
{
  return board_->fpgaNum();
}

uint32_t Detector::boardFpgaRev() const
{
  return board_->fpgaRev();
}
