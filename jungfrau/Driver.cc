#include "Driver.hh"
#include "DataFormat.hh"
#include "DetectorId.hh"
#include "sls/Detector.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/uio.h>

#define MSG_LEN 256

// Temporary fixed sizes :(
#define NUM_ROWS 512
#define NUM_COLUMNS 1024

#define error_print(fmt, ...) \
  { snprintf(_msgbuf, MSG_LEN, fmt, ##__VA_ARGS__); fprintf(stderr, _msgbuf); }

using namespace Pds::Jungfrau;

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

static std::chrono::nanoseconds secs_to_ns(double secs)
{
  std::chrono::duration<double> dursecs{secs};
  return std::chrono::duration_cast<std::chrono::nanoseconds>(dursecs);
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

bool DacsConfig::operator==(const DacsConfig& rhs) const
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

bool DacsConfig::operator!=(const DacsConfig& rhs) const
{
  return !(*this == rhs);
}

uint16_t DacsConfig::vb_ds() const      { return _vb_ds; }
uint16_t DacsConfig::vb_comp() const    { return _vb_comp; }
uint16_t DacsConfig::vb_pixbuf() const  { return _vb_pixbuf; }
uint16_t DacsConfig::vref_ds() const    { return _vref_ds; }
uint16_t DacsConfig::vref_comp() const  { return _vref_comp; }
uint16_t DacsConfig::vref_prech() const { return _vref_prech; }
uint16_t DacsConfig::vin_com() const    { return _vin_com; }
uint16_t DacsConfig::vdd_prot() const   { return _vdd_prot; }

Module::Module(const int id, const std::string& control, const std::string& host, unsigned port,
               const std::string& mac, const std::string& det_ip, bool use_flow_ctrl, bool config_det_ip) :
  _id(id), _control(control), _host(host), _port(port), _mac(mac), _det_ip(det_ip), _use_flow_ctrl(use_flow_ctrl),
  _socket(-1), _connected(false), _boot(true), _freerun(false), _poweron(false),
  _sockbuf_sz(sizeof(jungfrau_dgram)*JF_PACKET_NUM*JF_EVENTS_TO_BUFFER), _readbuf_sz(sizeof(jungfrau_header)),
  _frame_sz(JF_DATA_ELEM * sizeof(uint16_t)), _frame_elem(JF_DATA_ELEM),
  _speed(JungfrauConfigType::Quarter)
{
  _readbuf = new char[_readbuf_sz];
  _msgbuf  = new char[MSG_LEN];

  try {
    // allocate shared memory detector class with specified id.
    _det = new sls::Detector(_id);

    // set the hostname of the detector
    std::vector<std::string> hostnames{_control};
    try {
      _det->setHostname(hostnames);
    } catch (const sls::RuntimeError &err) {
      // If the detector is running try stopping acquisition and retry
      Status detstat = status();
      if ((detstat == RUNNING || detstat == WAITING) && stop()) {
        _det->setHostname(hostnames);
      } else {
        // if detector wasn't running or stop fails re-raise to outer handler
        throw;
      }
    }

    // Check if detector control hostname setup worked
    sls::defs::detectorType type  = _det->getDetectorType().squash();

    bool detmac_status = configure_mac(config_det_ip);

    int nb = -1;
    hostent* entries = gethostbyname(_host.c_str());
    if (entries) {
      _socket = ::socket(AF_INET, SOCK_DGRAM, 0);
      ::setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, &_sockbuf_sz, sizeof(unsigned));

      unsigned addr = htonl(*(in_addr_t*)entries->h_addr_list[0]);

      sockaddr_in sa;
      sa.sin_family = AF_INET;
      sa.sin_addr.s_addr = htonl(addr);
      sa.sin_port        = htons(port);

      nb = ::bind(_socket, (sockaddr*)&sa, sizeof(sa));
    }

    if (nb<0) {
      error_print("Error: failed to bind to Jungfrau data receiver at %s on port %d: %s\n", host.c_str(), port, strerror(errno));
    } else if (_det->getHostname().squash().compare(_control) != 0) {
      error_print("Error: failed to connect to Jungfrau control interface at %s\n", _control.c_str());
    } else if (type != sls::defs::JUNGFRAU) {
      error_print("Error: detector at %s on port %d is not a Jungfrau: %s\n", host.c_str(), port, sls::ToString(type).c_str());
    } else {
      _connected=detmac_status;
    }
  }
  catch(const sls::RuntimeError &err) {
    error_print("Error: failed to initialize Jungfrau control interface of %s: %s\n", _control.c_str(), err.what());
    // cleanup detector after failure if it was created
    if (_det) {
      delete _det;
      _det = 0;
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
}

void Module::shutdown()
{
  if (_socket >= 0) {
    ::close(_socket);
    _socket = -1;
  }
  if (_det) {
    _connected = false;
    delete _det;
    _det = 0;
    sls::freeSharedMemory(_id);
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
    error_print("Error: failed to connect to Jungfrau at address %s\n", _control.c_str());
    return false;
  }

  // stop the detector if not!
  Status detstat = status();
  if (detstat == RUNNING || detstat == WAITING) {
    if (!stop()) {
      error_print("Error: can't configure when the detector is in a running state\n");
      return false;
    }
  }

  _boot = !_det->getPowerChip().squash();
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

  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return detmac_status;
  }

  try {
    bool flow_ctrl = _det->getTenGigaFlowControl().squash();
    sls::IpAddr rx_ip = _det->getDestinationUDPIP().squash();
    sls::MacAddr rx_mac = _det->getDestinationUDPMAC().squash();

    if ((rx_ip == 0) || (rx_mac == 0)) {
      printf("detector udp_rx interface appears to be unset\n");
      needs_config = true;
    }

    if (flow_ctrl != _use_flow_ctrl) {
      printf("detector udp_rx interface flow control needs to be set\n");
      needs_config = true;
    }

    if (config_det_ip || needs_config) {
        printf("setting detector udp_rx interface flow control to %s\n", _use_flow_ctrl ? "on" : "off");
        _det->setTenGigaFlowControl(_use_flow_ctrl);
        printf("setting up detector udp_rx interface\n");
        _det->setDestinationUDPPort(_port);
        _det->setDestinationUDPIP(sls::HostnameToIp(_host.c_str()));
        _det->setDestinationUDPMAC(sls::MacAddr(_mac.c_str()));
        _det->setSourceUDPIP(sls::HostnameToIp(_det_ip.c_str()));
        _det->setSourceUDPMAC(sls::MacAddr("00:aa:bb:cc:dd:ee"));
        _det->validateUDPConfiguration();
        // if we get here they the config was successful
        detmac_status = true;
        // print readback values
        printf("rx_udpport:  %u\n", _det->getDestinationUDPPort().squash());
        printf("rx_udpip:    %s\n", _det->getDestinationUDPIP().squash().str().c_str());
        printf("rx_udpmac:   %s\n", _det->getDestinationUDPMAC().squash().str().c_str());
        printf("detectorip:  %s\n", _det->getSourceUDPIP().squash().str().c_str());
        printf("detectormac: %s\n", _det->getSourceUDPMAC().squash().str().c_str());
        printf("detector udp_rx interface is up\n");
    } else {
      printf("detector udp_rx interface is being set externally\n");
      detmac_status = true;
    }
  }
  catch (const sls::RuntimeError &err) {
    error_print("Error: detector udp_rx interface did not come up: %s\n", err.what());
  }

  return detmac_status;
}

bool Module::configure_dacs(const DacsConfig& dac_config)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    if (_boot || dac_config != _dac_config) {
      // Setting Dacs 12bit on 2.5V  (i.e. 2.5v=4096)
      printf("Setting Dacs:\n");

      // setting vb_ds
      printf("setting vb_ds to %hu\n", dac_config.vb_ds());
      _det->setDAC(sls::defs::VB_DS, dac_config.vb_ds());

      // setting vb_comp
      printf("setting vb_comp to %hu\n", dac_config.vb_comp());
      _det->setDAC(sls::defs::VB_COMP, dac_config.vb_comp());

      // setting vb_pixbuf
      printf("setting vb_pixbuf to %hu\n", dac_config.vb_pixbuf());
      _det->setDAC(sls::defs::VB_PIXBUF, dac_config.vb_pixbuf());

      // setting vref_ds
      printf("setting vref_ds to %hu\n", dac_config.vref_ds());
      _det->setDAC(sls::defs::VREF_DS, dac_config.vref_ds());

      // setting vref_comp
      printf("setting vref_comp to %hu\n", dac_config.vref_comp());
      _det->setDAC(sls::defs::VREF_COMP, dac_config.vref_comp());

      // setting vref_prech
      printf("setting vref_prech to %hu\n", dac_config.vref_prech());
      _det->setDAC(sls::defs::VREF_PRECH, dac_config.vref_prech());

      // setting vin_com
      printf("setting vin_com to %hu\n", dac_config.vin_com());
      _det->setDAC(sls::defs::VIN_COM, dac_config.vin_com());

      // setting vdd_prot
      printf("setting vdd_prot to %hu\n", dac_config.vdd_prot());
      _det->setDAC(sls::defs::VDD_PROT, dac_config.vdd_prot());
    }
  }
  catch (const sls::RuntimeError &err) {
    error_print("Error: detector dacs configuration failed: %s\n", err.what());
    return false;
  }

  return verify_dacs(dac_config);
}

bool Module::verify_dacs(const DacsConfig& dac_config)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    // readback all the dac values
    uint16_t vb_ds      = (uint16_t) _det->getDAC(sls::defs::VB_DS).squash();
    uint16_t vb_comp    = (uint16_t) _det->getDAC(sls::defs::VB_COMP).squash();
    uint16_t vb_pixbuf  = (uint16_t) _det->getDAC(sls::defs::VB_PIXBUF).squash();
    uint16_t vref_ds    = (uint16_t) _det->getDAC(sls::defs::VREF_DS).squash();
    uint16_t vref_comp  = (uint16_t) _det->getDAC(sls::defs::VREF_COMP).squash();
    uint16_t vref_prech = (uint16_t) _det->getDAC(sls::defs::VREF_PRECH).squash();
    uint16_t vin_com    = (uint16_t) _det->getDAC(sls::defs::VIN_COM).squash();
    uint16_t vdd_prot   = (uint16_t) _det->getDAC(sls::defs::VDD_PROT).squash();
    // updated cached dac_config to reflect readback
    _dac_config = DacsConfig(vb_ds,
                             vb_comp,
                             vb_pixbuf,
                             vref_ds,
                             vref_comp,
                             vref_prech,
                             vin_com,
                             vdd_prot);

    // print and set error for any readbacks that don't match
    check_readback("vb_ds", vb_ds, dac_config.vb_ds());
    check_readback("vb_comp", vb_comp, dac_config.vb_comp());
    check_readback("vb_pixbuf", vb_pixbuf, dac_config.vb_pixbuf());
    check_readback("vref_ds", vref_ds, dac_config.vref_ds());
    check_readback("vref_comp", vref_comp, dac_config.vref_comp());
    check_readback("vref_prech", vref_prech, dac_config.vref_prech());
    check_readback("vin_com", vin_com, dac_config.vin_com());
    check_readback("vdd_prot", vdd_prot, dac_config.vdd_prot());

    return _dac_config == dac_config;
  }
  catch (const sls::RuntimeError &err) {
    error_print("Error: detector dacs readback failed: %s\n", err.what());
    return false;
  }
}

bool Module::configure_power(bool reset_adc)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    if (_boot) {
      // power on the chips
      printf("powering on the chip\n");
      _det->setPowerChip(true);

      // reset adc
      if (reset_adc) {
        printf("resetting the adc\n");
        _det->writeAdcRegister(0x08, 0x3);
        _det->writeAdcRegister(0x08, 0x0);
        _det->writeAdcRegister(0x14, 0x40);
        _det->writeAdcRegister(0x4, 0xf);
        _det->writeAdcRegister(0x5, 0x3f);
        _det->writeAdcRegister(0x18, 0x2);

        // the value here seems hardware version dependent
        //_det->writeRegister(0x43, 0x453b2a9c);
      }
    }

    // initial powerup complete
    _poweron = true;

    return _poweron;
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector power configuration failed: %s\n", err.what());
    return false;
  }
}

bool Module::configure_speed(JungfrauConfigType::SpeedMode speed, bool& sleep)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    if (_boot || speed != _speed) {
      // set the speed value
      switch (speed) {
        case JungfrauConfigType::Quarter:
          printf("setting detector to quarter speed\n");
          _det->setReadoutSpeed(sls::defs::QUARTER_SPEED);
          break;
        case JungfrauConfigType::Half:
          printf("setting detector to half speed\n");
          _det->setReadoutSpeed(sls::defs::HALF_SPEED);
          break;
        default:
          error_print("Error: invalid clock speed setting for the detector %d\n", speed);
          return false;
      }

      _speed = speed;
      sleep = true;

      return verify_speed(speed);
    } else {
      return true;
    }
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector speed configuration failed: %s\n", err.what());
    return false;
  }
}

bool Module::verify_speed(JungfrauConfigType::SpeedMode speed)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    switch (_det->getReadoutSpeed().squash()) {
      case sls::defs::QUARTER_SPEED:
        return speed == JungfrauConfigType::Quarter;
      case sls::defs::HALF_SPEED:
        return speed == JungfrauConfigType::Half;
      default:
        error_print("Error: invalid clock speed setting for the detector\n");
        return false;
    }
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector speed readback failed: %s\n", err.what());
    return false;
  }
}

bool Module::configure_acquistion(uint64_t nframes,
                                  double trig_delay,
                                  double exposure_time,
                                  double exposure_period,
                                  bool force_reset)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  // reset run control only if requested
  if (force_reset) {
    reset();
  }

  try {
    // check if freerunning or triggered
    _freerun = nframes > 0;
    sls::defs::timingMode trig_mode = _freerun ? sls::defs::AUTO_TIMING : sls::defs::TRIGGER_EXPOSURE;
    // for trigger mode one frame per trigger
    nframes = _freerun ? nframes : 1;
    uint64_t ncycles = _freerun ? 1 : 10000000000;
    // convert time in seconds to ns
    sls::ns trig_delay_ns = secs_to_ns(trig_delay);
    sls::ns exposure_time_ns = secs_to_ns(exposure_time);
    sls::ns exposure_period_ns = secs_to_ns(exposure_period);

    printf("setting trigger delay to %.6f\n", trig_delay);
    _det->setDelayAfterTrigger(trig_delay_ns);

    printf("configuring for %s\n", _freerun ? "free run" : "triggered mode");
    _det->setTimingMode(trig_mode);
    _det->setNumberOfTriggers(ncycles);
    _det->setNumberOfFrames(nframes);

    printf("setting exposure period to %.6f seconds\n", exposure_period);
    _det->setPeriod(exposure_period_ns);

    printf("setting exposure time to %.6f seconds\n", exposure_time);
    _det->setExptime(exposure_time_ns);

    return verify_acquistion(nframes, ncycles, trig_delay, exposure_time, exposure_period);
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector acquistion configuration failed: %s\n", err.what());
    return false;
  }
}

bool Module::verify_acquistion(uint64_t nframes, uint64_t ncycles, double trig_delay, double exposure_time, double exposure_period)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    uint64_t nframes_rbv = _det->getNumberOfFrames().squash();
    uint64_t ncycles_rbv = _det->getNumberOfTriggers().squash();
    sls::ns trig_delay_rbv = _det->getDelayAfterTrigger().squash();
    sls::ns exposure_time_rbv = _det->getExptime().squash();
    sls::ns exposure_period_rbv = _det->getPeriod().squash();
    sls::defs::timingMode trig_mode_rbv = _det->getTimingMode().squash();

    return (nframes_rbv == nframes) &&
           (ncycles_rbv == ncycles) &&
           (trig_mode_rbv == (_freerun ? sls::defs::AUTO_TIMING : sls::defs::TRIGGER_EXPOSURE)) &&
           (trig_delay_rbv == secs_to_ns(trig_delay)) &&
           (exposure_time_rbv == secs_to_ns(exposure_time)) &&
           (exposure_period_rbv == secs_to_ns(exposure_period));
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector acquistion readback failed: %s\n", err.what());
    return false;
  }
}

bool Module::configure_gain(uint32_t bias, JungfrauConfigType::GainMode gain)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    printf("setting bias voltage to %d volts\n", bias);
    _det->setHighVoltage(bias);

    printf("setting gain mode %d\n", gain);

  // Now set the ones of the gain we want
  switch(gain) {
    case JungfrauConfigType::Normal:
      _det->setSettings(sls::defs::GAIN0);
      _det->setGainMode(sls::defs::DYNAMIC);
      break;
    case JungfrauConfigType::FixedGain1:
      _det->setSettings(sls::defs::GAIN0);
      _det->setGainMode(sls::defs::FIX_G1);
      break;
    case JungfrauConfigType::FixedGain2:
      _det->setSettings(sls::defs::GAIN0);
      _det->setGainMode(sls::defs::FIX_G2);
      break;
    case JungfrauConfigType::ForcedGain1:
      _det->setSettings(sls::defs::GAIN0);
      _det->setGainMode(sls::defs::FORCE_SWITCH_G1);
      break;
    case JungfrauConfigType::ForcedGain2:
      _det->setSettings(sls::defs::GAIN0);
      _det->setGainMode(sls::defs::FORCE_SWITCH_G2);
      break;
    case JungfrauConfigType::HighGain0:
      _det->setSettings(sls::defs::HIGHGAIN0);
      _det->setGainMode(sls::defs::DYNAMIC);
      break;
    default:
      error_print("Error: invalid gain mode for the detector\n");
      return false;
  }

    return verify_gain(bias, gain);
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector gain readback failed: %s\n", err.what());
    return false;
  }
}

bool Module::verify_gain(uint32_t bias, JungfrauConfigType::GainMode gain)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    uint32_t bias_rbv = _det->getHighVoltage().squash();
    sls::defs::detectorSettings settings_rbv = _det->getSettings().squash();
    sls::defs::gainMode gain_rbv = _det->getGainMode().squash();

    sls::defs::detectorSettings settings_expected;
    sls::defs::gainMode gain_expected;

    switch(gain) {
      case JungfrauConfigType::Normal:
        settings_expected = sls::defs::GAIN0;
        gain_expected = sls::defs::DYNAMIC;
        break;
      case JungfrauConfigType::FixedGain1:
        settings_expected = sls::defs::GAIN0;
        gain_expected = sls::defs::FIX_G1;
        break;
      case JungfrauConfigType::FixedGain2:
        settings_expected = sls::defs::GAIN0;
        gain_expected = sls::defs::FIX_G2;
        break;
      case JungfrauConfigType::ForcedGain1:
        settings_expected = sls::defs::GAIN0;
        gain_expected = sls::defs::FORCE_SWITCH_G1;
        break;
      case JungfrauConfigType::ForcedGain2:
        settings_expected = sls::defs::GAIN0;
        gain_expected = sls::defs::FORCE_SWITCH_G2;
        break;
      case JungfrauConfigType::HighGain0:
        settings_expected = sls::defs::HIGHGAIN0;
        gain_expected = sls::defs::DYNAMIC;
        break;
      default:
        error_print("Error: invalid gain mode for the detector\n");
        return false;
    }

    return (bias_rbv == bias) &&
           (settings_rbv == settings_expected) &&
           (gain_rbv == gain_expected);
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector gain readback failed: %s\n", err.what());
    return false;
  } 
}

bool Module::check_readback(const char* name, uint32_t rbv, uint32_t expected)
{
  if (rbv != expected) {
    error_print("Error: detector readback of %s does not match: %u vs %u\n",
                name, rbv, expected);
    return false;
  } else {
    return true;
  }
}

bool Module::check_size(uint32_t num_rows, uint32_t num_columns) const
{
  return (num_rows == NUM_ROWS) && (num_columns == NUM_COLUMNS);
}

uint64_t Module::next_frame()
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return false;
  }

  try {
    return _det->getNextFrameNumber().squash();
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector next frame readback failed: %s\n", err.what());
    return 0;
  }
}

void Module::set_next_frame(uint64_t nframe)
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return ;
  }

  try {
    _det->setNextFrameNumber(nframe);
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector next frame counter set failed: %s\n", err.what());
  }
}

uint64_t Module::moduleid()
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return 0;
  }

  try {
    return _det->getModuleId().squash();
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector module id readback failed: %s\n", err.what());
    return 0;
  }
}

uint64_t Module::serialnum()
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return 0;
  }

  try {
    return _det->getSerialNumber().squash();
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector serial number readback failed: %s\n", err.what());
    return 0;
  }
}

uint64_t Module::software()
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return 0;
  }

  try {
    uint64_t version_id = 0;
    std::string version = _det->getDetectorServerVersion().squash();
    size_t start=0, end =0;
    int shift = 3;
    while (shift >= 0) {
      end = version.find(".", start);
      version_id |= (strtoull(version.substr(start, end).c_str(), NULL, 0) << (shift*16));
      shift--;
      if (end != std::string::npos) {
        start = end+1;
      } else {
        break;
      }
    }

    return version_id;
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector software version readback failed: %s\n", err.what());
    return 0;
  }
}

uint64_t Module::firmware()
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return 0;
  }

  try {
    return _det->getFirmwareVersion().squash();
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector firmware readback failed: %s\n", err.what());
    return 0;
  }
}

Module::Status Module::status()
{
  Status status = UNKNOWN;
  try {
    if (_det) {
      sls::defs::runStatus rstat = _det->getDetectorStatus().squash();
      switch(rstat) {
        case sls::defs::IDLE:
          status = IDLE;
          break;
        case sls::defs::ERROR:
          status = ERROR;
          break;
        case sls::defs::WAITING:
          status = WAITING;
          break;
        case sls::defs::RUN_FINISHED:
          status = RUN_FINISHED;
          break;
        case sls::defs::TRANSMITTING:
          status = TRANSMITTING;
          break;
        case sls::defs::RUNNING:
          status = RUNNING;
          break;
        case sls::defs::STOPPED:
          status = STOPPED;
          break;
      }
    } else {
      error_print("Error: detector control interface is not initialized!\n");
    }
  }
  catch(const sls::RuntimeError &err) {
    error_print("Error: failed to get detector status: %s\n", err.what());
  }

  return status;
}

std::string Module::status_str()
{
  switch(status()) {
    case IDLE:
      return sls::ToString(sls::defs::IDLE);
    case ERROR:
      return sls::ToString(sls::defs::ERROR);
    case WAITING:
      return sls::ToString(sls::defs::WAITING);
    case RUN_FINISHED:
      return sls::ToString(sls::defs::RUN_FINISHED);
    case TRANSMITTING:
      return sls::ToString(sls::defs::TRANSMITTING);
    case RUNNING:
      return sls::ToString(sls::defs::RUNNING);
    case STOPPED:
      return sls::ToString(sls::defs::STOPPED);
    case UNKNOWN:
    default:
      return "unknown";  
  }
}

bool Module::start()
{
  try {
    if (_det) {
      // start the acquisiton
      _det->startDetector();
      // check the detector status
      Status detstat = status();
      /*
       * IDLE status is okay when the detector is free running since the frame acquisiton may have finished
       * before the status call happens. Often this happens in the case of aquiring only a single frame.
       *
       * Triggered mode always runs until explicitly stopped, so being at IDLE after starting aquisiton is
       * an error state.
       */
      return detstat == RUNNING || detstat == WAITING || ((detstat == IDLE) && (_freerun));
    } else {
      error_print("Error: detector control interface is not initialized!\n");
      return false;
    }
  }
  catch(const sls::RuntimeError &err) {
    error_print("Error: failed to start detector acquisition: %s\n", err.what());
    return false;
  }
}

bool Module::stop()
{
  try {
    if (_det) {
      // stop the acquisition
      _det->stopDetector();
      // check the detector status
      Status detstat = status();
      return detstat != RUNNING && detstat != WAITING;
    } else {
      error_print("Error: detector control interface is not initialized!\n");
      return false;
    }
  }
  catch(const sls::RuntimeError &err) {
    error_print("Error: failed to stop detector acquisition: %s\n", err.what());
    return false;
  }
}

void Module::reset()
{
  if (!_det) {
    error_print("Error: detector control interface is not initialized!\n");
    return ;
  }

  try {
    printf("reseting run control ...");
    // reset mem machine fifos fifos
    _det->writeRegister(0x4f, 0x4000);
    _det->writeRegister(0x4f, 0x0);
    // reset run control
    _det->writeRegister(0x4f, 0x0400);
    _det->writeRegister(0x4f, 0x0);
    printf(" done\n");
  } catch (const sls::RuntimeError &err) {
    error_print("Error: detector reset failed: %s\n", err.what());
  }
}

unsigned Module::flush()
{
  ssize_t nb;
  struct sockaddr_in clientaddr;
  unsigned count = 0;
  socklen_t clientaddrlen = sizeof(clientaddr);
  do {
    nb = ::recvfrom(_socket, _readbuf, sizeof(jungfrau_header), MSG_DONTWAIT, (struct sockaddr *)&clientaddr, &clientaddrlen);
    if (nb > 0) {
      count += 1;
    }
  } while (nb > 0);
  return count;
}

bool Module::get_frame(uint64_t* frame, uint16_t* data)
{
  return get_frame(frame, NULL, data);
}

bool Module::get_packet(uint64_t* frame, JungfrauModInfoType* metadata, uint16_t* data, bool* first_packet, bool* last_packet, unsigned* npackets)
{
  int nb;
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen = sizeof(clientaddr);
  jungfrau_header* header = NULL;
  struct iovec dgram_vec[2];

  nb = ::recvfrom(_socket, _readbuf, sizeof(jungfrau_header), MSG_PEEK, (struct sockaddr *)&clientaddr, &clientaddrlen);
  if (nb<0) {
    fprintf(stderr,"Error: failure receiving packet from Jungfru at %s on port %d: %s\n", _host.c_str(), _port, strerror(errno));
    return false;
  } else {
    header = (jungfrau_header*) _readbuf;

    if (*first_packet) {
      *frame = header->framenum;
      *first_packet = false;
    } else if(*frame != header->framenum) {
      *last_packet = true;
      fprintf(stderr,"Error: data out-of-order got data for frame %lu, but was expecting frame %lu from Jungfru at %s\n", header->framenum, *frame, _host.c_str());
      return false;
    }

    dgram_vec[0].iov_base = _readbuf;
    dgram_vec[0].iov_len = sizeof(jungfrau_header);
    dgram_vec[1].iov_base = &data[_frame_elem*header->packetnum];
    dgram_vec[1].iov_len = sizeof(jungfrau_dgram) - sizeof(jungfrau_header);

    nb = ::readv(_socket, dgram_vec, 2);
    if (nb<0) {
      fprintf(stderr,"Error: failure receiving packet from Jungfru at %s on port %d: %s\n", _host.c_str(), _port, strerror(errno));
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
    fprintf(stderr,"Error: frame %lu from Jungfrau at %s is incomplete, received %u out of %d expected\n", cur_frame, _host.c_str(), npackets, JF_PACKET_NUM);
  }
 
  return (npackets == JF_PACKET_NUM);
}

const char* Module::get_hostname() const
{
  return _control.c_str();
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

int Module::fd() const
{
  return _socket;
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

Detector::Detector(std::vector<Module*>& modules, bool use_threads, int thread_rtprio) :
  _aborted(false),
  _use_threads(use_threads),
  _threads(0),
  _thread_attr(0),
  _pfds(0),
  _num_modules(modules.size()),
  _module_frames(new uint64_t[modules.size()]),
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
  // explicitly zero the _module_frames buffer
  memset(_module_frames, 0, modules.size() * sizeof(uint64_t));
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
    _pfds = new pollfd[_num_modules+1];
    _pfds[_num_modules].fd       = _sigfd[0];
    _pfds[_num_modules].events   = POLLIN;
    _pfds[_num_modules].revents  = 0;
    for (unsigned i=0; i<_num_modules; i++) {
      _pfds[i].fd       = _modules[i]->fd();
      _pfds[i].events   = POLLIN;
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

uint64_t Detector::sync_next_frame()
{
  uint64_t frame_num[_num_modules];
  uint64_t next_frame = 1;
  for (unsigned i=0; i<_num_modules; i++) {
    frame_num[i] = _modules[i]->next_frame();
    if (frame_num[i] > next_frame) {
      next_frame = frame_num[i];
    }
  }

  for (unsigned i=0; i<_num_modules; i++) {
    if (frame_num[i] < next_frame) {
        printf("Warning: module %u is behind by %lu frame%s! - adjusting frame offset\n", i, next_frame - frame_num[i], (next_frame - frame_num[i])==1 ? "" : "s");
        _modules[i]->set_next_frame(next_frame);
    }
  }

  return next_frame;
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
      printf("configuring power of module %u\n", i);
      if(!_modules[i]->configure_power()) {
        fprintf(stderr, "Error: module %u power configuration failed!\n", i);
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
        *frame = _module_frames[i];
        frame_unset = false;
      } else {
        if (*frame != _module_frames[i]) {
          fprintf(stderr,"Error: data out-of-order got data for module %u got frame %lu, but was expecting frame %lu\n",
                  i, _module_frames[i], *frame);
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
    npoll = ::poll(_pfds, (nfds_t) _num_modules+1, -1);
    if (npoll < 0) {
      fprintf(stderr,"Error: frame poller failed with error code: %s\n", strerror(errno));
      return false;
    }

    if(_pfds[_num_modules].revents & POLLIN) {
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
      if ((_pfds[i].revents & POLLIN) && (!_module_last_packet[i])) {
        if (_modules[i]->get_packet(&_module_frames[i], metadata?&metadata[i]:NULL, _module_data[i], &_module_first_packet[i], &_module_last_packet[i], &_module_npackets[i])) {
          if (frame_unset) {
            *frame = _module_frames[i];
            frame_unset = false;
          } else {
            if (*frame != _module_frames[i]) {
              fprintf(stderr,"Error: data out-of-order got data for module %u got frame %lu, but was expecting frame %lu\n",
                      i, _module_frames[i], *frame);
              _module_last_packet[i] = true;
              drop_frame = true;
            }
          }

          if (_module_last_packet[i] && (_module_npackets[i] != JF_PACKET_NUM)) {
            fprintf(stderr,"Error: frame %lu from module %u is incomplete, received %u out of %d expected\n",
                    _module_frames[i], i, _module_npackets[i], JF_PACKET_NUM);
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
    module_config[i] = JungfrauModConfigType(DetId(_modules[i]->serialnum(), _modules[i]->moduleid()).full(),
                                             _modules[i]->software(),
                                             _modules[i]->firmware());
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
  unsigned counts[_num_modules];
  for (unsigned i=0; i<_num_modules; i++) counts[i] = 0;

  printf("Flushing any remaining data from module socket buffers\n");
  if (_use_threads) {
    for (unsigned i=0; i<_num_modules; i++) {
      printf("flushing data socket of module %u\n", i);
      counts[i] += _modules[i]->flush();
    }
  } else {
    int npoll = 0;
    do {
    npoll = ::poll(_pfds, (nfds_t) _num_modules+1, 500);
      if (npoll < 0) {
        fprintf(stderr,"Error: frame poller failed with error code: %s\n", strerror(errno));
      } else {
        for (unsigned i=0; i<_num_modules; i++) {
          if (_pfds[i].revents & POLLIN)
            counts[i] += _modules[i]->flush();
        }
      }
    } while (npoll>0);
  }

  for (unsigned i=0; i<_num_modules; i++) {
    printf("flushed %u packets from module %u\n", counts[i], i);
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

const char* Detector::get_hostname(unsigned module) const
{
  if (module < _num_modules) {
    return _modules[module]->get_hostname();
  } else {
    fprintf(stderr,"Error: detector does not contain module %u - only %u modules in detector!\n", module, _num_modules);
    return NULL;
  }
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

#undef MSG_LEN
#undef NUM_ROWS
#undef NUM_COLUMNS
#undef error_print
