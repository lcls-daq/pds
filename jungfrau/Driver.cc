#include "Driver.hh"
#include "DataFormat.hh"
#include "slsDetectorUsers.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CMD_LEN 128
#define MSG_LEN 256
#define ENDPOINT_LEN 64
#define ADCPHASE_HALF 72
#define ADCPHASE_QUARTER 25
#define CLKDIV_HALF 1
#define CLKDIV_QUARTER 2

// Temporary fixed sizes :(
#define NUM_ROWS 512
#define NUM_COLUMNS 1024

#define get_command_print(cmd, ...) \
  { printf("cmd_get %s: %s\n", cmd, get_command(cmd, ##__VA_ARGS__).c_str()); }

#define put_command_print(cmd, val, ...) \
  { printf("cmd_put %s: %s\n", cmd, put_command(cmd, val, ##__VA_ARGS__).c_str()); }

#define get_register_print(cmd, val, ...) \
  { printf("reg_gett %#x: %s\n", cmd, get_register(cmd, val, ##__VA_ARGS__).c_str()); }

#define put_register_print(cmd, val, ...) \
  { printf("reg_put %#x - %#x: %s\n", cmd, val, put_register(cmd, val, ##__VA_ARGS__).c_str()); }

#define put_adcreg_print(cmd, val, ...) \
  { printf("adc_put %#x - %#x: %s\n", cmd, val, put_adcreg(cmd, val, ##__VA_ARGS__).c_str()); }

#define setbit_print(cmd, val, ...) \
  { printf("setbit %#x - %d: %s\n", cmd, val, setbit(cmd, val, ##__VA_ARGS__).c_str()); }

#define clearbit_print(cmd, val, ...) \
  { printf("clearbit %#x - %d: %s\n", cmd, val, clearbit(cmd, val, ##__VA_ARGS__).c_str()); }

#define error_print(fmt, ...) \
  { snprintf(_msgbuf, MSG_LEN, fmt, ##__VA_ARGS__); fprintf(stderr, _msgbuf); }

using namespace Pds::Jungfrau;

static const char* ADCREG   = "adcreg";
static const char* REG      = "reg";
static const char* SETBIT   = "setbit";
static const char* CLEARBIT = "clearbit";

struct frame_thread_args {
  uint64_t* frame;
  uint16_t* data;
  JungfrauModInfoType* metadata; 
  Module* module;
  bool status;
};

static void* frame_thread(void* args_ptr)
{
  frame_thread_args* args = (frame_thread_args*) args_ptr;
  args->status = args->module->get_frame(args->frame, args->metadata, args->data);

  return NULL;
}

DacsConfig::DacsConfig() :
  _vb_ds(0),
  _vb_comp(0),
  _vb_pixbuf(0),
  _vref_ds(0),
  _vref_comp(0),
  _vref_prech(0),
  _vin_com(0),
  _vdd_prot(0)
{}

/*DacsConfig::DacsConfig(const DacsConfig& rhs) :
  _vb_ds(rhs.vb_ds()),
  _vb_comp(rhs.vb_comp()),
  _vb_pixbuf(rhs.vb_pixbuf()),
  _vref_ds(rhs.vref_ds()),
  _vref_comp(rhs.vref_comp()),
  _vref_prech(rhs.vref_prech()),
  _vin_com(rhs.vin_com()),
  _vdd_prot(rhs.vdd_prot())
{}*/

DacsConfig::DacsConfig(uint16_t vb_ds, uint16_t vb_comp, uint16_t vb_pixbuf, uint16_t vref_ds,
                       uint16_t vref_comp, uint16_t vref_prech, uint16_t vin_com, uint16_t  vdd_prot):
  _vb_ds(vb_ds),
  _vb_comp(vb_comp),
  _vb_pixbuf(vb_pixbuf),
  _vref_ds(vref_ds),
  _vref_comp(vref_comp),
  _vref_prech(vref_prech),
  _vin_com(vin_com),
  _vdd_prot(vdd_prot)
{}

DacsConfig::~DacsConfig() {}

bool DacsConfig::operator==(DacsConfig& rhs) const
{
  return equals(rhs);
}

bool DacsConfig::operator!=(DacsConfig& rhs) const
{
  return !equals(rhs);
}

bool DacsConfig::equals(DacsConfig& rhs) const
{
  if (_vb_ds != rhs.vb_ds())
    return false;
  else if (_vb_comp != rhs.vb_comp())
    return false;
  else if (_vb_pixbuf != rhs.vb_pixbuf())
    return false;
  else if (_vref_ds != rhs.vref_ds())
    return false;
  else if (_vref_comp != rhs.vref_comp())
    return false;
  else if (_vref_prech != rhs.vref_prech())
    return false;
  else if (_vin_com != rhs.vin_com())
    return false;
  else if (_vdd_prot != rhs.vdd_prot())
    return false;

  return true;
}

uint16_t DacsConfig::vb_ds() const      { return _vb_ds; }
uint16_t DacsConfig::vb_comp() const    { return _vb_comp; }
uint16_t DacsConfig::vb_pixbuf() const  { return _vb_pixbuf; }
uint16_t DacsConfig::vref_ds() const    { return _vref_ds; }
uint16_t DacsConfig::vref_comp() const  { return _vref_comp; }
uint16_t DacsConfig::vref_prech() const { return _vref_prech; }
uint16_t DacsConfig::vin_com() const    { return _vin_com; }
uint16_t DacsConfig::vdd_prot() const   { return _vdd_prot; }

Module::Module(const int id, const char* control, const char* host, unsigned port, const char* mac, const char* det_ip, bool config_det_ip, void* socket) :
  _id(id), _control(control), _host(host), _port(port), _mac(mac), _det_ip(det_ip),
  _socket(socket), _fd(-1), _connected(false), _boot(true), _freerun(false), _poweron(false), _pending(false),
  _sockbuf_sz(sizeof(jungfrau_dgram)*JF_PACKET_NUM*JF_EVENTS_TO_BUFFER), _readbuf_sz(sizeof(jungfrau_header)),
  _frame_sz(JF_DATA_ELEM * sizeof(uint16_t)), _frame_elem(JF_DATA_ELEM),
  _endpoint(0), _speed(JungfrauConfigType::Quarter)
{
  _readbuf = new char[_readbuf_sz];
  _msgbuf  = new char[MSG_LEN];
  for (int i=0; i<MAX_JUNGFRAU_CMDS; i++) {
    _cmdbuf[i] = new char[CMD_LEN];
  }

  int failed = 0;
  _det = new slsDetectorUsers(failed, _id);
  if (failed) {
    error_print("Error: failed to allocate share memory for Jungfrau control interface of %s!\n", _control);
    // cleanup detector after failure
    delete _det;
    _det = 0;
  } else {
    std::string reply = put_command("hostname", _control);
    // Check if detector control interface is present
    if (!reply.empty()) {
      std::string type  = _det->getDetectorType();

      bool detmac_status = configure_mac(config_det_ip);

      int nb = -1;
      if (_socket) {
        size_t endpoint_len = ENDPOINT_LEN;
        _endpoint = new char[endpoint_len];
        nb = zmq_getsockopt(_socket, ZMQ_LAST_ENDPOINT, _endpoint, &endpoint_len);
      } else {
        hostent* entries = gethostbyname(host);
        if (entries) {
          _fd = ::socket(AF_INET, SOCK_DGRAM, 0);
          ::setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, &_sockbuf_sz, sizeof(unsigned));

          unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);

          sockaddr_in sa;
          sa.sin_family = AF_INET;
          sa.sin_addr.s_addr = htonl(addr);
          sa.sin_port        = htons(port);

          nb = ::bind(_fd, (sockaddr*)&sa, sizeof(sa));
        }
      }

      if (nb<0) {
        if (_socket) {
          error_print("Error: failed to read endpoint of the Jungfrau zmq data receiver: %s\n", strerror(errno));
        } else {
          error_print("Error: failed to bind to Jungfrau data receiver at %s on port %d: %s\n", host, port, strerror(errno));
        }
      } else if (strncmp(control, reply.c_str(), strlen(control)) != 0) {
        error_print("Error: failed to connect to Jungfrau control interface at %s: %s\n", control, reply.c_str());
      } else if (strcmp(type.c_str(), "Jungfrau+") !=0) {
        error_print("Error: detector at %s on port %d is not a Jungfrau: %s\n", host, port, type.c_str());
      } else {
        _connected=detmac_status;
      }
    } else {
      error_print("Error: failed to connect to Jungfrau control interface of %s!\n", _control);
    }
  }
}

Module::~Module()
{
  shutdown();
  if (_readbuf) {
    delete[] _readbuf;
  }
  if (_msgbuf) {
    delete[] _msgbuf;
  }
  if (_endpoint) {
    delete[] _endpoint;
  }
  for (int i=0; i<MAX_JUNGFRAU_CMDS; i++) {
    if (_cmdbuf[i]) delete[] _cmdbuf[i];
  }
}

void Module::shutdown()
{
  if (_socket) {
    zmq_close(_socket);
    _socket = 0;
  }
  if (_fd >= 0) {
    ::close(_fd);
    _fd = -1;
  }
  if (_det) {
    put_command("free", "0");
    delete _det;
    _det = 0;
  }
}

bool Module::allocated() const
{
  if (_det) {
    return true;
  } else {
    return false;
  }
}

bool Module::connected() const
{
  return _connected;
}

bool Module::check_config()
{
  if (!_connected) {
    error_print("Error: failed to connect to Jungfrau at address %s\n", _control);
    return false;
  }

  // stop the detector if not!
  if (status() == RUNNING) {
    if (!stop()) {
      error_print("Error: can't configure when the detector is in a running state\n");
      return false;
    }
  }

  int power_status = 0;
  get_register_print(0x5e, &power_status);
  _boot = (power_status == 0);
  if (_boot) {
    printf("module chips need to be powered on\n");
    if (_poweron) {
      printf("Warning: module has likely been rebooted!\n");
      configure_mac(true);
    }
  } else {
    printf("module chips are already on\n");
  }

  return true;
}

bool Module::configure_mac(bool config_det_ip)
{
  bool needs_config = false;
  bool detmac_status = false;
  std::string rx_ip  = get_command("rx_udpip");
  std::string rx_mac = get_command("rx_udpmac");

  if (!strcmp(rx_ip.c_str(), "none") || !strcmp(rx_mac.c_str(), "none")) {
    printf("detector udp_rx interface appears to be unset\n");
    needs_config = true;
  }

  if (config_det_ip || needs_config) {
      printf("setting up detector udp_rx interface\n");
      put_command_print("rx_udpport", _port);
      put_command_print("rx_udpip",   _host);
      put_command_print("rx_udpmac", _mac);
      put_command_print("detectorip", _det_ip);
      put_command_print("detectormac", "00:aa:bb:cc:dd:ee");
      std::string cfgmac_reply = put_command("configuremac", 0);
      printf("cmd_put configuremac: %s\n", cfgmac_reply.c_str());
      if (!strcmp(cfgmac_reply.c_str(), "3") || !strcmp(cfgmac_reply.c_str(), "0")) {
        detmac_status = true;
        printf("detector udp_rx interface is up\n");
      } else {
        error_print("Error: detector udp_rx interface did not come up\n");
      }
  } else {
    printf("detector udp_rx interface is being set externally\n");
    detmac_status = true;
  }

  return detmac_status;
}

bool Module::configure_dacs(const DacsConfig& dac_config)
{
  if (_boot || dac_config != _dac_config) {
    // Setting Dacs 12bit on 2.5V  (i.e. 2.5v=4096)
    printf("Setting Dacs:\n");
  
    // setting vb_ds
    printf("setting vb_ds to %hu\n", dac_config.vb_ds());
    put_command_print("dac:5", dac_config.vb_ds());

    // setting vb_comp
    printf("setting vb_comp to %hu\n", dac_config.vb_comp());
    put_command_print("dac:0", dac_config.vb_comp());

    // setting vb_pixbuf
    printf("setting vb_pixbuf to %hu\n", dac_config.vb_pixbuf());
    put_command_print("dac:4", dac_config.vb_pixbuf());

    // setting vref_ds
    printf("setting vref_ds to %hu\n", dac_config.vref_ds());
    put_command_print("dac:6", dac_config.vref_ds());

    // setting vref_comp
    printf("setting vref_comp to %hu\n", dac_config.vref_comp());
    put_command_print("dac:7", dac_config.vref_comp());

    // setting vref_prech
    printf("setting vref_prech to %hu\n", dac_config.vref_prech());  
    put_command_print("dac:3", dac_config.vref_prech());

    // setting vin_com
    printf("setting vin_com to %hu\n", dac_config.vin_com());
    put_command_print("dac:2", dac_config.vin_com());

    // setting vdd_prot
    printf("setting vdd_prot to %hu\n", dac_config.vdd_prot());
    put_command_print("dac:1", dac_config.vdd_prot());

    _dac_config = dac_config;
  }

  return status() != ERROR;
}

bool Module::configure_adc()
{
  if (_boot) {
    // power on the chips
    printf("powering on the chip\n");
    put_register_print(0x5e, 0x1);

    // reset adc
    printf("resetting the adc\n");
    put_adcreg_print(0x08, 0x3);
    put_adcreg_print(0x08, 0x0);
    put_adcreg_print(0x14, 0x40);
    put_adcreg_print(0x4, 0xf);
    put_adcreg_print(0x5, 0x3f);
    put_adcreg_print(0x18, 0x2);
    put_register_print(0x43, 0x453b2a9c);
  }

  // initial powerup complete
  _poweron = true;

  return status() != ERROR;
}

bool Module::configure_speed(JungfrauConfigType::SpeedMode speed, bool& sleep)
{
  if (_boot || speed != _speed) {
    // set speed
    switch (speed) {
    case JungfrauConfigType::Quarter:
      printf("setting detector to quarter speed\n");
      put_command_print("clkdivider", CLKDIV_QUARTER);
      printf("setting adcphase to %d\n", ADCPHASE_QUARTER);
      put_command_print("adcphase", ADCPHASE_QUARTER);
      break;
    case JungfrauConfigType::Half:
      printf("setting detector to half speed\n");
      put_command_print("clkdivider", CLKDIV_HALF);
      printf("setting adcphase to %d\n", ADCPHASE_HALF);
      put_command_print("adcphase", ADCPHASE_HALF);
      break;
    default:
      error_print("Error: invalid clock speed setting for the camera %d\n", speed);
      return false;
    } 
    _speed = speed;
    sleep = true;
  }

  return status() != ERROR;
}

bool Module::configure_acquistion(uint64_t nframes, double trig_delay, double exposure_time, double exposure_period)
{
  reset();

  printf("setting trigger delay to %.6f\n", trig_delay);
  put_command("delay", trig_delay);

  if (!nframes) {
    printf("configuring triggered mode\n");
    put_register_print(0x4e, 0x3);
    put_command_print("cycles", 10000000000);
    put_command_print("frames", 1);
    put_command_print("period", exposure_period);
    _freerun = false;
  } else {
    printf("configuring for free run\n");
    put_register_print(0x4e, 0x0);
    put_command_print("cycles", 1);
    put_command_print("frames", nframes);
    put_command_print("period", exposure_period);
    _freerun = true;
  }

  printf("setting exposure time to %.6f seconds\n", exposure_time);
  put_command_print("exptime", 0.000010);

  return status() != ERROR;
}

bool Module::configure_gain(uint32_t bias, JungfrauConfigType::GainMode gain)
{
  printf("setting bias voltage to %d volts\n", bias);
  put_command_print("vhighvoltage", bias);

  printf("setting gain mode %d\n", gain);
  // Clear all the bits
  clearbit_print(0x5d, 0);
  clearbit_print(0x5d, 1);
  clearbit_print(0x5d, 2);
  clearbit_print(0x5d, 12);
  clearbit_print(0x5d, 13);
  // Now set the ones of the gain we want
  switch(gain) {
    case JungfrauConfigType::Normal:
      break;
    case JungfrauConfigType::FixedGain1:
      setbit_print(0x5d, 1);
      break;
    case JungfrauConfigType::FixedGain2:
      setbit_print(0x5d, 1);
      setbit_print(0x5d, 2);
      break;
    case JungfrauConfigType::ForcedGain1:
      setbit_print(0x5d, 12);
      break;
    case JungfrauConfigType::ForcedGain2:
      setbit_print(0x5d, 12);
      setbit_print(0x5d, 13);
      break;
    case JungfrauConfigType::HighGain0:
      setbit_print(0x5d, 0);
      break;
  }

  return status() != ERROR;
}

bool Module::check_size(uint32_t num_rows, uint32_t num_columns) const
{
  return (num_rows == NUM_ROWS) && (num_columns == NUM_COLUMNS);
}

std::string Module::put_command_raw(int narg, int pos)
{
  if(_det) {
    return _det->putCommand(narg, _cmdbuf, pos);
  } else {
    return "unable to send command";
  }
}

std::string Module::get_command_raw(int narg, int pos)
{
  if(_det) {
    return _det->getCommand(narg, _cmdbuf, pos);
  } else {
    return "unable to send command";
  }
}

std::string Module::put_command(const char* cmd, const char* value, int pos)
{
  if (strlen(cmd) >= CMD_LEN || strlen(value) >= CMD_LEN) {
    return "invalid command or value length";
  } 
  strcpy(_cmdbuf[0], cmd);
  strcpy(_cmdbuf[1], value);
  if (value) {
    return put_command_raw(2, pos);
  } else {
    return put_command_raw(1, pos);
  }
}

std::string Module::put_command(const char* cmd, const short value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%hd", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const unsigned short value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%hu", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const int value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%d", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const unsigned int value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%u", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%ld", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const unsigned long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%lu", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const long long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%lld", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const unsigned long long value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%llu", value);
  return put_command_raw(2, pos);
}

std::string Module::put_command(const char* cmd, const double value, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  sprintf(_cmdbuf[1], "%f", value);
  return put_command_raw(2, pos);
}

std::string Module::get_command(const char* cmd, int pos)
{
  if (strlen(cmd) >= CMD_LEN) {
    return "invalid command or value length";
  }
  strcpy(_cmdbuf[0], cmd);
  return get_command_raw(1, pos);
}

std::string Module::get_register(const int reg, int* register_value, int pos)
{
  strcpy(_cmdbuf[0], REG);
  sprintf(_cmdbuf[1], "%#x", reg);
  std::string reply = get_command_raw(2, pos);

  if (register_value) {
    *register_value = (int) strtol(reply.c_str(), NULL, 0);
  }

  return reply;
}

std::string Module::put_register(const int reg, const int value, int pos)
{
  strcpy(_cmdbuf[0], REG);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2], "%#x", value);
  return put_command_raw(3, pos);
}

std::string Module::put_adcreg(const int reg, const int value, int pos)
{
  strcpy(_cmdbuf[0], ADCREG);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2], "%#x", value);
  return put_command_raw(3, pos);
}

std::string Module::setbit(const int reg, const int bit, int pos)
{
  strcpy(_cmdbuf[0], SETBIT);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2],  "%d", bit);
  return put_command_raw(3, pos);
}

std::string Module::clearbit(const int reg, const int bit, int pos)
{
  strcpy(_cmdbuf[0], CLEARBIT);
  sprintf(_cmdbuf[1], "%#x", reg);
  sprintf(_cmdbuf[2],  "%d", bit);  
  return put_command_raw(3, pos);
}

uint64_t Module::nframes()
{
  std::string reply = get_command("nframes");
  return strtoull(reply.c_str(), NULL, 10);
}

uint64_t Module::serialnum()
{
  std::string reply = get_command("detectornumber");
  return strtoull(reply.c_str(), NULL, 0);
}

uint64_t Module::version()
{
  std::string reply = get_command("detectorversion");
  return strtoull(reply.c_str(), NULL, 0);
}

uint64_t Module::firmware()
{
  std::string reply = get_command("softwareversion");
  return strtoull(reply.c_str(), NULL, 0);
}

Module::Status Module::status(const std::string& reply)
{
  if (!strcmp(reply.c_str(), "waiting"))
    return WAIT;
  else if (!strcmp(reply.c_str(), "idle"))
    return IDLE;
  else if (!strcmp(reply.c_str(), "running"))
    return RUNNING;
  else if (!strcmp(reply.c_str(), "data"))
    return DATA;
  else
    return ERROR;
}

Module::Status Module::status()
{
  std::string reply = get_command("status");
  return status(reply);
}

std::string Module::status_str()
{
  return get_command("status");
}

bool Module::start()
{
  std::string reply = put_command("status", "start");
  printf("starting detector: %s\n", reply.c_str());
  /*
   * IDLE status is okay when the detector is free running since the frame acquisiton may have finished
   * before the status call happens. Often this happens in the case of aquiring only a single frame.
   *
   * Triggered mode always runs until explicitly stopped, so being at IDLE after starting aquisiton is
   * an error state.
   */
  return status(reply) == RUNNING || status(reply) == WAIT || ((status(reply) == IDLE) && (_freerun)) ;
}

bool Module::stop()
{
  std::string reply = put_command("status", "stop");
  if (!strcmp(reply.c_str(), "error")) {
    printf("stopping detector:  done\n");
    reset();
  } else {
    printf("stopping detector: %s\n", reply.c_str());
  }
  return status(reply) != RUNNING;
}

void Module::reset()
{
  printf("reseting run control ...");
  // reset mem machine fifos fifos
  put_register(0x4f, 0x4000);
  put_register(0x4f, 0x0);
  // reset run control
  put_register(0x4f, 0x0400);
  put_register(0x4f, 0x0);
  printf(" done\n");
}

void Module::flush()
{
  if (_socket) {
    flush_socket();
  } else {
    flush_fd();
  }
}

void Module::flush_fd()
{
  ssize_t nb;
  struct sockaddr_in clientaddr;
  unsigned count = 0;
  socklen_t clientaddrlen = sizeof(clientaddr);
  do {
    nb = ::recvfrom(_fd, _readbuf, sizeof(jungfrau_header), MSG_DONTWAIT, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (nb > 0) {
      count += 1;
    }
  } while (nb > 0);
  printf("flushed %u packets from socket buffer\n", count);
}

void Module::flush_socket()
{
  int nb;
  int64_t more = 0;
  unsigned count = 0;
  size_t more_size = sizeof more;
  do {
    nb = zmq_recv(_socket, _readbuf, _readbuf_sz, ZMQ_DONTWAIT);
    if (nb > 0) {
      do {
        int rc = zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size);
        if (rc > 0 && more) {
          rc = zmq_recv(_socket, _readbuf, _readbuf_sz, 0);
        }
        // if zmq command failed break out of loop
        if (rc < 0) break;
      } while (more);
      count += 1;
    }
  } while (nb > 0);
  _pending = false;
  printf("flushed %u messages from socket buffer\n", count);
}

bool Module::get_frame(uint64_t* frame, uint16_t* data)
{
  return get_frame(frame, NULL, data);
}

bool Module::get_packet(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data, bool* first_packet, bool* last_packet, unsigned* npackets)
{
  if (_socket) {
    return get_packet_socket(frame, metadata, data, first_packet, last_packet, npackets);
  } else {
    return get_packet_fd(frame, metadata, data, first_packet, last_packet, npackets);
  }
}

bool Module::get_packet_fd(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data, bool* first_packet, bool* last_packet, unsigned* npackets)
{
  int nb;
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen = sizeof(clientaddr);
  jungfrau_header* header = NULL;
  struct iovec dgram_vec[2];

  nb = ::recvfrom(_fd, _readbuf, sizeof(jungfrau_header), MSG_PEEK, (struct sockaddr *)&clientaddr, &clientaddrlen);
  if (nb<0) {
    fprintf(stderr,"Error: failure receiving packet from Jungfru at %s on port %d: %s\n", _host, _port, strerror(errno));
    return false;
  } else {
    header = (jungfrau_header*) _readbuf;

    if (*first_packet) {
      *frame = header->framenum;
      *first_packet = false;
    } else if(*frame != header->framenum) {
      *last_packet = true;
      fprintf(stderr,"Error: data out-of-order got data for frame %lu, but was expecting frame %lu from Jungfru at %s\n", header->framenum, *frame, _host);
      return false;
    }

    dgram_vec[0].iov_base = _readbuf;
    dgram_vec[0].iov_len = sizeof(jungfrau_header);
    dgram_vec[1].iov_base = &data[_frame_elem*header->packetnum];
    dgram_vec[1].iov_len = sizeof(jungfrau_dgram) - sizeof(jungfrau_header);

    nb = ::readv(_fd, dgram_vec, 2);
    if (nb<0) {
      fprintf(stderr,"Error: failure receiving packet from Jungfru at %s on port %d: %s\n", _host, _port, strerror(errno));
      return false;
    }

    *last_packet = (header->packetnum == (JF_PACKET_NUM -1));
    (*npackets)++;
  }

  if ((*npackets == JF_PACKET_NUM) && header) {
    if (metadata) new(metadata) JungfrauModInfoType(header->timestamp, header->exptime, header->moduleID, header->xCoord, header->yCoord, header->zCoord);
  }

  return true;
}

bool Module::get_packet_socket(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data, bool* first_packet, bool* last_packet, unsigned* npackets)
{
  int nb;
  int64_t more = 0;
  size_t more_size = sizeof more;
  jungfrau_header* header = (jungfrau_header*) _readbuf;

  if (!_pending) {
    nb = zmq_recv(_socket, _readbuf, sizeof(jungfrau_header), 0);
    if (nb<0) {
      fprintf(stderr,"Error: failure receiving packet header from Jungfru zmq server at address %s: %s\n", _endpoint, strerror(errno));
      return false;
    } else {
      _pending = true;
    }
  }

  if (*first_packet) {
    *frame = header->framenum;
    *first_packet = false;
  } else if(*frame != header->framenum) {
    *last_packet = true;
    fprintf(stderr,"Error: data out-of-order got data for frame %lu, but was expecting frame %lu from Jungfru at %s\n", header->framenum, *frame, _endpoint);
    return false;
  }

  nb = zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size);
  if (nb < 0) {
    fprintf(stderr,"Error: failure receiving packet payload from Jungfru zmq server at address %s: %s\n", _endpoint, strerror(errno));
    return false;
  } else if (!more) {
    fprintf(stderr,"Error: received malformed packet from Jungfru zmq server at address %s: %s\n", _endpoint, strerror(errno));
    return false;
  } else {
    nb = zmq_recv(_socket, &data[_frame_elem*header->packetnum], sizeof(jungfrau_dgram) - sizeof(jungfrau_header), 0);
    if (nb<0) {
      fprintf(stderr,"Error: failure receiving packet payload from Jungfru zmq server at address %s: %s\n", _endpoint, strerror(errno));
      return false;
    }

    _pending = false;
    *last_packet = (header->packetnum == (JF_PACKET_NUM -1));
    (*npackets)++;
  }

  if ((*npackets == JF_PACKET_NUM) && header) {
    if (metadata) new(metadata) JungfrauModInfoType(header->timestamp, header->exptime, header->moduleID, header->xCoord, header->yCoord, header->zCoord);
  }

  // check for unexpected additional parts for the zmq message
  unsigned count = 0;
  do {
    nb = zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size);
    if (nb > 0 && more) {
      count += 1;
      nb = zmq_recv(_socket, _readbuf, _readbuf_sz, 0);
    }
    if (nb < 0) break;
  } while(more);

  // check if we found any unexpected message parts
  if (count != 0) {
    fprintf(stderr,"Error: received malformed packet with %u extra parts from Jungfru zmq server at address %s: %s\n",
            count, _endpoint, strerror(errno));
    return false;
  }

  return true;
}

bool Module::get_frame(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data)
{
  unsigned npackets = 0;
  uint64_t cur_frame = 0;
  bool first_packet = true;
  bool last_packet = false;

  while (!last_packet) {
    get_packet(&cur_frame, metadata, data, &first_packet, &last_packet, &npackets);
  }

  if (npackets == JF_PACKET_NUM) {
    *frame = cur_frame;
  } else {
    fprintf(stderr,"Error: frame %lu from Jungfrau at %s is incomplete, received %u out of %d expected\n",
            cur_frame, _socket ? _endpoint : _host, npackets, JF_PACKET_NUM);
  }
 
  return (npackets == JF_PACKET_NUM);
}

const char* Module::error()
{
  return _msgbuf;
}

void Module::set_error(const char* fmt, ...)
{
  va_list err_args;
  va_start(err_args, fmt);
  vsnprintf(_msgbuf, MSG_LEN, fmt, err_args);
  va_end(err_args);
  fprintf(stderr, _msgbuf);
}

void Module::clear_error()
{
  _msgbuf[0] = 0;
}

void* Module::socket() const
{
  return _socket;
}

int Module::fd() const
{
  return _fd;
}

unsigned Module::get_num_rows() const
{
  return NUM_ROWS;
}

unsigned Module::get_num_columns() const
{
  return NUM_COLUMNS;
}

unsigned Module::get_num_pixels() const
{
  return NUM_ROWS * NUM_COLUMNS;
}

unsigned Module::get_frame_size() const
{
  return NUM_ROWS * NUM_COLUMNS * sizeof(uint16_t);
}

void* Module::connect_socket(void* context, const char* host, const unsigned device, const unsigned module)
{
  void* socket = NULL;
  if (context) {
    unsigned port = calculate_zmq_port(device, module);
    if (port) {
      socket = zmq_socket(context, ZMQ_PUSH);
      char endpoint[strlen(host) + 30];
      snprintf(endpoint, sizeof(endpoint), "tcp://%s:%u", host, port);
      if (zmq_connect(socket, endpoint) < 0) {
        zmq_close(socket);
        socket = NULL;
      }
    }
  }

  return socket;
}

void* Module::bind_socket(void* context, const char* host, const unsigned device, const unsigned module)
{
  void* socket = NULL;
  if (context) {
    unsigned port = calculate_zmq_port(device, module);
    if (port) {
      socket = zmq_socket(context, ZMQ_PULL);
      char endpoint[strlen(host) + 30];
      snprintf(endpoint, sizeof(endpoint), "tcp://%s:%u", host, port);
      if (zmq_bind(socket, endpoint) < 0) {
        zmq_close(socket);
        socket = NULL;
      }
    }
  }

  return socket;
}

unsigned Module::calculate_zmq_port(const unsigned device, const unsigned module)
{
  if (module < JungfrauConfigType::MaxModulesPerDetector) {
    return ZmqBasePort + JungfrauConfigType::MaxModulesPerDetector * device + module;
  }

  return 0;
}

Detector::Detector(std::vector<Module*>& modules, bool use_threads, int thread_rtprio) :
  _aborted(false),
  _use_threads(use_threads),
  _threads(0),
  _thread_attr(0),
  _pfds(0),
  _num_modules(modules.size()),
  _module_frames(new uint64_t[modules.size()]),
  _module_frames_offset(new uint64_t[modules.size()]),
  _module_first_packet(new bool[modules.size()]),
  _module_last_packet(new bool[modules.size()]),
  _module_npackets(new unsigned[modules.size()]),
  _module_data(new uint16_t*[modules.size()]),
  _modules(modules),
  _msgbuf(new const char*[modules.size()])
{
  int err = ::pipe(_sigfd);
  if (err) {
    fprintf(stderr, "%s pipe error: %s\n", __FUNCTION__, strerror(errno));
  }
  // explicitly zero the _module_frames and _module_frames_offset buffers
  memset(_module_frames, 0, modules.size() * sizeof(uint64_t));
  memset(_module_frames_offset, 0, modules.size() * sizeof(uint64_t));
  if (_use_threads) {
    _threads = new pthread_t[_num_modules];
    _thread_attr = new pthread_attr_t;
    pthread_attr_init(_thread_attr);
    if (thread_rtprio > 0) {
      if (thread_rtprio > sched_get_priority_max(SCHED_FIFO)) {
        thread_rtprio = sched_get_priority_max(SCHED_FIFO);
      } else if (thread_rtprio < sched_get_priority_min(SCHED_FIFO)) {
        thread_rtprio = sched_get_priority_min(SCHED_FIFO);
      }
      struct sched_param params;
      params.sched_priority = thread_rtprio;
      pthread_attr_setschedpolicy(_thread_attr, SCHED_FIFO);
      pthread_attr_setinheritsched(_thread_attr, PTHREAD_EXPLICIT_SCHED);
      pthread_attr_setschedparam(_thread_attr, &params);
    }
  } else {
    _pfds = new zmq_pollitem_t[_num_modules+1];
    _pfds[_num_modules].socket   = 0;
    _pfds[_num_modules].fd       = _sigfd[0];
    _pfds[_num_modules].events   = ZMQ_POLLIN;
    _pfds[_num_modules].revents  = 0;
    for (unsigned i=0; i<_num_modules; i++) {
      _pfds[i].socket   = _modules[i]->socket();
      _pfds[i].fd       = _modules[i]->fd();
      _pfds[i].events   = ZMQ_POLLIN;
      _pfds[i].revents  = 0;
    }
  }
}

Detector::~Detector()
{
  for (unsigned i=0; i<_num_modules; i++) {
    delete _modules[i];
  }
  _modules.clear();
  if (_threads) delete[] _threads;
  if (_thread_attr) {
    pthread_attr_destroy(_thread_attr);
    delete _thread_attr;
  }
  if (_pfds) delete[] _pfds;
  if (_module_frames) delete[] _module_frames;
  if (_module_frames_offset) delete[] _module_frames_offset;
  if (_module_first_packet) delete[] _module_first_packet;
  if (_module_last_packet) delete[] _module_last_packet;
  if (_module_npackets) delete[] _module_npackets;
  if (_module_data) delete[] _module_data;
  if (_msgbuf) delete[] _msgbuf;
}

void Detector::shutdown()
{
  abort(); // kill any active frame waits
  for (unsigned i=0; i<_num_modules; i++) {
    _modules[i]->shutdown();
  }
}

bool Detector::allocated() const
{
  for (unsigned i=0; i<_num_modules; i++) {
    if(!_modules[i]->allocated()) return false;
  }
  return true;
}

bool Detector::connected() const
{
  for (unsigned i=0; i<_num_modules; i++) {
    if(!_modules[i]->connected()) return false;
  }
  return true;
}

void Detector::signal(int sig)
{
  if(!_use_threads) {
    ::write(_sigfd[1], &sig, sizeof(sig));
  }
}

void Detector::abort()
{
  _aborted = true;
  signal(JF_FRAME_WAIT_EXIT);
}

bool Detector::aborted() const
{
  return _aborted;
}

uint64_t Detector::sync_nframes()
{
  uint64_t frame_num[_num_modules];
  uint64_t last_frame = 0;
  for (unsigned i=0; i<_num_modules; i++) {
    frame_num[i] = _modules[i]->nframes();
    if (frame_num[i] > last_frame) {
      last_frame = frame_num[i];
    }
  }

  for (unsigned i=0; i<_num_modules; i++) {
    if (frame_num[i] < last_frame) {
        printf("Warning: module %u is behind by %lu frame%s! - adjusting frame offset\n", i, last_frame - frame_num[i], (last_frame - frame_num[i])==1 ? "" : "s");
        _module_frames_offset[i] = last_frame - frame_num[i];
    }
  }

  return last_frame;
}

bool Detector::check_size(uint32_t num_modules, uint32_t num_rows, uint32_t num_columns) const
{
  for (unsigned i=0; i<_num_modules; i++) {
    if (!_modules[i]->check_size(num_rows, num_columns))
      return false;
  }

  return (num_modules == _num_modules);
}

bool Detector::configure(uint64_t nframes, JungfrauConfigType::GainMode gain, JungfrauConfigType::SpeedMode speed, double trig_delay, double exposure_time, double exposure_period, uint32_t bias, const DacsConfig& dac_config)
{
  bool bsleep = false;
  bool success = true;

  printf("Configuring %u modules\n", _num_modules);
  
  for (unsigned i=0; i<_num_modules; i++) {
    printf("checking status of module %u\n", i);
    if(!_modules[i]->check_config()) {
      fprintf(stderr, "Error: module %u is not in a configurable state!\n", i);
      success = false;
    }
  }

  // If all modules are configurable then continue with config
  if (success) {
    for (unsigned i=0; i<_num_modules; i++) {
      printf("configuring dacs of module %u\n", i);
      if(!_modules[i]->configure_dacs(dac_config)) {
        fprintf(stderr, "Error: module %u dac configuration failed!\n", i);
        success = false;
      }
    }

    for (unsigned i=0; i<_num_modules; i++) {
      printf("configuring adc of module %u\n", i);
      if(!_modules[i]->configure_adc()) {
        fprintf(stderr, "Error: module %u adc configuration failed!\n", i);
        success = false;
      }
    }

    for (unsigned i=0; i<_num_modules; i++) {
      printf("configuring clock speed of module %u\n", i);
      if(!_modules[i]->configure_speed(speed, bsleep)) {
        fprintf(stderr, "Error: module %u clock speed configuration failed!\n", i);
        success = false;
      }
    }

    if (bsleep) {
      // sleeping after reconfiguring clock
      sleep(1);
      bsleep = false;
    }

    for (unsigned i=0; i<_num_modules; i++) {
      printf("configuring acquistion settings of module %u\n", i);
      if(!_modules[i]->configure_acquistion(nframes, trig_delay, exposure_time, exposure_period)) {
        fprintf(stderr, "Error: module %u acquistion settings configuration failed!\n", i);
        success = false;
      }
    }

    for (unsigned i=0; i<_num_modules; i++) {
      printf("configuring gain and bias of module %u\n", i);
      if(!_modules[i]->configure_gain(bias, gain)) {
        fprintf(stderr, "Error: module %u gain and bias configuration failed!\n", i);
        success = false;
      }
    }
  }

  return success;
}

bool Detector::get_frame(uint64_t* frame, uint16_t* data)
{
  return get_frame(frame, NULL, data);
}

bool Detector::get_frame(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data)
{
  // clear get_frame aborted flag
  _aborted = false;
  if (_use_threads)
    return get_frame_thread(frame, metadata, data);
  else
    return get_frame_poll(frame, metadata, data);
}

bool Detector::get_frame_thread(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data)
{
  int thread_ret = 0;
  bool drop_frame = false;
  bool frame_unset = true;
  frame_thread_args args[_num_modules];

  // Start frame reading threads
  for (unsigned i=0; i<_num_modules; i++) {
    args[i].frame = &_module_frames[i];
    args[i].data = data;
    args[i].metadata = metadata;
    args[i].module = _modules[i];
    thread_ret = pthread_create(&_threads[i], _thread_attr, frame_thread, &args[i]);
    if (thread_ret) {
      fprintf(stderr, "Error: failed to launch frame fetching thread for module %u: %s\n", i, strerror(thread_ret));
      return false;
    }

    data += _modules[i]->get_num_pixels();
    if(metadata) metadata++;
  }

  // Wait for the threads to finish
  for (unsigned i=0; i<_num_modules; i++) {
    pthread_join(_threads[i], NULL);
    if (!args[i].status) {
      fprintf(stderr,"Error: module %u failed to return a frame!\n", i);
      drop_frame = true;
    }
  }

  if (!drop_frame) {
    // Check that we got a consistent frame
    for (unsigned i=0; i<_num_modules; i++) {
      if (frame_unset) {
        *frame = (_module_frames[i] + _module_frames_offset[i]);
        frame_unset = false;
      } else {
        if (*frame != (_module_frames[i] + _module_frames_offset[i])) {
          fprintf(stderr,"Error: data out-of-order got data for module %u got frame %lu, but was expecting frame %lu\n", i, _module_frames[i] + _module_frames_offset[i], *frame);
          drop_frame = true;
        }
      }
    }
  }

  return !drop_frame;
}

bool Detector::get_frame_poll(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data)
{
  int npoll = 0;
  bool drop_frame = false;
  bool frame_unset = true;
  bool frame_complete = false;

  for (unsigned i=0; i<_num_modules; i++) {
    _module_first_packet[i] = true;
    _module_last_packet[i] = false;
    _module_npackets[i] = 0;
    _module_data[i] = data;
    data += _modules[i]->get_num_pixels();
  }

  while(!frame_complete) {
    frame_complete = true;
    npoll = zmq_poll(_pfds, _num_modules+1, -1);
    if (npoll < 0) {
      fprintf(stderr,"Error: frame poller failed with error code: %s\n", strerror(errno));
      return false;
    }

    if(_pfds[_num_modules].revents & ZMQ_POLLIN) {
        int signal;
        ::read(_sigfd[0], &signal, sizeof(signal));
        if (signal == JF_FRAME_WAIT_EXIT) {
          printf("received frame wait exit signal - canceling get_frame\n");
          return false;
        } else {
          fprintf(stderr,"Warning: received unknown signal: %d\n", signal);
        }
    }

    for (unsigned i=0; i<_num_modules; i++) {
      if ((_pfds[i].revents & ZMQ_POLLIN) && (!_module_last_packet[i])) {
        if (_modules[i]->get_packet(&_module_frames[i], metadata?&metadata[i]:NULL, _module_data[i], &_module_first_packet[i], &_module_last_packet[i], &_module_npackets[i])) {
          if (frame_unset) {
            *frame = _module_frames[i] + _module_frames_offset[i];
            frame_unset = false;
          } else {
            if (*frame != (_module_frames[i] + _module_frames_offset[i])) {
              fprintf(stderr,"Error: data out-of-order got data for module %u got frame %lu, but was expecting frame %lu\n",
                      i, _module_frames[i] + _module_frames_offset[i], *frame);
              _module_last_packet[i] = true;
              drop_frame = true;
            }
          }

          if (_module_last_packet[i] && (_module_npackets[i] != JF_PACKET_NUM)) {
            fprintf(stderr,"Error: frame %lu from module %u is incomplete, received %u out of %d expected\n",
                    _module_frames[i] + _module_frames_offset[i], i, _module_npackets[i], JF_PACKET_NUM);
            drop_frame = true;
          }
        }
      }

      if (!_module_last_packet[i]) {
        frame_complete = false;
      }
    }
  }

  return !drop_frame;
}

bool Detector::get_module_config(JungfrauModConfigType* module_config, unsigned max_modules)
{
  if (_num_modules > max_modules) {
    fprintf(stderr,"Error: configuration array size (%u) is smaller than the number of modules (%u) in the detector!\n", max_modules, _num_modules);
    return false;
  }

  for (unsigned i=0; i<_num_modules; i++) {
    module_config[i] = JungfrauModConfigType(_modules[i]->serialnum(), _modules[i]->version(), _modules[i]->firmware());
  }

  return true;
}

bool Detector::start()
{
  bool success = true;
  for (unsigned i=0; i<_num_modules; i++) {
    if (!_modules[i]->start()) {
      fprintf(stderr,"Error: failure starting acquistion of module %u of the detector!\n", i);
      success = false;
    }
  }
  return success;
}

bool Detector::stop()
{
  bool success = true;
  for (unsigned i=0; i<_num_modules; i++) {
    if (!_modules[i]->stop()) {
      fprintf(stderr,"Error: failure stopping acquistion of module %u of the detector!\n", i);
      success = false;
    }
  }
  return success;
}

void Detector::flush()
{
  printf("Flushing any remaining data from module socket buffers\n");
  for (unsigned i=0; i<_num_modules; i++) {
    printf("flushing data socket of module %u\n", i);
    _modules[i]->flush();
  }
}

unsigned Detector::get_num_rows(unsigned module) const
{
  if (module < _num_modules) {
    return _modules[module]->get_num_rows();
  } else {
    fprintf(stderr,"Error: detector does not contain module %u - only %u modules in detector!\n", module, _num_modules);
    return 0;
  }
}

unsigned Detector::get_num_columns(unsigned module) const
{
  if (module < _num_modules) {
    return _modules[module]->get_num_columns();
  } else {
    fprintf(stderr,"Error: detector does not contain module %u - only %u modules in detector!\n", module, _num_modules);
    return 0;
  }
}

unsigned Detector::get_num_modules() const
{
  return _num_modules;
}

unsigned Detector::get_num_pixels() const
{
  unsigned total_num_pixels = 0;
  for (unsigned i=0; i<_num_modules; i++) {
    total_num_pixels += _modules[i]->get_num_pixels();
  }
  return total_num_pixels;
}

unsigned Detector::get_frame_size() const
{
  unsigned total_frame_size = 0;
  for (unsigned i=0; i<_num_modules; i++) {
    total_frame_size += _modules[i]->get_frame_size();
  }
  return total_frame_size;
}

const char** Detector::errors() 
{
  for (unsigned i=0; i<_num_modules; i++) {
    _msgbuf[i] = _modules[i]->error();
  }
  return _msgbuf;
}

void Detector::clear_errors()
{
  for (unsigned i=0; i<_num_modules; i++) {
    _modules[i]->clear_error();
  }
}

#undef CMD_LEN
#undef MSG_LEN
#undef ENDPOINT_LEN
#undef ADCPHASE_HALF
#undef ADCPHASE_QUARTER
#undef CLKDIV_HALF
#undef CLKDIV_QUARTER
#undef NUM_ROWS
#undef NUM_COLUMNS
#undef get_command_print
#undef put_command_print
#undef get_register_print
#undef put_register_print
#undef put_adcreg_print
#undef setbit_print
#undef clearbit_print
#undef error_print
