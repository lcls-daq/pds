#include "Daedalus.hh"
#include "Error.hh"

using namespace Pds::NsCam;

Daedalus::Daedalus(std::shared_ptr<Board> board) :
  Sensor(SensorType::DAEDALUS, board, 3)
{}

bool Daedalus::checkSensorVoltStat()
{
  return board_->getSubRegister("DAEDALUS_DET");
}
