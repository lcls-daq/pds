#ifndef Pds_Zyla_Driver_hh
#define Pds_Zyla_Driver_hh

#include "andor3/include/atutility.h"

#include <stddef.h>
#include <stdint.h>

namespace Pds {
  namespace Zyla {
    class Driver {
      public:
        Driver(AT_H cam, unsigned nbuffers=1);
        ~Driver();
      public:
        enum FanSpeed {
          Off,
          Low,
          On,
          UnsupportedFanSpeed,
        };
        enum CoolingSetpoint {
          Temp_0C,
          Temp_Neg5C,
          Temp_Neg10C,
          Temp_Neg15C,
          Temp_Neg20C,
          Temp_Neg25C,
          Temp_Neg30C,
          Temp_Neg35C,
          Temp_Neg40C,
          UnsupportedTemp,
        };
        enum GainMode {
          HighWellCap12Bit,
          LowNoise12Bit,
          LowNoiseHighWellCap16Bit,
          UnsupportedGain,
        };
        enum TriggerMode {
          Internal,
          ExternalLevelTransition,
          ExternalStart,
          ExternalExposure,
          Software,
          Advanced,
          External,
          UnsupportedTrigger,
        };
        enum ShutteringMode {
          Rolling,
          Global,
          UnsupportedShutter,
        };
        enum ReadoutRate {
          Rate280MHz,
          Rate200MHz,
          Rate100MHz,
          Rate10MHz,
          UnsupportedReadout,
        };
        enum GateMode {
          CWOn,
          CWOff,
          FireOnly,
          GateOnly,
          FireAndGate,
          DDG,
          UnsupportedGate,
        };
        enum InsertionDelay {
          Normal,
          Fast,
          UnsupportedDelay,
        };
      public:
        bool set_image(AT_64 width, AT_64 height, AT_64 orgX, AT_64 orgY, AT_64 binX, AT_64 binY, bool noise_filter, bool blemish_correction, bool fast_frame=true);
        bool set_cooling(bool enable, CoolingSetpoint setpoint, FanSpeed fan_speed);
        bool set_trigger(TriggerMode trigger, double trigger_delay, bool overlap);
        bool set_exposure(double exposure_time);
        bool set_readout(ShutteringMode shutter, ReadoutRate readout_rate, GainMode gain);
        bool set_intensifier(GateMode gate, InsertionDelay delay, AT_64 mcp_gain, bool mcp_intelligate);
        bool configure(const AT_64 nframes=0);
        bool start();
        bool stop(bool flush_buffers=true);
        bool flush();
        bool close();
        bool is_present() const;
        size_t frame_size() const;
        bool get_frame(AT_64& timestamp, uint16_t* data);
      public:
        AT_64 sensor_width() const;
        AT_64 sensor_height() const;
        AT_64 baseline() const;
        AT_64 clock() const;
        AT_64 clock_rate() const;
        AT_64 image_stride() const;
        AT_64 image_width() const;
        AT_64 image_height() const;
        AT_64 image_orgX() const;
        AT_64 image_orgY() const;
        AT_64 image_binX() const;
        AT_64 image_binY() const;
        AT_64 image_binning() const;
        double readout_time() const;
        double frame_rate() const;
        double pixel_height() const;
        double pixel_width() const;
        double temperature() const;
        double exposure() const;
        double exposure_min() const;
        double exposure_max() const;
        bool overlap_mode() const;
        bool cooling_on() const;
        bool check_cooling(bool is_stable=true) const;
        bool wait_cooling(unsigned timeout, bool is_stable=true) const;

        bool get_cooling_status(AT_WC* buffer, int buffer_size) const;
        bool get_shutter_mode(AT_WC* buffer, int buffer_size) const;
        bool get_trigger_mode(AT_WC* buffer, int buffer_size) const;
        bool get_gain_mode(AT_WC* buffer, int buffer_size) const;
        bool get_readout_rate(AT_WC* buffer, int buffer_size) const;
        bool get_binning_mode(AT_WC* buffer, int buffer_size) const;
        bool get_name(AT_WC* buffer, int buffer_size) const;
        bool get_model(AT_WC* buffer, int buffer_size) const;
        bool get_family(AT_WC* buffer, int buffer_size) const;
        bool get_serial(AT_WC* buffer, int buffer_size) const;
        bool get_firmware(AT_WC* buffer, int buffer_size) const;
        bool get_interface_type(AT_WC* buffer, int buffer_size) const;
        bool get_sdk_version(AT_WC* buffer, int buffer_size) const;
      private:
        bool at_check_implemented(const AT_WC* feature) const;
        bool at_check_write(const AT_WC* feature) const;
        bool at_get_string(const AT_WC* feature, AT_WC* buffer, int buffer_size) const;
        AT_64 at_get_int(const AT_WC* feature) const;
        AT_64 at_get_int_min(const AT_WC* feature) const;
        AT_64 at_get_int_max(const AT_WC* feature) const;
        double at_get_float(const AT_WC* feature) const;
        double at_get_float_min(const AT_WC* feature) const;
        double at_get_float_max(const AT_WC* feature) const;
        bool at_get_enum(const AT_WC* feature, AT_WC* buffer, int buffer_size) const;
        bool at_set_int(const AT_WC* feature, const AT_64 value);
        bool at_set_float(const AT_WC* feature, const double value);
        bool at_set_enum(const AT_WC* feature, const AT_WC* value);
        bool at_set_bool(const AT_WC* feature, bool value);
      private:
        AT_H            _cam;
        unsigned        _nbuffers;
        bool            _open;
        bool            _queued;
        int             _buffer_size;
        unsigned char*  _data_buffer;
        AT_64           _stride;
        AT_64           _width;
        AT_64           _height;
    };
  }
}

#endif
