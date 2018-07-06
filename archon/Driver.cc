#include "Driver.hh"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define NUM_BUFFERS 3
#define MODULE_MAX 12
#define MSG_HEADER_LEN 3
#define MSG_BINHEADER_LEN 4
#define MAX_CMD_LEN 64
#define MAX_CONFIG_LINE 2048
#define BUFFER_SIZE 8192
#define BURST_LEN 1024
#define LOAD_PARAM_MAX 1000000

static const char* PowerModeStrings[] = {"Unknown", "Not Configured", "Off", "Intermediate", "On", "Standby"};

static long long int diff_ms(timespec* end, timespec* start) {
  return ((end->tv_sec - start->tv_sec) * 1000) + ((end->tv_nsec - start->tv_nsec) / 1000000);
}


using namespace Pds::Archon;

OutputParser::OutputParser()
{}

OutputParser::~OutputParser()
{}

int OutputParser::parse(char* buffer)
{
  int nparse = 0;
  std::string key;
  std::string value;
  // Remove trailing line feed from buffer
  if (buffer[strlen(buffer)-1] == '\n')
    buffer[strlen(buffer)-1] = 0;
  char* entry = strtok(buffer, " ");
  while (entry != NULL) {
        char* val = strchr(entry, '=');
        if (val != NULL) {
          *val++ = 0;
          key.assign(entry);
          value.assign(val);
          _data[key] = value;
          nparse++;
        }
        entry = strtok(NULL, " ");
  }
  return nparse;
}


std::string OutputParser::get_value(std::string key) const
{
  std::string value;
  std::map<std::string,std::string>::const_iterator it;

  it = _data.find(key);
  if (it != _data.end())
    value.assign(it->second);
  return value;
}

uint64_t OutputParser::get_value_as_uint64(std::string key) const
{
  return strtoull(get_value(key).c_str(), NULL, 16);
}

int64_t OutputParser::get_value_as_int64(std::string key) const
{
  return strtoll(get_value(key).c_str(), NULL, 16);
}

uint32_t OutputParser::get_value_as_uint32(std::string key) const
{
  return strtoul(get_value(key).c_str(), NULL, 0);
}

int32_t OutputParser::get_value_as_int32(std::string key) const
{
  return strtol(get_value(key).c_str(), NULL, 0);
}

uint16_t OutputParser::get_value_as_uint16(std::string key) const
{
  return strtoul(get_value(key).c_str(), NULL, 16);
}

int16_t OutputParser::get_value_as_int16(std::string key) const
{
  return strtol(get_value(key).c_str(), NULL, 16);
}

double OutputParser::get_value_as_double(std::string key) const
{
  return strtod(get_value(key).c_str(), NULL);
}

void OutputParser::dump() const
{
  std::map<std::string,std::string>::const_iterator it;
  for(it = _data.begin(); it != _data.end(); ++it) {
    printf("%s=%s\n", it->first.c_str(), it->second.c_str());
  }
}


BufferInfo::BufferInfo(unsigned nbuffers) :
  _nbuffers(nbuffers),
  _num_expected_entries(NUM_ENTRIES_BASE + NUM_ENTRIES_PER_BUFFER * nbuffers),
  _current_buf(0)
{
  _cmd_buff = new char[MAX_CMD_LEN];
}

BufferInfo::~BufferInfo()
{
  if (_cmd_buff) {
    delete[] _cmd_buff;
  }
}

bool BufferInfo::update(char* buffer)
{
  if (_num_expected_entries == parse(buffer)) {
    uint32_t latest_frame=0;
    for (unsigned i=1; i<=_nbuffers; ++i) {
      if ((frame_num(i) > latest_frame) && complete(i)) {
        latest_frame = frame_num(i);
        _current_buf = i;
      }
    }
    return true;
  } else {
    return false;
  }
}

unsigned BufferInfo::nbuffers() const
{
  return _nbuffers;
}

unsigned BufferInfo::frame_ready(uint32_t frame_number) const
{
  for (unsigned i=1; i<=_nbuffers; i++) {
    if ((frame_num(i) == frame_number) && complete(i)) {
      return i;
    }
  }

  return 0;
}

uint64_t BufferInfo::fetch_time() const
{
    return get_value_as_uint64("TIMER");
}

uint32_t BufferInfo::current_buffer() const
{
  return _current_buf;
}

uint32_t BufferInfo::read_buffer() const
{
  return get_value_as_uint32("RBUF");
}

uint32_t BufferInfo::write_buffer() const
{
  return get_value_as_uint32("WBUF");
}

bool BufferInfo::is32bit(unsigned buffer_idx) const
{
  return get_buffer("BUF%uSAMPLE", buffer_idx);
}

bool BufferInfo::complete(unsigned buffer_idx) const
{
  return get_buffer("BUF%uCOMPLETE", buffer_idx);
}

uint32_t BufferInfo::size(unsigned buffer_idx) const
{
  return height(buffer_idx) * width(buffer_idx) * (is32bit(buffer_idx) ? 4 : 2);
}

FrameMode BufferInfo::mode(unsigned buffer_idx) const
{
  uint32_t value = get_buffer("BUF%uMODE", buffer_idx);
  if (value>Error) {
    return Error;
  } else {
    return (FrameMode) value;
  }
}

uint32_t BufferInfo::base(unsigned buffer_idx) const
{
  return get_buffer("BUF%uBASE", buffer_idx);
}

uint32_t BufferInfo::offset(unsigned buffer_idx) const
{
  return get_buffer("BUF%uRAWOFFSET", buffer_idx);
}

uint32_t BufferInfo::frame_num(unsigned buffer_idx) const
{
  return get_buffer("BUF%uFRAME", buffer_idx);
}

uint32_t BufferInfo::width(unsigned buffer_idx) const
{
  return get_buffer("BUF%uWIDTH", buffer_idx);
}

uint32_t BufferInfo::height(unsigned buffer_idx) const
{
  return get_buffer("BUF%uHEIGHT", buffer_idx);
}

uint32_t BufferInfo::pixels(unsigned buffer_idx) const
{
  return get_buffer("BUF%uPIXELS", buffer_idx);
}

uint32_t BufferInfo::lines(unsigned buffer_idx) const
{
  return get_buffer("BUF%uLINES", buffer_idx);
}

uint32_t BufferInfo::raw_blocks(unsigned buffer_idx) const
{
  return get_buffer("BUF%uRAWBLOCKS", buffer_idx);
}

uint32_t BufferInfo::raw_lines(unsigned buffer_idx) const
{
  return get_buffer("BUF%uRAWLINES", buffer_idx);
}

uint64_t BufferInfo::timestamp(unsigned buffer_idx) const
{
  return get_ts_buffer("BUF%uTIMESTAMP", buffer_idx);
}

uint64_t BufferInfo::re_timestamp(unsigned buffer_idx) const
{
  return get_ts_buffer("BUF%uRETIMESTAMP", buffer_idx);
}

uint64_t BufferInfo::fe_timestamp(unsigned buffer_idx) const
{
  return get_ts_buffer("BUF%uFETIMESTAMP", buffer_idx);
}

uint32_t BufferInfo::get_buffer(const char* cmd_fmt, unsigned buffer_idx) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, cmd_fmt, buffer_idx ? buffer_idx : _current_buf);
  std::string cmd(_cmd_buff);
  return get_value_as_uint32(cmd);
}

uint64_t BufferInfo::get_ts_buffer(const char* cmd_fmt, unsigned buffer_idx) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, cmd_fmt, buffer_idx ? buffer_idx : _current_buf);
  std::string cmd(_cmd_buff);
  return get_value_as_uint64(cmd);
}

System::System(int num_modules) :
  _num_modules(num_modules)
{
  _cmd_buff = new char[MAX_CMD_LEN];
}

System::~System()
{
  if (_cmd_buff) {
    delete[] _cmd_buff;
  }
}

int System::num_modules() const
{
  return _num_modules;
}

uint32_t System::type() const
{
  return get_value_as_uint32("BACKPLANE_TYPE");
}

uint32_t System::rev() const
{
  return get_value_as_uint32("BACKPLANE_REV");
}

std::string System::version() const
{
  return get_value("BACKPLANE_VERSION");
}

uint16_t System::id() const
{
  return get_value_as_uint16("BACKPLANE_ID");
}

uint16_t System::present() const
{
  return get_value_as_uint16("MOD_PRESENT");
}

bool System::module_present(unsigned mod) const
{
  if (mod < (unsigned) _num_modules) {
    return ((1U<<mod) & present());
  } else {
    return false;
  }
}

uint32_t System::module_type(unsigned mod) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, "MOD%u_TYPE", mod);
  std::string cmd(_cmd_buff);
  return get_value_as_uint32(cmd);
}

uint32_t System::module_rev(unsigned mod) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, "MOD%u_REV", mod);
  std::string cmd(_cmd_buff);
  return get_value_as_uint32(cmd);
}

std::string System::module_version(unsigned mod) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, "MOD%u_VERSION", mod);
  std::string cmd(_cmd_buff);
  return get_value(cmd);
}

uint16_t System::module_id(unsigned mod) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, "MOD%u_ID", mod);
  std::string cmd(_cmd_buff);
  return get_value_as_uint16(cmd);
}

bool System::update(char* buffer)
{
  return (parse(buffer) == (NUM_ENTRIES_PER_MOD * _num_modules + NUM_ENTRIES_BASE));
}

Status::Status() :
  _num_module_entries(0)
{}

Status::~Status()
{}

int Status::num_module_entries() const
{
  return _num_module_entries;
}

bool Status::update(char* buffer)
{
  int num_entries = parse(buffer);
  _num_module_entries = num_entries - NUM_ENTRIES;
  return (num_entries >= NUM_ENTRIES);
}

bool Status::is_valid() const
{
  return get_value_as_uint32("VALID");
}

uint32_t Status::count() const
{
  return get_value_as_uint32("COUNT");
}

uint32_t Status::log_entries() const
{
  return get_value_as_uint32("LOG");
}

PowerMode Status::power() const
{
  uint32_t value = get_value_as_uint32("POWER");
  if (value>Standby) {
    return Unknown;
  } else {
    return (PowerMode) value;
  }
}

const char* Status::power_str() const
{
  return PowerModeStrings[power()];
}

bool Status::is_power_good() const
{
  return get_value_as_uint32("POWERGOOD");
}

bool Status::is_overheated() const
{
  return get_value_as_uint32("OVERHEAT");
}

double Status::backplane_temp() const
{
  return get_value_as_double("BACKPLANE_TEMP");
}


FrameMetaData::FrameMetaData() :
  number(0),
  timestamp(0),
  size(0)
{}

FrameMetaData::FrameMetaData(uint32_t number, uint64_t timestamp, ssize_t size) :
  number(number),
  timestamp(timestamp),
  size(size)
{}

FrameMetaData::~FrameMetaData()
{}


Driver::Driver(const char* host, unsigned port) :
  _host(host),
  _port(port),
  _socket(-1),
  _connected(false),
  _acq_mode(Stopped),
  _msgref(0),
  _readbuf_sz(BUFFER_SIZE),
  _writebuf_sz(BUFFER_SIZE),
  _last_frame(0),
  _system(MODULE_MAX),
  _buffer_info(NUM_BUFFERS)
{
  // 1ms frame poll interval by default
  _sleep_time.tv_sec = 0;
  _sleep_time.tv_nsec = 1000000U; // 1ms
  _readbuf = new char[_readbuf_sz];
  _writebuf = new char[_writebuf_sz];
  _message = &_readbuf[MSG_HEADER_LEN];
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);
  connect();
}

Driver::~Driver()
{
  if (_socket >= 0) {
    ::close(_socket);
  }
  if (_readbuf) {
    delete[] _readbuf;
  }
  if (_writebuf) {
    delete[] _writebuf;
  }
}

AcqMode Driver::acquisition_mode() const
{
  return _acq_mode;
}

void Driver::connect()
{
  if (!_connected) {
    hostent* entries = gethostbyname(_host);
    if (entries) {
      unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);
      sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(addr);
      sa.sin_port        = htons(_port);

      int nb = ::connect(_socket, (sockaddr*)&sa, sizeof(sa));
      if (nb<0) {
        fprintf(stderr, "Error: failed to connect to Archon at %s on port %d: %s\n", _host, _port, strerror(errno));
      } else {
        _connected = true;
      }
    } else {
      fprintf(stderr, "Error: failed to connect to Archon at %s - unknown host\n", _host);
    }
  }
}

bool Driver::configure(const char* filepath)
{
  if (!_connected) {
    connect();
    if (!_connected) {
      fprintf(stderr, "Error: unable to load Archon configuration file: controller is not connected!\n");
      return false;
    }
  }
  
  printf("Attempting to load Archon configuration file: %s\n", filepath);

  FILE* f = fopen(filepath, "r");
  if (f) {
    int linenum = 0;
    bool is_conf = false;
    size_t line_sz = 2048;
    size_t hdr_sz = MSG_HEADER_LEN;
    char* hdr  = (char *)malloc(hdr_sz);
    char* reply = (char *)malloc(line_sz+hdr_sz);
    char* line = (char *)malloc(line_sz);

    command("POLLOFF");
    command("CLEARCONFIG");

    while (getline(&line, &line_sz, f) != -1) {
      if (!strncmp(line, "[SYSTEM]", 8)) {
        is_conf = false;
        continue;
      } else if (!strncmp(line, "[CONFIG]", 8)) {
        is_conf = true;
        continue;
      }
      if (is_conf && strcmp(line, "\n")) {
        for(char* p = line; (p = strchr(p, '\\')); ++p) {
          *p = '/';
        }
        char *pr = line, *pw = line;
        while (*pr) {
          *pw = *pr++;
          pw += (*pw != '"');
        }
        *pw = '\0';
        if (!wr_config_line(linenum, line)) {
          fprintf(stderr, "Error writing configuration line %d: %s\n", linenum, line);
          command("POLLON");
          return false;
        }
        linenum++;
      }
    }
    if (line) {
      free(hdr);
      free(reply);
      free(line);
    }
    fclose(f);

    command("POLLON");
    return command("APPLYALL");
  } else {
    fprintf(stderr, "Error opening %s\n", filepath);
    return false;
  }
}

bool Driver::fetch_frame(uint32_t frame_number, void* data, FrameMetaData* frame_meta, bool need_fetch)
{
  if (need_fetch) {
    if (!fetch_buffer_info()) {
      fprintf(stderr, "Error fetching frame buffer info from controller!\n");
      return false;
    }
  }

  ssize_t size = 0;
  uint64_t timestamp = 0;
  unsigned buffer_idx = _buffer_info.frame_ready(frame_number);
  if (buffer_idx) {
    timestamp = _buffer_info.timestamp(buffer_idx);
    size = fetch_buffer(buffer_idx, data);
    if (size != _buffer_info.size(buffer_idx)) {
      fprintf(stderr, "Error fetching frame %u: unexpected size %lu vs expected of %u\n", frame_number, size, _buffer_info.size(buffer_idx));
      return false;
    }
  } else {
    fprintf(stderr, "Error fetching frame %u: frame not available!\n", frame_number);
    return false;
  }

  if (frame_meta) {
    frame_meta->number = frame_number;
    frame_meta->timestamp = timestamp;
    frame_meta->size = size;
  }

  _last_frame = frame_number;

  return true;
}

bool Driver::wait_frame(void* data, FrameMetaData* frame_meta, int timeout)
{
  bool waiting = true;
  uint32_t newest_frame = 0;
  timespec start_time;
  timespec current_time;
  clock_gettime(CLOCK_REALTIME, &start_time);
  while((_acq_mode != Stopped) && waiting && fetch_buffer_info()) {
    newest_frame = _buffer_info.frame_num();
    if (newest_frame > _last_frame) {
      // We have seen at least one new frame
      waiting = false;
    }
    while (newest_frame > _last_frame) {
      if (fetch_frame(_last_frame+1, data, frame_meta, false)) {
        return true;
      } else {
        _last_frame++;
      }
    }
    clock_gettime(CLOCK_REALTIME, &current_time);
    if ((timeout > 0) && (diff_ms(&current_time, &start_time) > timeout)) {
      printf("wait_frame timed out after %d ms!\n", timeout);
      return false;
    }
    nanosleep(&_sleep_time, NULL);
  }

  return false;
}

bool Driver::start_acquisition(uint32_t num_frames)
{
  bool status = false;
  if (!fetch_buffer_info()) {
    fprintf(stderr, "Error fetching frame buffer info from controller!\n");
    return false;
  }
  _last_frame = _buffer_info.frame_num();

  if (num_frames) {
    status = load_parameter("Exposures", num_frames);
    _acq_mode = Fixed;
  } else {
    status = load_parameter("ContinuousExposures", 1U);
    _acq_mode = Continuous;
  }

  return status;
}

bool Driver::stop_acquisition()
{
  bool status = false;

  switch(_acq_mode) {
    case Fixed:
      status = load_parameter("Exposures", 0U);
      break;
    case Continuous:
      status = load_parameter("ContinuousExposures", 0U);
      break;
    case Stopped:
    default:
      status = true;
      break;
  }
  _acq_mode = Stopped;
  return status;
}

bool Driver::set_preframe_clear(bool enable)
{
  return load_parameter("SweepCount", enable ? 1 : 0);
}

bool Driver::set_integration_time(unsigned milliseconds)
{
  return load_parameter("IntMS", milliseconds);
}

bool Driver::set_non_integration_time(unsigned milliseconds)
{
  return load_parameter("NoIntMS", milliseconds);
}

void Driver::set_frame_poll_interval(unsigned microseconds)
{
  _sleep_time.tv_sec = microseconds / 1000000U;
  _sleep_time.tv_nsec = (microseconds % 1000000U) * 1000U;
}

ssize_t Driver::fetch_buffer(unsigned buffer_idx, void* data)
{
  ssize_t len;
  ssize_t ret;
  ssize_t wbytes = 0;
  char* data_ptr = (char*) data;
  char buf[16];
  char cmdstr[22 + MSG_HEADER_LEN];
  uint32_t base = _buffer_info.base(buffer_idx);
  uint32_t size = _buffer_info.size(buffer_idx);
  uint32_t blocks = (size + BURST_LEN -1) / BURST_LEN;

  if (!lock_buffer(buffer_idx)) {
    fprintf(stderr, "Error on locking buffer: %u\n", buffer_idx);
    return wbytes;
  }

  sprintf(cmdstr, ">%02XFETCH%08X%08X\n", _msgref, base, blocks);
  sprintf(buf, "<%02X:", _msgref);
  ::write(_socket, cmdstr, strlen(cmdstr));
  while (wbytes < size) {
    len = ::recv(_socket, _readbuf, MSG_BINHEADER_LEN, MSG_WAITALL);
    if (len<0) {
      fprintf(stderr, "Error: no response from controller: %s\n", strerror(errno));
      return len;
    }
    if (!strncmp(_readbuf, buf, MSG_BINHEADER_LEN)) {
      //_msgref += 1;
      if ((size - wbytes) < BURST_LEN) {
        ret = ::recv(_socket, data_ptr, size - wbytes, MSG_WAITALL);
        // Flush remaining bytes from the last block
        len = ::recv(_socket, _readbuf, BURST_LEN - (size - wbytes), MSG_WAITALL);
      } else {
        ret = ::recv(_socket, data_ptr, BURST_LEN, MSG_WAITALL);
      }
      data_ptr += ret;
      wbytes += ret;
    } else {
      // Flush the leftover message from buffer
      len = _recv(&_readbuf[len], _readbuf_sz - len);
      sprintf(buf, ">%02XFETCHLOG\n", _msgref);
      ::write(_socket, buf, strlen(buf));
      len = _recv(_readbuf, _readbuf_sz);
      if (len < MSG_HEADER_LEN) {
        fprintf(stderr, "Error on command (%s): no message from camera\n", cmdstr);
      } else {
        fprintf(stderr, "Error on command (%s): %s\n", cmdstr, _message);
        _msgref += 1;
      }
      return wbytes;
    }
  }

  _msgref += 1;

  if (!lock_buffer(0)) {
    fprintf(stderr, "Error unlocking buffer: %u\n", buffer_idx);
  }

  return wbytes;
}

bool Driver::command(const char* cmd)
{
  int len;
  char buf[16];
  char cmdstr[strlen(cmd) + MSG_HEADER_LEN + 1];
  sprintf(cmdstr, ">%02X%s\n", _msgref, cmd);
  sprintf(buf, "<%02X", _msgref);
  ::write(_socket, cmdstr, strlen(cmdstr));
  len = _recv(_readbuf, _readbuf_sz);
  if (len<0) {
    fprintf(stderr, "Error: no response from controller: %s\n", strerror(errno));
    return false;
  }
  if (!strncmp(_readbuf, buf, MSG_HEADER_LEN)) {
    _msgref += 1;
    return true;
  } else {
    sprintf(buf, ">%02XFETCHLOG\n", _msgref);
    ::write(_socket, buf, strlen(buf));
    len = _recv(_readbuf, _readbuf_sz);
    if (len < MSG_HEADER_LEN) {
      fprintf(stderr, "Error on command (%s): no message from camera\n", cmdstr);
    } else {
      fprintf(stderr, "Error on command (%s): %s\n", cmdstr, _message);
      _msgref += 1;
    }
    return false;
  }
}

bool Driver::load_parameter(const char* param, unsigned value, bool fast)
{
  if (value > LOAD_PARAM_MAX) {
    fprintf(stderr, "Requested value (%u) is greater than the max of %d\n", value, LOAD_PARAM_MAX);
    return false;
  }

  if (fast)
    snprintf(_writebuf, BUFFER_SIZE, "FASTLOADPARAM %s %u", param, value);
  else
    snprintf(_writebuf, BUFFER_SIZE, "LOADPARAM %s %u", param, value);

  return command(_writebuf);
}

bool Driver::wr_config_line(unsigned num, const char* line)
{
  if (strlen(line) > MAX_CONFIG_LINE) {
    fprintf(stderr, "Configuration line %d is longer than controller max of %d\n", num, MAX_CONFIG_LINE);
    return false;
  }

  snprintf(_writebuf, BUFFER_SIZE, "WCONFIG%04d%s", num, line);
  return command(_writebuf);
}

bool Driver::fetch_system()
{
  if (command("SYSTEM")) {
    return _system.update(_message);
  }
  return false;
}

bool Driver::fetch_status()
{
  if (command("STATUS")) {
    return _status.update(_message);
  }
  return false;
}

bool Driver::fetch_buffer_info()
{
  if (command("FRAME")) {
    return _buffer_info.update(_message);
  }
  return false;
}

int Driver::_recv(char* buf, unsigned bufsz)
{
  int len;
  len = ::read(_socket, buf, bufsz);
  if (len<0) {
    buf[0] = '\0'; // make sure buffer is 'empty'
    return len;
  } else if (len>0 && buf[len-1] != '\n') {
    len += _recv(&buf[len], bufsz-len);
  }
  // make sure buffer is properly terminated
  if (len < (int) bufsz) {
    buf[len] = '\0';
  }
  return len;
}

const unsigned long long Driver::time()
{
  if (command("TIMER")) {
    char* timer_val = strchr(_message, '=');
    if (timer_val != NULL) {
      timer_val++;
      return strtoull(timer_val, NULL, 16);
    }
  }

  return 0;
}

const char* Driver::command_output(const char* cmd, char delim)
{
  if (command(cmd)) {
    char* msg_ptr = _message;
    while((msg_ptr = strchr(msg_ptr, ' ')) != NULL) {
        *msg_ptr++ = delim;
    }
  } else {
    *_message = 0;
  }

  return _message;
}

const char* Driver::message() const
{
  return _message;
}

const System& Driver::system() const
{
  return _system;
}

const Status& Driver::status() const
{
  return _status;
}

const BufferInfo& Driver::buffer_info() const
{
  return _buffer_info;
}

bool Driver::lock_buffer(unsigned buffer_idx)
{
  if (buffer_idx > _buffer_info.nbuffers()) {
    fprintf(stderr, "Invalid buffer number for locking: %u\n", buffer_idx);
    return false;
  } else {
    char cmdstr[6];
    sprintf(cmdstr, "LOCK%u", buffer_idx);
    return command(cmdstr);
  }
}

#undef NUM_BUFFERS
#undef MODULE_MAX
#undef MSG_HEADER_LEN
#undef MSG_BINHEADER_LEN
#undef MAX_CMD_LEN
#undef MAX_CONFIG_LINE
#undef BUFFER_SIZE
#undef BURST_LEN
#undef LOAD_PARAM_MAX
