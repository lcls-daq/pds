#include "Driver.hh"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#define NUM_BUFFERS 3
#define MODULE_MAX 12
#define MSG_HEADER_LEN 3
#define MSG_BINHEADER_LEN 4
#define MAX_CMD_LEN 64
#define MAX_CONFIG_LINE 2048
#define BUFFER_SIZE 8192
#define BURST_LEN 1024
#define LOAD_PARAM_MAX 1000000
#define XV_MOD_TYPE 12
#define HEATERX_MOD_TYPE 11
#define CONF_FLT_DELTA 0.001
#define MIN_BATCH_TS_VER 1252

static const char* PowerModeStrings[] = {"Unknown", "Not Configured", "Off", "Intermediate", "On", "Standby"};

static long long int diff_ms(timespec* end, timespec* start) {
  return ((end->tv_sec - start->tv_sec) * 1000) + ((end->tv_nsec - start->tv_nsec) / 1000000);
}


using namespace Pds::Archon;

OutputParser::OutputParser() :
  _delimeters(" ")
{}

OutputParser::OutputParser(const char* delimeters) :
  _delimeters(delimeters)
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
  char* entry = strtok(buffer, _delimeters);
  while (entry != NULL) {
        char* val = strchr(entry, '=');
        if (val != NULL) {
          *val++ = 0;
          key.assign(entry);
          value.assign(val);
          _data[key] = value;
          nparse++;
        }
        entry = strtok(NULL, _delimeters);
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

size_t OutputParser::size() const
{
  return _data.size();
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

uint32_t BufferInfo::latest_frame() const
{
  return frame_num(write_buffer());
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
  // If the buffer is complete read the whole buffer otherwise read only the completed lines
  if (complete(buffer_idx)) {
    return height(buffer_idx) * width(buffer_idx) * (is32bit(buffer_idx) ? 4 : 2);
  } else {
    return lines(buffer_idx) * width(buffer_idx) * (is32bit(buffer_idx) ? 4 : 2);
  }
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

uint32_t System::build() const
{
  uint32_t build = 0;
  std::string ver = version();
  size_t pos = ver.find(".");

  if (pos != std::string::npos) {
    pos = ver.find(".", pos + 1);
    if (pos != std::string::npos) {
      build = strtoul(ver.substr(pos+1, ver.find(".", pos + 1)).c_str(), NULL, 0);
    }
  }

  return build;
}

std::string System::version() const
{
  return get_value("BACKPLANE_VERSION");
}

std::string System::id() const
{
  return get_value("BACKPLANE_ID");
}

std::string System::power_id() const
{
  return get_value("POWER_ID");
}

uint16_t System::present() const
{
  return get_value_as_uint16("MOD_PRESENT");
}

bool System::module_present(unsigned mod) const
{
  if ((mod > 0) && (mod <= (unsigned) _num_modules)) {
    return ((1U<<(mod-1)) & present());
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

uint32_t System::module_build(unsigned mod) const
{
  uint32_t build = 0;
  std::string ver = module_version(mod);
  size_t pos = ver.find(".");

  if (pos != std::string::npos) {
    pos = ver.find(".", pos + 1);
    if (pos != std::string::npos) {
      build = strtoul(ver.substr(pos + 1, ver.find(".", pos + 1)).c_str(), NULL, 0);
    }
  }

  return build;
}

std::string System::module_version(unsigned mod) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, "MOD%u_VERSION", mod);
  std::string cmd(_cmd_buff);
  return get_value(cmd);
}

std::string System::module_id(unsigned mod) const
{
  snprintf(_cmd_buff, MAX_CMD_LEN, "MOD%u_ID", mod);
  std::string cmd(_cmd_buff);
  return get_value(cmd);
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

double Status::module_temp(unsigned module_num) const
{
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "MOD%d/TEMP", module_num);
  return get_value_as_double(buffer);
}

double Status::get_module_voltage(unsigned module_num, const char* module_name, int channel) const
{
  return get_module_readback(module_num, module_name, channel, "V");
}

double Status::get_module_current(unsigned module_num, const char* module_name, int channel) const
{
  return get_module_readback(module_num, module_name, channel, "I");
}

double Status::get_heater_output(unsigned module_num, char heater) const
{
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "MOD%d/HEATER%cOUTPUT", module_num, heater);
  return get_value_as_double(buffer);
}

uint32_t Status::get_heater_pid(unsigned module_num, char heater, char term) const
{
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "MOD%d/HEATER%c%c", module_num, heater, term);
  return get_value_as_uint32(buffer);
}

double Status::get_sensor_temp(unsigned module_num, char sensor) const
{
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "MOD%d/TEMP%c", module_num, sensor);
  return get_value_as_double(buffer);
}

double Status::get_module_readback(unsigned module_num, const char* module_name, int channel, const char* type) const
{
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "MOD%d/%s_%s%d", module_num, module_name, type, abs(channel));
  return get_value_as_double(buffer);
}

Config::Config() :
  OutputParser("\n")
{}

Config::~Config()
{}

int Config::num_config_lines() const
{
  return size();
}

uint32_t Config::active_taplines() const
{
  char buffer[32];
  uint32_t active = 0;
  uint32_t num = get_value_as_uint32("TAPLINES");
  for (unsigned i=0; i<num; i++) {
    snprintf(buffer, sizeof(buffer), "TAPLINE%u", i);
    std::string tapcfg = get_value(buffer);
    if (!tapcfg.empty())
      active++;
  }
  return active;
}

uint32_t Config::taplines() const
{
  return get_value_as_uint32("TAPLINES");
}

uint32_t Config::linecount() const
{
  return get_value_as_uint32("LINECOUNT");
}

uint32_t Config::pixelcount() const
{
  return get_value_as_uint32("PIXELCOUNT");
}

uint32_t Config::samplemode() const
{
  return get_value_as_uint32("SAMPLEMODE");
}

uint32_t Config::linescan() const
{
  return get_value_as_uint32("LINESCAN");
}

uint32_t Config::bytes_per_pixel() const
{
  uint32_t nbytes;
  switch(samplemode()) {
  case 0:
    nbytes = 2;
    break;
  case 1:
    nbytes = 4;
    break;
  default:
    nbytes = 0;
    break;
  }

  return nbytes;
}

uint32_t Config::pixels_per_line() const
{
  return active_taplines() * pixelcount();
}

uint32_t Config::total_pixels() const
{
  return pixels_per_line() * linecount();
}

uint32_t Config::frame_size() const
{
  return total_pixels() * bytes_per_pixel();
}

uint32_t Config::trigout_force() const
{
  return get_value_as_uint32("TRIGOUTFORCE");
}

uint32_t Config::trigout_level() const
{
  return get_value_as_uint32("TRIGOUTLEVEL");
}

uint32_t Config::trigout_invert() const
{
  return get_value_as_uint32("TRIGOUTINVERT");
}

std::string Config::line(unsigned num) const
{
  char line_name[32];
  snprintf(line_name, sizeof(line_name), "LINE%u", num);
  return get_value(line_name);
}

std::string Config::constant(const char* name) const
{
  return extract_sub_key("CONSTANTS", "CONSTANT%u", name);
}

int Config::get_cache(std::string key) const
{
  int value = -1;
  std::map<std::string,unsigned>::const_iterator it;

  it = _config_cache.find(key);
  if (it != _config_cache.end())
    value = it->second;

  return value;
}

std::string Config::parameter(const char* name) const
{
  return extract_sub_key("PARAMETERS", "PARAMETER%u", name);
}

int Config::get_parameter_num(const char* name) const
{
  return extract_sub_key_index("PARAMETERS", "PARAMETER%u", name);
}

bool Config::cache(const char* line, unsigned num)
{
  if (line != NULL) {
    std::string key;
    char buffer[strlen(line) + 1];
    strcpy(buffer, line);
    char* div = strchr(buffer, '=');
    if (div != NULL)
      *div = 0;
    key.assign(buffer);
    _config_cache[key] = num;

    return true;
  } else {
    return false;
  }
}

bool Config::update(char* buffer)
{
  return parse(buffer) > 0;
}

int Config::extract_sub_key_index(const char* num_entries, const char* entry_fmt, const char* key) const
{
  char buffer[32];
  char search[strlen(key) + 2];
  int result = -1;
  snprintf(search, sizeof(search), "%s=", key);
  uint32_t num = get_value_as_uint32(num_entries);
  for (unsigned i=0; i<num; i++) {
    snprintf(buffer, sizeof(buffer), entry_fmt, i);
    std::string value = get_value(buffer);
    if(!strncmp(search, value.c_str(), strlen(search))) {
      result = i;
    }
  }
  return result;
}

std::string Config::extract_sub_key(const char* num_entries, const char* entry_fmt, const char* key) const
{
  char buffer[32];
  char search[strlen(key) + 2];
  std::string result;
  snprintf(search, sizeof(search), "%s=", key);
  uint32_t num = get_value_as_uint32(num_entries);
  for (unsigned i=0; i<num; i++) {
    snprintf(buffer, sizeof(buffer), entry_fmt, i);
    std::string value = get_value(buffer);
    if(!strncmp(search, value.c_str(), strlen(search))) {
      const char* val = strchr(value.c_str(), '=');
      result.assign(++val);
      break;
    }
  }
  return result;
}

FrameMetaData::FrameMetaData() :
  number(0),
  timestamp(0),
  re_timestamp(0),
  fe_timestamp(0),
  fetch(0),
  batch(0),
  width(0),
  height(0),
  is32bit(0),
  size(0)
{}

FrameMetaData::FrameMetaData(uint32_t number,
                             uint64_t timestamp,
                             uint64_t re_timestamp,
                             uint64_t fe_timestamp,
                             uint64_t fetch,
                             uint32_t batch,
                             uint32_t width,
                             uint32_t height,
                             uint32_t is32bit,
                             ssize_t size) :
  number(number),
  timestamp(timestamp),
  re_timestamp(re_timestamp),
  fe_timestamp(fe_timestamp),
  fetch(fetch),
  batch(batch),
  width(width),
  height(height),
  is32bit(is32bit),
  size(size)
{}

FrameMetaData::~FrameMetaData()
{}

HeaterConfig::HeaterConfig() :
  label(""),
  enable(0),
  force(0),
  forcelevel(0.0),
  limit(0.0),
  target(0.0),
  sensor(0),
  pterm(0.0),
  iterm(0.0),
  dterm(0.0),
  itermlimit(0),
  ramp(0),
  ramprate(0),
  updatetime(0)
{}

HeaterConfig::HeaterConfig(const std::string& label,
                           uint32_t enable,
                           uint32_t force,
                           double forcelevel,
                           double limit,
                           double target,
                           uint32_t sensor,
                           double pterm,
                           double iterm,
                           double dterm,
                           uint32_t itermlimit,
                           uint32_t ramp,
                           uint32_t ramprate,
                           uint32_t updatetime) :
  label(label),
  enable(enable),
  force(force),
  forcelevel(forcelevel),
  limit(limit),
  target(target),
  sensor(sensor),
  pterm(pterm),
  iterm(iterm),
  dterm(dterm),
  itermlimit(itermlimit),
  ramp(ramp),
  ramprate(ramprate),
  updatetime(updatetime)
{}

HeaterConfig::~HeaterConfig()
{}

bool HeaterConfig::operator==(const HeaterConfig& rhs) const
{
  return (label == rhs.label &&
          enable == rhs.enable &&
          force == rhs.force &&
          forcelevel == rhs.forcelevel &&
          limit == rhs.limit &&
          target == rhs.target &&
          sensor == rhs.sensor &&
          (fabs(pterm - rhs.pterm) < CONF_FLT_DELTA) &&
          (fabs(iterm - rhs.iterm) < CONF_FLT_DELTA) &&
          (fabs(dterm - rhs.dterm) < CONF_FLT_DELTA) &&
          itermlimit == rhs.itermlimit &&
          ramp == rhs.ramp &&
          ramprate == rhs.ramprate &&
          updatetime == rhs.updatetime);
}

bool HeaterConfig::operator!=(const HeaterConfig& rhs) const
{
  return !(*this == rhs);
}

SensorConfig::SensorConfig() :
  label(""),
  type(0),
  current(0),
  lowerlimit(0.0),
  upperlimit(0.0),
  filter(0)
{}

SensorConfig::SensorConfig(const std::string& label,
                           uint32_t type,
                           uint32_t current,
                           double lowerlimit,
                           double upperlimit,
                           uint32_t filter) :
  label(label),
  type(type),
  current(current),
  lowerlimit(lowerlimit),
  upperlimit(upperlimit),
  filter(filter)
{}

SensorConfig::~SensorConfig()
{}

bool SensorConfig::operator==(const SensorConfig& rhs) const
{
  return (label == rhs.label &&
          type == rhs.type &&
          current == rhs.current &&
          (fabs(lowerlimit - rhs.lowerlimit) < CONF_FLT_DELTA) &&
          (fabs(upperlimit - rhs.upperlimit) < CONF_FLT_DELTA) &&
          filter == rhs.filter);
}

bool SensorConfig::operator!=(const SensorConfig& rhs) const
{
  return !(*this == rhs);
}

BiasConfig::BiasConfig() :
  label(""),
  voltage(0.0),
  enable(0),
  order(0)
{}

BiasConfig::BiasConfig(const std::string& label,
                       double voltage,
                       uint32_t enable,
                       uint32_t order) :
  label(label),
  voltage(voltage),
  enable(enable),
  order(order)
{}

BiasConfig::~BiasConfig()
{}

bool BiasConfig::operator==(const BiasConfig& rhs) const
{
  return (label == rhs.label &&
          (fabs(voltage - rhs.voltage) < CONF_FLT_DELTA) &&
          enable == rhs.enable &&
          order == rhs.order);
}

bool BiasConfig::operator!=(const BiasConfig& rhs) const
{
  return !(*this == rhs);
}

Driver::Driver(const char* host, unsigned port) :
  _host(host),
  _port(port),
  _socket(-1),
  _connected(false),
  _timeout_req(false),
  _acq_mode(Stopped),
  _msgref(0),
  _readbuf_sz(BUFFER_SIZE),
  _writebuf_sz(BUFFER_SIZE),
  _end_frame(0),
  _last_frame(0),
  _batched_ts(false),
  _pending_cfg(false),
  _sleep_enabled(true),
  _system(MODULE_MAX),
  _buffer_info(NUM_BUFFERS)
{
  // 1ms frame poll interval by default
  _sleep_time.tv_sec = 0;
  _sleep_time.tv_nsec = 1000000U; // 1ms
  _readbuf = new char[_readbuf_sz];
  _writebuf = new char[_writebuf_sz];
  _message = &_readbuf[MSG_HEADER_LEN];
  connect();
  // check firmware version
  if (fetch_system()) {
    _batched_ts = (_system.build() >= MIN_BATCH_TS_VER);
  }
}

Driver::~Driver()
{
  disconnect();
  if (_readbuf) {
    delete[] _readbuf;
  }
  if (_writebuf) {
    delete[] _writebuf;
  }
}

AcqMode Driver::acquisition_mode()
{
  if (_end_frame && _last_frame >= _end_frame) {
    _acq_mode = Stopped;
  }

  return _acq_mode;
}

void Driver::connect()
{
  if (!_connected) {
    if (_socket < 0) {
      _socket = ::socket(AF_INET, SOCK_STREAM, 0);
    }
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

void Driver::disconnect()
{
  _connected = false;
  if (_socket >= 0) {
    ::close(_socket);
    _socket = -1;
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
    return load_config_file(f);
  } else {
    fprintf(stderr, "Error opening %s\n", filepath);
    return false;
  }
}

bool Driver::configure(void* buffer, size_t size)
{
  if (!_connected) {
    connect();
    if (!_connected) {
      fprintf(stderr, "Error: unable to load Archon configuration file: controller is not connected!\n");
      return false;
    }
  }

  printf("Attempting to load Archon configuration file from memory\n");

  FILE* f = fmemopen(buffer, size, "r");
  if (f) {
    return load_config_file(f);
  } else {
    fprintf(stderr, "Error opening in memory file!\n");
    return false;
  }
}

bool Driver::load_config_file(FILE* f)
{
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
  uint64_t re_timestamp = 0;
  uint64_t fe_timestamp = 0;
  uint64_t fetch = 0;
  uint32_t batch = _config.linescan();
  uint32_t width = 0;
  uint32_t height = 0;
  bool is32bit = false;
  unsigned buffer_idx = _buffer_info.frame_ready(frame_number);
  if (buffer_idx) {
    fetch = _buffer_info.fetch_time();
    timestamp = _buffer_info.timestamp(buffer_idx);
    re_timestamp = _buffer_info.re_timestamp(buffer_idx);
    fe_timestamp = _buffer_info.fe_timestamp(buffer_idx);
    width = _buffer_info.width(buffer_idx);
    height = _buffer_info.height(buffer_idx);
    is32bit = _buffer_info.is32bit(buffer_idx);
    size = fetch_buffer(buffer_idx, data, batch && _batched_ts);
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
    frame_meta->re_timestamp = re_timestamp;
    frame_meta->fe_timestamp = fe_timestamp;
    frame_meta->fetch = fetch;
    frame_meta->batch = batch;
    frame_meta->width = width;
    frame_meta->height = height;
    frame_meta->is32bit = is32bit;
    frame_meta->size = size;
  }

  _last_frame = frame_number;

  return true;
}

bool Driver::flush_frame(void* data, FrameMetaData* frame_meta)
{
  if (!fetch_buffer_info()) {
    fprintf(stderr, "Error fetching frame buffer info from controller!\n");
    return false;
  }

  ssize_t size = 0;
  uint64_t timestamp = 0;
  uint64_t re_timestamp = 0;
  uint64_t fe_timestamp = 0;
  uint64_t fetch = 0;
  uint32_t batch = _config.linescan();
  uint32_t width = 0;
  uint32_t height = 0;
  bool is32bit = false;
  uint32_t frame_number = 0;
  unsigned buffer_idx = _buffer_info.write_buffer();
  if (!_buffer_info.complete(buffer_idx)) {
    fetch = _buffer_info.fetch_time();
    frame_number = _buffer_info.frame_num(buffer_idx);
    timestamp = _buffer_info.timestamp(buffer_idx);
    re_timestamp = _buffer_info.re_timestamp(buffer_idx);
    fe_timestamp = _buffer_info.fe_timestamp(buffer_idx);
    width = _buffer_info.width(buffer_idx);
    height = _buffer_info.height(buffer_idx);
    is32bit = _buffer_info.is32bit(buffer_idx);
    size = fetch_buffer(buffer_idx, data, batch && _batched_ts);
    if (size != _buffer_info.size(buffer_idx)) {
      fprintf(stderr, "Error fetching frame %u: unexpected size %lu vs expected of %u\n", frame_number, size, _buffer_info.size(buffer_idx));
      return false;
    }

    if (frame_meta) {
      frame_meta->number = frame_number;
      frame_meta->timestamp = timestamp;
      frame_meta->re_timestamp = re_timestamp;
      frame_meta->fe_timestamp = fe_timestamp;
      frame_meta->fetch = fetch;
      frame_meta->batch = batch;
      frame_meta->width = width;
      frame_meta->height = height;
      frame_meta->is32bit = is32bit;
      frame_meta->size = size;
    }

    _last_frame = frame_number;

    return true;
  }

  return false;
}

bool Driver::wait_frame(void* data, FrameMetaData* frame_meta, int timeout)
{
  bool waiting = true;
  uint32_t newest_frame = 0;
  timespec start_time;
  timespec current_time;
  clock_gettime(CLOCK_REALTIME, &start_time);
  while((acquisition_mode() != Stopped) && !_timeout_req && waiting && fetch_buffer_info()) {
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
    if (_sleep_enabled) nanosleep(&_sleep_time, NULL);
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
  _last_frame = _buffer_info.latest_frame();

  if (num_frames) {
    status = load_parameter("Exposures", num_frames);
    _end_frame = _last_frame + num_frames;
    _acq_mode = Fixed;
  } else {
    status = load_parameter("ContinuousExposures", 1U);
    _end_frame = 0;
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

bool Driver::clear_acquisition()
{
  if (_config.linescan()) {
    if (stop_acquisition()) {
      // If we are in linescan mode toggle it to clear any partial frames
      if (set_linescan_mode(false)) {
        return set_linescan_mode(true);
      } else {
        return false;
      }
    } else {
      return false;
    }
  } else {
    return stop_acquisition();
  }
}

bool Driver::wait_power_mode(PowerMode mode, int timeout)
{
  bool waiting = true;

  timespec start_time;
  timespec current_time;
  clock_gettime(CLOCK_REALTIME, &start_time);
  while(!_timeout_req && waiting && fetch_status()) {
    if (mode == _status.power()) {
      waiting = false;
    } else {
      clock_gettime(CLOCK_REALTIME, &current_time);
      if ((timeout > 0) && (diff_ms(&current_time, &start_time) > timeout)) {
        printf("wait_power mode timed out after %d ms!\n", timeout);
        break;
      }
      if (_sleep_enabled) nanosleep(&_sleep_time, NULL);
    }
  }

  return !waiting;
}

bool Driver::power_on()
{
  if (fetch_status()) {
    if (_status.power() == Pds::Archon::On) {
      return true;
    } else {
      if (_pending_cfg && !command("APPLYCDS")) {
        return false;
      }
      _pending_cfg = false;
      return command("POWERON");
    }
  } else {
    return false;
  }
}

bool Driver::power_off()
{
  if (fetch_status()) {
    if (_status.power() == Pds::Archon::Off)
      return true;
    else
      return command("POWEROFF");
  } else {
    return false;
  }
}

bool Driver::set_number_of_lines(unsigned num_lines, unsigned num_batch, bool reload)
{
  if (edit_config_line("LINESCAN", num_batch ? 1 : 0) &&
      edit_config_line("LINECOUNT", num_batch ? num_batch * num_lines: num_lines)) {
    if (reload) {
      if (!command("APPLYCDS"))
        return false;
    }

    return load_parameter("Lines", num_lines);
  } else {
    return false;
  }
}

bool Driver::set_number_of_pixels(unsigned num_pixels, bool reload)
{
  if (edit_config_line("PIXELCOUNT", num_pixels)) {
    if (reload) {
      if (!command("APPLYCDS"))
        return false;
    }

    return load_parameter("Pixels", num_pixels + 1);
  } else {
    return false;
  }
}

bool Driver::set_vertical_binning(unsigned binning)
{
  return load_parameter("VerticalBinning", binning);
}

bool Driver::set_horizontal_binning(unsigned binning)
{
  return load_parameter("HorizontalBinning", binning);
}

bool Driver::set_preframe_clear(unsigned num_lines)
{
  return load_parameter("PreSweepCount", num_lines);
}

bool Driver::set_idle_clear(unsigned num_lines)
{
  return load_parameter("IdleSweepCount", num_lines);
}

bool Driver::set_preframe_skip(unsigned num_lines)
{
  return load_parameter("PreSkipLines", num_lines);
}

bool Driver::set_integration_time(unsigned milliseconds)
{
  return load_parameter("IntMS", milliseconds);
}

bool Driver::set_non_integration_time(unsigned milliseconds)
{
  return load_parameter("NoIntMS", milliseconds);
}

bool Driver::set_waiting_time(unsigned milliseconds)
{
  return load_parameter("WaitMS", milliseconds);
}

bool Driver::set_linescan_mode(bool enable, bool reload)
{
  if (edit_config_line("LINESCAN", enable ? 1 : 0)) {
    if (reload) {
      if (!command("APPLYCDS"))
        return false;
    }

    return true;
  } else {
    return false;
  }
}

bool Driver::set_external_trigger(bool enable, bool reload)
{
  if (edit_config_line("TRIGINENABLE", enable ? 1 : 0)) {
    if (reload) {
      if (!command("APPLYSYSTEM"))
        return false;
    }

    return load_parameter("ExternalTrigger", enable ? 1 : 0);
  } else {
    return false;
  }
}

bool Driver::set_trigger_out(bool force, bool level, bool invert, bool reload)
{
  if (edit_config_line("TRIGOUTFORCE", force ? 1 : 0) &&
      edit_config_line("TRIGOUTLEVEL", level ? 1 : 0) &&
      edit_config_line("TRIGOUTINVERT", invert ? 1 : 0)) {
    if (reload) {
      if (!command("APPLYSYSTEM"))
        return false;
    }

    return true;
  } else {
    return false;
  }
}

bool Driver::set_clock_at(unsigned ticks)
{
  return load_parameter("AT", ticks);
}

bool Driver::set_clock_st(unsigned ticks)
{
  return load_parameter("ST", ticks);
}

bool Driver::set_clock_stm1(unsigned ticks)
{
  return load_parameter("STM1", ticks);
}

bool Driver::set_pocket_pump(unsigned count)
{
  return load_parameter("PumpCount", count);
}

bool Driver::set_bias_config(int channel, BiasConfig* bias, bool reload, bool fetch)
{
  char buffer[32];
  int module = -1;
  size_t base_len;
  char* modify = buffer;

  if (bias) {
    if (!fetch || fetch_system()) {
      for (int i=1; i<=_system.num_modules(); i++) {
        if (_system.module_type(i) == XV_MOD_TYPE) {
          module = i;
          break;
        }
      }

      if (module > 0) {
        snprintf(buffer, sizeof(buffer), "MOD%d/XV%s_", module, channel>0 ? "P" : "N");
        base_len = strlen(buffer);
        modify += base_len;

        snprintf(modify, sizeof(buffer) - base_len, "LABEL%d", abs(channel));
        if (!edit_config_line(buffer, bias->label.c_str())) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "V%d", abs(channel));
        if (!edit_config_line(buffer, bias->voltage)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "ENABLE%d", abs(channel));
        if (!edit_config_line(buffer, bias->enable)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "ORDER%d", abs(channel));
        if (!edit_config_line(buffer, bias->order)) {
          return false;
        }

        if (reload) {
          if (fetch_status()) {
            if (_status.power() == Pds::Archon::On) {
              // if power is on apply module settings
              snprintf(buffer, sizeof(buffer), "APPLYMOD%02X", module-1);
              return command(buffer);
            } else {
              // since power is off we'll need to apply modules settings on power on
              _pending_cfg = true;
              // if power is not on but also not off that is an error state
              return _status.power() <= Pds::Archon::Off;
            }
          }
        } else {
          return true;
        }
      }
    }
  }

  return false;
}

bool Driver::get_bias_config(int channel, BiasConfig* bias, bool fetch)
{
  char buffer[32];
  int module = -1;
  size_t base_len;
  char* modify = buffer;

  if (bias) {
    if (!fetch || fetch_system()) {
      for (int i=1; i<=_system.num_modules(); i++) {
        if (_system.module_type(i) == XV_MOD_TYPE) {
          module = i;
          break;
        }
      }

      if (module > 0) {
        snprintf(buffer, sizeof(buffer), "MOD%d/XV%s_", module, channel>0 ? "P" : "N");
        base_len = strlen(buffer);
        modify += base_len;

        snprintf(modify, sizeof(buffer) - base_len, "LABEL%d", abs(channel));
        bias->label = _config.get_value(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "V%d", abs(channel));
        bias->voltage = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "ENABLE%d", abs(channel));
        bias->enable = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "ORDER%d", abs(channel));
        bias->order = _config.get_value_as_uint32(buffer);

        return true;
      }
    }
  }

  return false;
}

bool Driver::get_bias(int channel, float* voltage, float* current, bool fetch)
{
  char buffer[32];
  int module = -1;
  if (!fetch || (fetch_system() && fetch_status())) {
    for (int i=1; i<=_system.num_modules(); i++) {
      if (_system.module_type(i) == XV_MOD_TYPE) {
        module = i;
        break;
      }
    }

    if (module > 0) {
      snprintf(buffer, sizeof(buffer), "XV%s", channel>0 ? "P" : "N");
      if (voltage)
        *voltage = _status.get_module_voltage(module, buffer, abs(channel));
      if (current)
        *current = _status.get_module_current(module, buffer, abs(channel));
      return true;
    }
  }

  return false;
}

bool Driver::get_heater_pid(char name, char term, uint32_t* value, bool fetch)
{
  int module = -1;
  if (!fetch || (fetch_system() && fetch_status())) {
    for (int i=1; i<=_system.num_modules(); i++) {
      if (_system.module_type(i) == HEATERX_MOD_TYPE) {
        module = i;
        break;
      }
    }

    if (module > 0) {
      if (value)
        *value = _status.get_heater_pid(module, name, term);
      return true;
    }
  }

  return false;
}

bool Driver::get_heater_output(char name, double* output, bool fetch)
{
  int module = -1;
  if (!fetch || (fetch_system() && fetch_status())) {
    for (int i=1; i<=_system.num_modules(); i++) {
      if (_system.module_type(i) == HEATERX_MOD_TYPE) {
        module = i;
        break;
      }
    }

    if (module > 0) {
      if (output)
        *output = _status.get_heater_output(module, name);
      return true;
    }
  }

  return false;
}

bool Driver::get_sensor_temp(char name, double* temp, bool fetch)
{
  int module = -1;
  if (!fetch || (fetch_system() && fetch_status())) {
    for (int i=1; i<=_system.num_modules(); i++) {
      if (_system.module_type(i) == HEATERX_MOD_TYPE) {
        module = i;
        break;
      }
    }

    if (module > 0) {
      if (temp)
        *temp = _status.get_sensor_temp(module, name);
      return true;
    }
  }

  return false;
}

bool Driver::set_heater_config(char name, HeaterConfig* heater, bool reload, bool fetch)
{
  char buffer[32];
  int module = -1;
  size_t base_len;
  char* modify = buffer;

  if (heater) {
    if (!fetch || fetch_system()) {
      for (int i=1; i<=_system.num_modules(); i++) {
        if (_system.module_type(i) == HEATERX_MOD_TYPE) {
          module = i;
          break;
        }
      }

      if (module > 0) {
        snprintf(buffer, sizeof(buffer), "MOD%d/HEATERUPDATETIME", module);
        if (!edit_config_line(buffer, heater->updatetime)) {
          return false;
        }

        snprintf(buffer, sizeof(buffer), "MOD%d/HEATER%c", module, name);
        base_len = strlen(buffer);
        modify += base_len;

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LABEL");
        if (!edit_config_line(buffer, heater->label.c_str())) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "ENABLE");
        if (!edit_config_line(buffer, heater->enable)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "FORCE");
        if (!edit_config_line(buffer, heater->force)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "FORCELEVEL");
        if (!edit_config_line(buffer, heater->forcelevel)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LIMIT");
        if (!edit_config_line(buffer, heater->limit)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "TARGET");
        if (!edit_config_line(buffer, heater->target)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "SENSOR");
        if (!edit_config_line(buffer, heater->sensor)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "P");
        if (!edit_config_line(buffer, heater->pterm)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "I");
        if (!edit_config_line(buffer, heater->iterm)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "D");
        if (!edit_config_line(buffer, heater->dterm)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "IL");
        if (!edit_config_line(buffer, heater->itermlimit)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "RAMP");
        if (!edit_config_line(buffer, heater->ramp)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "RAMPRATE");
        if (!edit_config_line(buffer, heater->ramprate)) {
          return false;
        }

        if (reload) {
          snprintf(buffer, sizeof(buffer), "APPLYMOD%02X", module-1);
          return command(buffer);
        } else {
          return true;
        }
      }
    }
  }

  return false;
}

bool Driver::get_heater_config(char name, HeaterConfig* heater, bool fetch)
{
  char buffer[32];
  int module = -1;
  size_t base_len;
  char* modify = buffer;

  if (heater) {
    if (!fetch || fetch_system()) {
      for (int i=1; i<=_system.num_modules(); i++) {
        if (_system.module_type(i) == HEATERX_MOD_TYPE) {
          module = i;
          break;
        }
      }

      if (module > 0) {
        snprintf(buffer, sizeof(buffer), "MOD%d/HEATERUPDATETIME", module);
        heater->updatetime = _config.get_value_as_uint32(buffer);

        snprintf(buffer, sizeof(buffer), "MOD%d/HEATER%c", module, name);
        base_len = strlen(buffer);
        modify += base_len;

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LABEL");
        heater->label = _config.get_value(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "ENABLE");
        heater->enable = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "FORCE");
        heater->force = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "FORCELEVEL");
        heater->forcelevel = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LIMIT");
        heater->limit = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "TARGET");
        heater->target = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "SENSOR");
        heater->sensor = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "P");
        heater->pterm = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "I");
        heater->iterm = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "D");
        heater->dterm = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "IL");
        heater->itermlimit = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "RAMP");
        heater->ramp = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "RAMPRATE");
        heater->ramprate = _config.get_value_as_uint32(buffer);

        return true;
      }
    }
  }

  return false;
}

bool Driver::set_sensor_config(char name, SensorConfig* sensor, bool reload, bool fetch)
{
  char buffer[32];
  int module = -1;
  size_t base_len;
  char* modify = buffer;

  if (sensor) {
    if (!fetch || fetch_system()) {
      for (int i=1; i<=_system.num_modules(); i++) {
        if (_system.module_type(i) == HEATERX_MOD_TYPE) {
          module = i;
          break;
        }
      }

      if (module > 0) {
        snprintf(buffer, sizeof(buffer), "MOD%d/SENSOR%c", module, name);
        base_len = strlen(buffer);
        modify += base_len;

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LABEL");
        if (!edit_config_line(buffer, sensor->label.c_str())) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "TYPE");
        if (!edit_config_line(buffer, sensor->type)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "CURRENT");
        if (!edit_config_line(buffer, sensor->current)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LOWERLIMIT");
        if (!edit_config_line(buffer, sensor->lowerlimit)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "UPPERLIMIT");
        if (!edit_config_line(buffer, sensor->upperlimit)) {
          return false;
        }

        snprintf(modify, sizeof(buffer) - base_len, "%s", "FILTER");
        if (!edit_config_line(buffer, sensor->filter)) {
          return false;
        }

        if (reload) {
          snprintf(buffer, sizeof(buffer), "APPLYMOD%02X", module-1);
          return command(buffer);
        } else {
          return true;
        }
      }
    }
  }

  return false;
}

bool Driver::get_sensor_config(char name, SensorConfig* sensor, bool fetch)
{
  char buffer[32];
  int module = -1;
  size_t base_len;
  char* modify = buffer;

  if (sensor) {
    if (!fetch || fetch_system()) {
      for (int i=1; i<=_system.num_modules(); i++) {
        if (_system.module_type(i) == HEATERX_MOD_TYPE) {
          module = i;
          break;
        }
      }

      if (module > 0) {
        snprintf(buffer, sizeof(buffer), "MOD%d/SENSOR%c", module, name);
        base_len = strlen(buffer);
        modify += base_len;

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LABEL");
        sensor->label = _config.get_value(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "TYPE");
        sensor->type = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "CURRENT");
        sensor->current = _config.get_value_as_uint32(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "LOWERLIMIT");
        sensor->lowerlimit = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "UPPERLIMIT");
        sensor->upperlimit = _config.get_value_as_double(buffer);

        snprintf(modify, sizeof(buffer) - base_len, "%s", "FILTER");
        sensor->filter = _config.get_value_as_uint32(buffer);

        return true;
      }
    }
  }

  return false;
}

void Driver::timeout_waits(bool request_timeout)
{
  _timeout_req = request_timeout;
}

void Driver::set_frame_poll_interval(unsigned microseconds)
{
  _sleep_enabled = (microseconds != 0);
  _sleep_time.tv_sec = microseconds / 1000000U;
  _sleep_time.tv_nsec = (microseconds % 1000000U) * 1000U;
}

bool Driver::has_batched_timestamps() const
{
  return _batched_ts;
}

ssize_t Driver::fetch_buffer(unsigned buffer_idx, void* data, bool batched_ts)
{
  ssize_t len;
  ssize_t ret;
  ssize_t wbytes = 0;
  char* data_ptr = (char*) data;
  char buf[16];
  char cmdstr[23 + MSG_HEADER_LEN];
  uint32_t base = _buffer_info.base(buffer_idx);
  uint32_t size = _buffer_info.size(buffer_idx);
  uint32_t blocks = (size + BURST_LEN -1) / BURST_LEN;

  if (!lock_buffer(buffer_idx, batched_ts)) {
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

  if (!lock_buffer(0, batched_ts)) {
    fprintf(stderr, "Error unlocking buffer: %u\n", buffer_idx);
  }

  return wbytes;
}

bool Driver::command(const char* cmd)
{
  int len;
  char buf[16];
  char cmdstr[strlen(cmd) + MSG_HEADER_LEN + 1];

  if (!_connected) {
    fprintf(stderr, "Error: unable to send command to Archon: controller is not connected!\n");
    return false;
  }

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

  if (fast) {
    snprintf(_writebuf, BUFFER_SIZE, "FASTLOADPARAM %s %u", param, value);
  } else {
    replace_param_line(param, value);
    snprintf(_writebuf, BUFFER_SIZE, "LOADPARAM %s", param);
  }

  return command(_writebuf);
}

const char* Driver::rd_config_line(unsigned num)
{
  snprintf(_writebuf, BUFFER_SIZE, "RCONFIG%04X", num);
  if (command(_writebuf)) {
    char* msg_ptr = _message + strlen(_message) - 1;
    if (*msg_ptr == '\n')
      *msg_ptr = 0;
  } else {
    *_message = 0;
  }

  return _message; 
}

bool Driver::wr_config_line(unsigned num, const char* line, bool cache)
{
  if (strlen(line) > MAX_CONFIG_LINE) {
    fprintf(stderr, "Configuration line %d is longer than controller max of %d\n", num, MAX_CONFIG_LINE);
    return false;
  }

  if (cache) {
    char buffer[strlen(line)+1];
    strcpy(buffer, line);
    _config.update(buffer);
    _config.cache(buffer, num);
  }
  snprintf(_writebuf, BUFFER_SIZE, "WCONFIG%04X%s", num, line);
  return command(_writebuf);
}

bool Driver::replace_param_line(const char* param, unsigned value, bool use_cache)
{
  char* par = NULL;
  int line_num = -1;
  char newline[MAX_CONFIG_LINE+1];
  newline[MAX_CONFIG_LINE] = 0;
  if (use_cache) {
    snprintf(newline, sizeof(newline), "PARAMETER%d", _config.get_parameter_num(param));
    line_num = _config.get_cache(newline);
    par = newline + strlen(newline);
    *par++ = '=';
  } else {
    char search[strlen(param) + 2];
    snprintf(search, sizeof(search), "%s=", param);
    for (int l=0; l < MAX_CONFIG_LINE; l++) {
      strncpy(newline, rd_config_line(l), MAX_CONFIG_LINE);
      if (!strncmp("PARAMETER", newline, 9)) {
        par = strchr(newline, '=');
        if (!strncmp(search, ++par, strlen(search))) {
          line_num = l;
          break;
        }
      }
    }
  }

  if (line_num < 0) {
    fprintf(stderr, "No configuration parameter line found matching '%s'\n", param);
    return false;
  } else {
    snprintf(par, MAX_CONFIG_LINE - (par-newline), "%s=%u", param, value);
    return wr_config_line(line_num, newline);
  }
}

bool Driver::replace_config_line(const char* key, const char* newline)
{
  int num = find_config_line(key);
  if (num < 0) {
    fprintf(stderr, "No configuration line found matching '%s'\n", key);
    return false;
  } else {
    return wr_config_line(num, newline);
  }
}

bool Driver::edit_config_line(const char* key, const char* value)
{
  char newline[MAX_CONFIG_LINE+1];
  snprintf(newline, sizeof(newline), "%s=%s", key, value);
  return replace_config_line(key, newline);
}

bool Driver::edit_config_line(const char* key, unsigned value)
{
  char newline[MAX_CONFIG_LINE+1];
  snprintf(newline, sizeof(newline), "%s=%u", key, value);
  return replace_config_line(key, newline);
}

bool Driver::edit_config_line(const char* key, signed value)
{
  char newline[MAX_CONFIG_LINE+1];
  snprintf(newline, sizeof(newline), "%s=%d", key, value);
  return replace_config_line(key, newline);
}

bool Driver::edit_config_line(const char* key, double value)
{
  char newline[MAX_CONFIG_LINE+1];
  snprintf(newline, sizeof(newline), "%s=%.3f", key, value);
  return replace_config_line(key, newline);
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

bool Driver::fetch_config()
{
  char buffer[MAX_CONFIG_LINE+1];
  buffer[MAX_CONFIG_LINE] = 0;
  for (unsigned i=0;i<MAX_CONFIG_LINE;i++) {
    strncpy(buffer, rd_config_line(i), MAX_CONFIG_LINE);
    _config.update(buffer);
  }

  return true;
}

int Driver::find_config_line(const char* line, bool use_cache)
{
  int line_num = -1;
  if (use_cache) {
    line_num = _config.get_cache(line);
  } else {
    char search[strlen(line) + 2];
    snprintf(search, sizeof(search), "%s=", line);
    for (int l=0; l < MAX_CONFIG_LINE; l++) {
      if (!strncmp(search, rd_config_line(l), strlen(search))) {
        line_num = l;
        break;
      }
    }
  }

  return line_num;
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

const uint32_t Driver::last_frame() const
{
  return _last_frame;
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

const Config& Driver::config() const
{
  return _config;
}

bool Driver::lock_buffer(unsigned buffer_idx, bool batched_ts)
{
  if (buffer_idx > _buffer_info.nbuffers()) {
    fprintf(stderr, "Invalid buffer number for locking: %u\n", buffer_idx);
    return false;
  } else {
    char cmdstr[6];
    sprintf(cmdstr, batched_ts ? "LOCKT%u" : "LOCK%u", buffer_idx);
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
#undef XV_MOD_TYPE
#undef HEATERX_MOD_TYPE
#undef CONF_FLT_DELTA
#undef MIN_BATCH_TS_VER
