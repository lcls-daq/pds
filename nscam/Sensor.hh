#ifndef Pds_NsCam_Sensor_hh
#define Pds_NsCam_Sensor_hh

#include "pds/nscam/Board.hh"

#include <array>

namespace Pds {
  namespace NsCam {
    struct Timing {
      uint32_t open;
      uint32_t closed;
      uint32_t delay;
    };

    typedef std::vector<uint64_t> Sequence;
    typedef std::map<SideType, Sequence> TimingCache;

    class Sensor {
    public:
      static std::shared_ptr<Sensor> create(SensorType stype, std::shared_ptr<Board> board);
      Sensor(SensorType stype,
             std::shared_ptr<Board> board,
             uint32_t nframes);
      Sensor(SensorType stype,
             std::shared_ptr<Board> board,
             uint32_t nseq,
             uint32_t nframes,
             uint32_t firstframe);
      Sensor(SensorType stype,
             std::shared_ptr<Board> board,
             uint32_t nseq,
             uint32_t nframes,
             uint32_t firstframe,
             uint32_t ncols,
             uint32_t nrows);
      virtual ~Sensor() = default;

      virtual void info() const;
      virtual SensorType type() const;
      virtual std::string name() const;

      virtual uint32_t nframes() const;
      virtual bool manualTiming() const;

      virtual void initSensor();
      virtual bool checkSensorVoltStat() = 0;
      virtual Sequence getArbTiming(SideType side) const;
      virtual Timing getTiming(SideType side) const;
      virtual Sequence getManualTiming(SideType side) const;
      virtual void setArbTiming(SideType side, const Sequence& sequence);
      virtual void setTiming(SideType side, const Timing& timing);
      virtual void setManualTiming(SideType side, const Sequence& sequence);

      virtual void restoreCachedTiming();

    protected:
      SensorType stype_;
      std::shared_ptr<Board> board_;
      uint32_t nseq_;
      uint32_t nframes_;
      uint32_t firstframe_;
      uint32_t ncols_;
      uint32_t nrows_;
      std::array<uint32_t, 2> interlacing_;
      bool manualTiming_;
      TimingCache timingCache_;
    };
  }
}

#endif

