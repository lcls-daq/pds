#include "Daedalus.hh"
#include "Error.hh"

using namespace Pds::NsCam;

Daedalus::Daedalus(std::shared_ptr<Board> board) :
  Sensor(SensorType::DAEDALUS, board, 3, 0, 2, 512, 1024, 2)
{}

bool Daedalus::checkSensorVoltStat()
{
  return board_->getSubRegister("DAEDALUS_DET");
}
