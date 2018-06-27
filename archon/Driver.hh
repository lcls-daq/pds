#ifndef Pds_Archon_Driver_hh
#define Pds_Archon_Driver_hh

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
        virtual ~OutputParser();
        int parse(char* buffer);
        std::string get_value(std::string key) const;
        uint64_t get_value_as_uint64(std::string key) const;
        int64_t get_value_as_int64(std::string key) const;
        uint32_t get_value_as_uint32(std::string key) const;
        int32_t get_value_as_int32(std::string key) const;
        double get_value_as_double(std::string key) const;
        void dump() const;
      private:
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
        bool is_power_good() const;
        bool is_overheated() const;
        bool update(char* buffer);
      protected:
        static const int NUM_ENTRIES = 31;
      private:
        int _num_module_entries;
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
        bool configure(const char* filepath);
        bool command(const char* cmd);
        bool load_parameter(const char* param, unsigned value, bool fast=true);
        bool wr_config_line(unsigned num, const char* line);
        bool fetch_status();
        bool fetch_buffer_info();
        bool fetch_frame(uint32_t frame_number, void* data, FrameMetaData* frame_meta=NULL, bool need_fetch=true);
        bool wait_frame(void* data, FrameMetaData* frame_meta=NULL, int timeout=0);
        bool start_acquisition(uint32_t num_frames=0);
        bool stop_acquisition();
        bool set_integration_time(unsigned milliseconds);
        bool set_non_integration_time(unsigned milliseconds);
        void set_frame_poll_interval(unsigned microseconds);
        const unsigned long long time();
        const char* command_output(const char* cmd, char delim='\n');
        const char* message() const;
        AcqMode acquisition_mode() const;
        const Status& status() const;
        const BufferInfo& buffer_info() const;
      private:
        ssize_t fetch_buffer(unsigned buffer_idx, void* data);
        bool lock_buffer(unsigned buffer_idx);
      private:
        int           _recv(char* buf, unsigned bufsz);
        const char*   _host;
        unsigned      _port;
        int           _socket;
        bool          _connected;
        AcqMode       _acq_mode;
        unsigned char _msgref;
        unsigned      _readbuf_sz;
        unsigned      _writebuf_sz;
        char*         _readbuf;
        char*         _writebuf;
        char*         _message;
        uint32_t      _last_frame;
        timespec      _sleep_time;
        Status        _status;
        BufferInfo    _buffer_info;
    };
  }
}

#endif
