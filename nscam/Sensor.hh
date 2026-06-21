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
             uint32_t nseq,
             uint32_t minframe,
             uint32_t maxframe,
             uint32_t maxwidth,
             uint32_t maxheight,
             uint32_t bytesperpixel);
      virtual ~Sensor() = default;

      virtual void info() const;
      virtual SensorType type() const;
      virtual std::string name() const;

      /* fixed sensor values */
      virtual uint32_t minframe() const;
      virtual uint32_t maxframe() const;
      virtual uint32_t maxwidth() const;
      virtual uint32_t maxheight() const;
      virtual uint32_t bytesperpixel() const;

      /* roi functions */
      virtual uint32_t nframes() const;
      virtual uint32_t firstframe() const;
      virtual uint32_t lastframe() const;
      virtual uint32_t firstrow() const;
      virtual uint32_t lastrow() const;
      virtual size_t width() const;
      virtual size_t height() const;
      virtual size_t npixels() const;
      virtual size_t payloadSize() const;
      virtual void setRows();
      virtual void setRows(uint32_t minrow, uint32_t maxrow);
      virtual void setFrames();
      virtual void setFrames(uint32_t minframe, uint32_t maxframe);

      /* manual timing flag */
      virtual bool manualTiming() const;

      virtual void initSensor();
      virtual bool checkSensorVoltStat() = 0;
      virtual Sequence getArbTiming(SideType side) const;
      virtual Timing getTiming(SideType side) const;
      virtual Sequence getManualTiming(SideType side) const;
      virtual void setArbTiming(SideType side, const Sequence& sequence);
      virtual void setTiming(SideType side, const Timing& timing);
      virtual void setManualTiming(SideType side, const Sequence& sequence);
      virtual void setOscillator(OscillatorType osc);
      virtual OscillatorType getOscillator() const;

      virtual std::unique_ptr<uint8_t[]>  readFrame8();
      virtual std::unique_ptr<uint16_t[]> readFrame16();
      virtual std::unique_ptr<uint32_t[]> readFrame32();

      virtual void restoreCachedTiming();

    protected:
      SensorType stype_;
      std::shared_ptr<Board> board_;
      uint32_t nseq_;
      uint32_t minframe_;
      uint32_t maxframe_;
      uint32_t maxwidth_;
      uint32_t maxheight_;
      uint32_t firstframe_;
      uint32_t lastframe_;
      uint32_t firstrow_;
      uint32_t lastrow_;
      uint32_t bytesperpixel_;
      std::array<uint32_t, 2> interlacing_;
      bool manualTiming_;
      TimingCache timingCache_;
    };
  }
}

#endif

