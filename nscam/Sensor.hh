#ifndef Pds_NsCam_Sensor_hh
#define Pds_NsCam_Sensor_hh

#include "pds/nscam/Device.hh"

namespace Pds {
  namespace NsCam {
    class Sensor : public Device {
    public:
      static std::shared_ptr<Sensor> create(SensorType stype, BoardType btype, std::shared_ptr<Comm> comm);
      Sensor(SensorType stype,
             BoardType btype,
             std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames,
             const std::map<std::string, uint32_t>& defaults);
      virtual ~Sensor() = default;

      virtual void info() const;
      virtual SensorType type() const;
      virtual std::string name() const;

    protected:
      SensorType stype_;
      BoardType btype_;
    };
  }
}

#endif

