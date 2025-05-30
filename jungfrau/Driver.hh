#ifndef Pds_Jungfrau_Driver_hh
#define Pds_Jungfrau_Driver_hh

#include "pds/config/JungfrauConfigType.hh"

#include <poll.h>
#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <vector>

namespace sls {
  class Detector;
}

namespace Pds {
  namespace Jungfrau {
    class DacsConfig {
      public:
        DacsConfig();
        DacsConfig(uint16_t vb_ds, uint16_t vb_comp, uint16_t vb_pixbuf, uint16_t vref_ds,
                   uint16_t vref_comp, uint16_t vref_prech, uint16_t vin_com, uint16_t  vdd_prot);
        ~DacsConfig();
        bool operator==(const DacsConfig& rhs) const;
        bool operator!=(const DacsConfig& rhs) const;
      public:
        uint16_t vb_ds() const;
        uint16_t vb_comp() const;
        uint16_t vb_pixbuf() const;
        uint16_t vref_ds() const;
        uint16_t vref_comp() const;
        uint16_t vref_prech() const;
        uint16_t vin_com() const;
        uint16_t vdd_prot() const;
      private:
        uint16_t  _vb_ds;
        uint16_t  _vb_comp;
        uint16_t  _vb_pixbuf;
        uint16_t  _vref_ds;
        uint16_t  _vref_comp;
        uint16_t  _vref_prech;
        uint16_t  _vin_com;
        uint16_t  _vdd_prot;
    };

    class Module {
      public:
        enum Status { IDLE, ERROR, WAITING, RUN_FINISHED, TRANSMITTING, RUNNING, STOPPED, UNKNOWN };
        Module(const int id, const std::string& control, const std::string& host, const unsigned port,
               const std::string& mac, const std::string& det_ip, bool use_flow_ctrl, bool config_det_ip=true);
        ~Module();
        void shutdown();
        bool allocated() const;
        bool connected() const;
        bool check_config();
        bool configure_mac(bool config_det_ip=true);
        bool configure_dacs(const DacsConfig& dac_config);
        bool configure_power(bool reset_adc=false);
        bool configure_speed(JungfrauConfigType::SpeedMode speed, bool& sleep);
        bool configure_acquistion(uint64_t nframes,
                                  double trig_delay,
                                  double exposure_time,
                                  double exposure_period,
                                  bool force_reset=false);
        bool configure_gain(uint32_t bias, JungfrauConfigType::GainMode gain);
        bool check_size(uint32_t num_rows, uint32_t num_columns) const;
        uint64_t next_frame();
        uint64_t moduleid();
        uint64_t serialnum();
        uint64_t software();
        uint64_t firmware();
        Status status();
        std::string status_str();
        bool start();
        bool stop();
        unsigned flush();
        void reset();
        bool get_packet(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data, bool* first_packet, bool* last_packet, unsigned* npackets);
        bool get_frame(uint64_t* framenum, uint16_t* data);
        bool get_frame(uint64_t* framenum, JungfrauModInfoType* metadata, uint16_t* data);
        const char* get_hostname() const;
        const char* error();
        void set_error(const char* fmt, ...);
        void clear_error();
        void set_next_frame(uint64_t nframe);
        int fd() const;
        unsigned get_num_rows() const;
        unsigned get_num_columns() const;
        unsigned get_num_pixels() const;
        unsigned get_frame_size() const;
      private:
        bool check_readback(const char* name, uint32_t rbv, uint32_t expected);
        bool verify_dacs(const DacsConfig& dac_config);
        bool verify_speed(JungfrauConfigType::SpeedMode speed);
        bool verify_acquistion(uint64_t nframes, uint64_t ncycles, double trig_delay, double exposure_time, double exposure_period);
        bool verify_gain(uint32_t bias, JungfrauConfigType::GainMode gain);
      private:
        const int         _id;
        std::string       _control;
        std::string       _host;
        const unsigned    _port;
        std::string       _mac;
        std::string       _det_ip;
        const bool        _use_flow_ctrl;
        int               _socket;
        bool              _connected;
        bool              _boot;
        bool              _freerun;
        bool              _poweron;
        unsigned          _sockbuf_sz;
        unsigned          _readbuf_sz;
        unsigned          _frame_sz;
        unsigned          _frame_elem;
        char*             _readbuf;
        char*             _msgbuf;
        sls::Detector*    _det;
        DacsConfig        _dac_config;
        JungfrauConfigType::SpeedMode _speed;
    };

    class Detector {
      public:
        Detector(std::vector<Module*>& modules, bool use_threads, int thread_rtprio=0);
        ~Detector();
        void shutdown();
        bool allocated() const;
        bool connected() const;
        bool configure(uint64_t nframes, JungfrauConfigType::GainMode gain, JungfrauConfigType::SpeedMode speed, double trig_delay, double exposure_time, double exposure_period, uint32_t bias, const DacsConfig& dac_config);
        bool check_size(uint32_t num_modules, uint32_t num_rows, uint32_t num_columns) const;
        bool get_frame(uint64_t* framenum, uint16_t* data);
        bool get_frame(uint64_t* framenum, JungfrauModInfoType* metadata, uint16_t* data);
        bool get_module_config(JungfrauModConfigType* module_config, unsigned max_modules=JungfrauConfigType::MaxModulesPerDetector);
        bool start();
        bool stop();
        void flush();
        unsigned get_num_rows(unsigned module) const;
        unsigned get_num_columns(unsigned module) const;
        unsigned get_num_modules() const;
        unsigned get_num_pixels() const;
        unsigned get_frame_size() const;
        uint64_t sync_next_frame();
        const char* get_hostname(unsigned module) const;
        const char** errors();
        void clear_errors();
        void abort();
        bool aborted() const;
      private:
        void signal(int sig);
        bool get_frame_thread(uint64_t* framenum, JungfrauModInfoType* metadata, uint16_t* data);
        bool get_frame_poll(uint64_t* framenum, JungfrauModInfoType* metadata, uint16_t* data);
      private:
        bool                  _aborted;
        bool                  _use_threads;
        pthread_t*            _threads;
        pthread_attr_t*       _thread_attr;
        pollfd*               _pfds;
        int                   _sigfd[2];
        unsigned              _num_modules;
        uint64_t*             _module_frames;
        bool*                 _module_first_packet;
        bool*                 _module_last_packet;
        unsigned*             _module_npackets;
        uint16_t**            _module_data;
        std::vector<Module*>& _modules;
        const char**          _msgbuf;
    };
  }
}

#endif
