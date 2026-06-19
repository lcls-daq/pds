#include "Icarus.hh"
#include "Error.hh"

using namespace Pds::NsCam;

Icarus::Icarus(std::shared_ptr<Board> board) :
  Sensor(SensorType::ICARUS, board, 4, 2, 1)
{}

bool Icarus::checkSensorVoltStat()
{
  return board_->getSubRegister("ICARUS_DET");
}
