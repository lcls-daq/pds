#ifndef Pds_Archon_Driver_hh
#define Pds_Archon_Driver_hh

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <map>

namespace Pds {
  namespace Archon {

    enum AcqMode   { Stopped=0, Fixed=1, Continuous=2 };
    enum FrameMode { Top=0, Bottom=1, Split=2, Error=3 };
    enum PowerMode { Unknown=0, NotConfigured=1, Off=2, Intermediate=3, On=4, Standby=5 };

    class OutputParser {
      public:
        OutputParser();
        OutputParser(const char* delimeters);
        virtual ~OutputParser();
        int parse(char* buffer);
        std::string get_value(std::string key) const;
        uint64_t get_value_as_uint64(std::string key) const;
        int64_t get_value_as_int64(std::string key) const;
        uint32_t get_value_as_uint32(std::string key) const;
        int32_t get_value_as_int32(std::string key) const;
        uint16_t get_value_as_uint16(std::string key) const;
        int16_t get_value_as_int16(std::string key) const;
        double get_value_as_double(std::string key) const;
        virtual bool update(char* buffer) = 0;
        void dump() const;
      protected:
        size_t size() const;
      private:
        const char* _delimeters;
        std::map<std::string, std::string> _data;
    };

    class BufferInfo : public OutputParser {
      public:
        BufferInfo(unsigned nbuffers);
        ~BufferInfo();
        bool update(char* buffer);
        unsigned nbuffers() const;
        unsigned frame_ready(uint32_t frame_number) const;
        uint64_t fetch_time() const;
        uint32_t current_buffer() const;
        uint32_t read_buffer() const;
        uint32_t write_buffer() const;
        uint32_t latest_frame() const;
        bool is32bit(unsigned buffer_idx=0) const;
        bool complete(unsigned buffer_idx=0) const;
        uint32_t size(unsigned buffer_idx=0) const;
        FrameMode mode(unsigned buffer_idx=0) const;
        uint32_t base(unsigned buffer_idx=0) const;
        uint32_t offset(unsigned buffer_idx=0) const;
        uint32_t frame_num(unsigned buffer_idx=0) const;
        uint32_t width(unsigned buffer_idx=0) const;
        uint32_t height(unsigned buffer_idx=0) const;
        uint32_t pixels(unsigned buffer_idx=0) const;
        uint32_t lines(unsigned buffer_idx=0) const;
        uint32_t raw_blocks(unsigned buffer_idx=0) const;
        uint32_t raw_lines(unsigned buffer_idx=0) const;
        uint64_t timestamp(unsigned buffer_idx=0) const;
        uint64_t re_timestamp(unsigned buffer_idx=0) const;
        uint64_t fe_timestamp(unsigned buffer_idx=0) const;
      protected:
        static const int NUM_ENTRIES_BASE = 3;
        static const int NUM_ENTRIES_PER_BUFFER = 19;
        uint32_t get_buffer(const char* cmd_fmt, unsigned buffer_idx) const;
        uint64_t get_ts_buffer(const char* cmd_fmt, unsigned buffer_idx) const;
      private:
        const unsigned  _nbuffers;
        const int       _num_expected_entries;
        unsigned        _current_buf;
        char*           _cmd_buff;
    };

    class System : public OutputParser {
      public:
        System(int num_modules);
        ~System();
        int num_modules() const;
        uint32_t type() const;
        uint32_t rev() const;
        std::string version() const;
        uint16_t id() const;
        uint16_t present() const;
        uint32_t module_type(unsigned mod) const;
        uint32_t module_rev(unsigned mod) const;
        std::string module_version(unsigned mod) const;
        uint16_t module_id(unsigned mod) const;
        bool module_present(unsigned mod) const;
        bool update(char* buffer);
      protected:
        static const int NUM_ENTRIES_BASE = 6;
        static const int NUM_ENTRIES_PER_MOD = 4;
      private:
        int   _num_modules;
        char* _cmd_buff;
    };

    class Status : public OutputParser {
      public:
        Status();
        ~Status();
        int num_module_entries() const;
        bool is_valid() const;
        uint32_t count() const;
        uint32_t log_entries() const;
        PowerMode power() const;
        const char* power_str() const;
        double backplane_temp() const;
        double get_module_voltage(unsigned module_num, const char* module_name, int channel) const;
        double get_module_current(unsigned module_num, const char* module_name, int channel) const;
        bool is_power_good() const;
        bool is_overheated() const;
        bool update(char* buffer);
      protected:
        static const int NUM_ENTRIES = 31;
      private:
        double get_module_readback(unsigned module_num, const char* module_name, int channel, const char* type) const;
        int _num_module_entries;
    };

    class Config : public OutputParser {
      public:
        Config();
        ~Config();
        int num_config_lines() const;
        uint32_t active_taplines() const;
        uint32_t taplines() const;
        uint32_t linecount() const;
        uint32_t pixelcount() const;
        uint32_t samplemode() const;
        uint32_t linescan() const;
        uint32_t bytes_per_pixel() const;
        uint32_t pixels_per_line() const;
        uint32_t total_pixels() const;
        uint32_t frame_size() const;
        std::string line(unsigned num) const;
        std::string parameter(const char* name) const;
        std::string constant(const char* name) const;
        int get_parameter_num(const char* name) const;
        int get_cache(std::string key) const;
        bool cache(const char* line, unsigned num);
        bool update(char* buffer);
      private:
        int extract_sub_key_index(const char* num_entries, const char* entry_fmt, const char* key) const;
        std::string extract_sub_key(const char* num_entries, const char* entry_fmt, const char* key) const;
        std::map<std::string, unsigned> _config_cache;
    };

    class FrameMetaData {
      public:
        FrameMetaData();
        FrameMetaData(uint32_t number, uint64_t timestamp, ssize_t size);
        ~FrameMetaData();
      public:
        uint32_t  number;
        uint64_t  timestamp;
        ssize_t   size;
    };

    class Driver {
      public:
        Driver(const char* host, unsigned port);
        ~Driver();
        void connect();
        void disconnect();
        bool configure(const char* filepath);
        bool configure(void* buffer, size_t size);
        bool command(const char* cmd);
        bool load_parameter(const char* param, unsigned value, bool fast=true);
        bool wr_config_line(unsigned num, const char* line, bool cache=true);
        bool edit_config_line(const char* key, const char* value);
        bool edit_config_line(const char* key, unsigned value);
        bool edit_config_line(const char* key, signed value);
        bool edit_config_line(const char* key, double value);
        bool fetch_system();
        bool fetch_status();
        bool fetch_buffer_info();
        bool fetch_config();
        bool fetch_frame(uint32_t frame_number, void* data, FrameMetaData* frame_meta=NULL, bool need_fetch=true);
        bool wait_frame(void* data, FrameMetaData* frame_meta=NULL, int timeout=0);
        bool flush_frame(void* data, FrameMetaData* frame_meta=NULL);
        bool start_acquisition(uint32_t num_frames=0);
        bool stop_acquisition();
        bool clear_acquisition();
        bool wait_power_mode(PowerMode mode, int timeout=0);
        bool power_on();
        bool power_off();
        bool set_number_of_lines(unsigned num_lines, unsigned num_batch, bool reload=true);
        bool set_number_of_lines(unsigned num_lines, bool reload=true);
        bool set_number_of_pixels(unsigned num_pixels, bool reload=true);
        bool set_vertical_binning(unsigned binning);
        bool set_horizontal_binning(unsigned binning);
        bool set_preframe_clear(unsigned num_lines);
        bool set_idle_clear(unsigned num_lines=1);
        bool set_preframe_skip(unsigned num_lines);
        bool set_integration_time(unsigned milliseconds);
        bool set_non_integration_time(unsigned milliseconds);
        bool set_linescan_mode(bool enable, bool reload=true);
        bool set_external_trigger(bool enable, bool reload=true);
        bool set_clock_at(unsigned ticks);
        bool set_clock_st(unsigned ticks);
        bool set_clock_stm1(unsigned ticks);
        bool set_bias(int channel, bool enabled, float voltage, bool fetch=false);
        bool get_bias(int channel, float* voltage, float* current, bool fetch=false);
        int find_config_line(const char* line, bool use_cache=true);
        void timeout_waits(bool request_timeout=true);
        void set_frame_poll_interval(unsigned microseconds);
        const unsigned long long time();
        const char* rd_config_line(unsigned num);
        const char* command_output(const char* cmd, char delim='\n');
        const char* message() const;
        AcqMode acquisition_mode() const;
        const System& system() const;
        const Status& status() const;
        const BufferInfo& buffer_info() const;
        const Config& config() const;
      private:
        ssize_t fetch_buffer(unsigned buffer_idx, void* data);
        bool lock_buffer(unsigned buffer_idx);
        bool replace_param_line(const char* param, unsigned value, bool use_cache=true);
        bool replace_config_line(const char* key, const char* newline);
        bool load_config_file(FILE* f);
      private:
        int           _recv(char* buf, unsigned bufsz);
        const char*   _host;
        unsigned      _port;
        int           _socket;
        bool          _connected;
        bool          _timeout_req;
        AcqMode       _acq_mode;
        unsigned char _msgref;
        unsigned      _readbuf_sz;
        unsigned      _writebuf_sz;
        char*         _readbuf;
        char*         _writebuf;
        char*         _message;
        uint32_t      _last_frame;
        timespec      _sleep_time;
        System        _system;
        Status        _status;
        BufferInfo    _buffer_info;
        Config        _config;
    };
  }
}

#endif
