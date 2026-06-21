#include "Icarus2.hh"
#include "Error.hh"

using namespace Pds::NsCam;

Icarus2::Icarus2(std::shared_ptr<Board> board) :
  Sensor(SensorType::ICARUS2, board, 4, 0, 3, 512, 1024, 2)
{}

bool Icarus2::checkSensorVoltStat()
{
  return board_->getSubRegister("ICARUS_DET");
}
