#include "Detector.hh"

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
  sensor_(Sensor::create(stype, btype, comm_))
{
  initialize();
}

Detector::~Detector()
{}

void Detector::initialize()
{
  board_->initBoard();
  //board_->initPots();
  //board_->initSensor();
  //self.initPowerCheck()
  //self.getBoardInfo()
  //self.printBoardInfo()
}

void Detector::reinitialize()
{
  //TODO
}

void Detector::reboot()
{
  //TODO
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
