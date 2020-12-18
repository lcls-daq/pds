#include "Driver.hh"
#include "Features.hh"
#include "Errors.hh"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <wchar.h>

#define LENGTH_FIELD_SIZE 4
#define CID_FIELD_SIZE 4

#define set_config_bool(feature, value)                                     \
  if (!at_set_bool(feature, value)) {                                       \
    fprintf(stderr, "Failure setting %ls feature of the camera\n", feature);\
    return false; }

#define set_config_int(feature, value)                            \
  if (!at_set_int(feature, value)) {                              \
    fprintf(stderr,                                               \
            "Failure setting %ls feature of the camera to %lld\n",\
            feature, value);                                      \
    return false; }

#define set_config_float(feature, value)                        \
  if (!at_set_float(feature, value)) {                          \
    fprintf(stderr,                                             \
            "Failure setting %ls feature of the camera to %G\n",\
            feature, value);                                    \
    return false; }

#define set_enum_case(feature, enum_case, value)                            \
  case enum_case:                                                           \
    if (!at_set_enum(feature, value)) {                                     \
      fprintf(stderr, "Failure setting %ls feature of the camera to %ls\n", \
      feature, value);                                                      \
      return false;                                                         \
    }                                                                       \
    break;

namespace Pds {
  namespace Zyla {
    static const AT_WC* AT3_NOT_STABILISED = L"Not Stabilised";
    static const AT_WC* AT3_STABILISED = L"Stabilised";
    static const AT_WC* AT3_COOLER_OFF = L"Cooler Off";
    static const AT_WC* AT3_PIXEL_MONO_16 = L"Mono16";
    static const AT_WC* AT3_BINNING_1X1 = L"1x1";
    static const AT_WC* AT3_BINNING_2X2 = L"2x2";
    static const AT_WC* AT3_BINNING_3X3 = L"3x3";
    static const AT_WC* AT3_BINNING_4X4 = L"4x4";
    static const AT_WC* AT3_BINNING_8X8 = L"8x8";
  }
}

using namespace Pds::Zyla;

Driver::Driver(AT_H cam, unsigned nbuffers) :
  _cam(cam),
  _nbuffers(nbuffers),
  _open(cam!=AT_HANDLE_UNINITIALISED),
  _queued(false),
  _buffer_size(0),
  _data_buffer(0),
  _stride(0),
  _width(0),
  _height(0)
{
}

Driver::~Driver()
{
  if (_open) AT_Close(_cam);
  if (_data_buffer) delete[] _data_buffer;
}

bool Driver::set_image(AT_64 width, AT_64 height, AT_64 orgX, AT_64 orgY, AT_64 binX, AT_64 binY, bool noise_filter, bool blemish_correction, bool fast_frame)
{
  // Setup the image size
  if (at_check_implemented(AT3_AOI_H_BIN) && at_check_implemented(AT3_AOI_V_BIN)) {
    if (at_get_int(AT3_AOI_H_BIN) != 1) { // clear past binning settings
      set_config_int(AT3_AOI_H_BIN, 1LL);
    }
    if (at_get_int(AT3_AOI_V_BIN) != 1) { // clear past binning settings
      set_config_int(AT3_AOI_V_BIN, 1LL);
    }
  } else {
    if (!at_set_enum(AT3_AOI_BINNING, AT3_BINNING_1X1)) {
      fprintf(stderr, "Unable to set the binning mode of the camera to %ls!\n", AT3_BINNING_1X1);
      return false;
    }
  }
  set_config_int(AT3_AOI_WIDTH, width);
  set_config_int(AT3_AOI_LEFT, orgX);
  set_config_int(AT3_AOI_HEIGHT, height);
  set_config_int(AT3_AOI_TOP, orgY);
  if (at_check_implemented(AT3_AOI_H_BIN) && at_check_implemented(AT3_AOI_V_BIN)) {
    set_config_int(AT3_AOI_H_BIN, binX);
    set_config_int(AT3_AOI_V_BIN, binY);
  } else if (binX == binY) {
    switch(binX) {
      set_enum_case(AT3_AOI_BINNING, 1, AT3_BINNING_1X1);
      set_enum_case(AT3_AOI_BINNING, 2, AT3_BINNING_2X2);
      set_enum_case(AT3_AOI_BINNING, 3, AT3_BINNING_3X3);
      set_enum_case(AT3_AOI_BINNING, 4, AT3_BINNING_4X4);
      set_enum_case(AT3_AOI_BINNING, 8, AT3_BINNING_8X8);
    default:
      fprintf(stderr, "Unsupported binnig setting: %lldx%lld\n", binX, binY);
        return false;
    }
  } else {
    fprintf(stderr, "Binning of %lldx%lld is not allowed, the camera only supports symmetric binning!\n", binX, binY);
    return false;
  }
  // Enable/Disable the on FPGA image corrections
  set_config_bool(AT3_NOISE_FILTER, noise_filter);
  set_config_bool(AT3_BLEMISH_CORRECTION, blemish_correction);
  set_config_bool(AT3_AOI_FAST_FRAME, fast_frame);
  // If we got here this is done! :)
  return true;
}

bool Driver::set_cooling(bool enable, CoolingSetpoint setpoint, FanSpeed fan_speed)
{
  // Enable cooling
  set_config_bool(AT3_SENSOR_COOLING, enable);
  // Set Cooling setpoint (only if cooling is enabled)
  if (at_check_write(AT3_TEMPERATURE_CONTROL) && enable) {
    switch(setpoint) {
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_0C,     L"0.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg5C,  L"-5.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg10C, L"-10.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg15C, L"-15.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg20C, L"-20.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg25C, L"-25.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg30C, L"-30.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg35C, L"-35.00");
      set_enum_case(AT3_TEMPERATURE_CONTROL, Temp_Neg40C, L"-40.00");
      default:
        fprintf(stderr, "Unknown cooling setpoint value: %d\n", setpoint);
        return false;
    }
  }
  // Set Fan speed
  switch(fan_speed) {
    set_enum_case(AT3_FAN_SPEED, Off, L"Off");
    set_enum_case(AT3_FAN_SPEED, Low, L"Low");
    set_enum_case(AT3_FAN_SPEED, On,  L"On");
    default:
      fprintf(stderr, "Unknown fan_speed value: %d\n", fan_speed);
      return false;
  }
  return true;
}

bool Driver::set_trigger(TriggerMode trigger, double trigger_delay, bool overlap)
{
  // Set the trigger mode
  switch(trigger) {
    set_enum_case(AT3_TRIGGER_MODE, Internal,                 L"Internal");
    set_enum_case(AT3_TRIGGER_MODE, ExternalLevelTransition,  L"External Level Transition");
    set_enum_case(AT3_TRIGGER_MODE, ExternalStart,            L"External Start");
    set_enum_case(AT3_TRIGGER_MODE, ExternalExposure,         L"External Exposure");
    set_enum_case(AT3_TRIGGER_MODE, Software,                 L"Software");
    set_enum_case(AT3_TRIGGER_MODE, Advanced,                 L"Advanced");
    set_enum_case(AT3_TRIGGER_MODE, External,                 L"External");
    default:
      fprintf(stderr, "Unknown tigger setting value: %d\n", trigger);
      return false;
  }
  // Set readout overlap mode (if enabled next acquistion can start while camera is reading out last frame)
  if (at_check_write(AT3_OVERLAP)) {
    set_config_bool(AT3_OVERLAP, overlap);
  }
  // Set the internal trigger delay
  if (at_check_write(AT3_EXTERN_TRIGGER_DELAY)) {
    set_config_float(AT3_EXTERN_TRIGGER_DELAY, trigger_delay);
  }

  return true;
}

bool Driver::set_exposure(double exposure_time)
{
  // Set the camera exposure time
  if (at_check_write(AT3_EXPOSURE_TIME)) {
    set_config_float(AT3_EXPOSURE_TIME, exposure_time);
  }

  return true;
}

bool Driver::set_readout(ShutteringMode shutter, ReadoutRate readout_rate, GainMode gain)
{
  if (!at_set_enum(AT3_PIXEL_ENCODING, AT3_PIXEL_MONO_16)) {
    fprintf(stderr, "Unable to set the pixel encoding of the camera to %ls!\n", AT3_PIXEL_MONO_16);
    return false;
  }
  // Set the camera electronic shuttering mode
  switch(shutter) {
    set_enum_case(AT3_SHUTTERING_MODE, Rolling, L"Rolling");
    set_enum_case(AT3_SHUTTERING_MODE, Global,  L"Global");
    default:
      fprintf(stderr, "Unknown shutter mode value: %d\n", shutter);
      return false;
  }
  // Set the pixel readout rate
  switch(readout_rate) {
    set_enum_case(AT3_PIXEL_READOUT_RATE, Rate280MHz, L"280 MHz");
    set_enum_case(AT3_PIXEL_READOUT_RATE, Rate200MHz, L"200 MHz");
    set_enum_case(AT3_PIXEL_READOUT_RATE, Rate100MHz, L"100 MHz");
    set_enum_case(AT3_PIXEL_READOUT_RATE, Rate10MHz,  L"10 MHz");
    default:
      fprintf(stderr, "Unknown readout rate value: %d\n", readout_rate);
      return false;
  }
  // Set the gain mode
  switch(gain) {
    set_enum_case(AT3_PREAMP_GAIN_MODE, HighWellCap12Bit,         L"12-bit (high well capacity)");
    set_enum_case(AT3_PREAMP_GAIN_MODE, LowNoise12Bit,            L"12-bit (low noise)");
    set_enum_case(AT3_PREAMP_GAIN_MODE, LowNoiseHighWellCap16Bit, L"16-bit (low noise & high well capacity)");
    default:
      fprintf(stderr, "Unknown gain mode value: %d\n", gain);
      return false;
  }

  return true;
}

bool Driver::set_intensifier(GateMode gate, InsertionDelay delay, AT_64 mcp_gain, bool mcp_intelligate)
{
  if(at_check_implemented(AT3_GATE_MODE)) {
    // Set the gate mode
    switch(gate) {
      set_enum_case(AT3_GATE_MODE, CWOn,        L"CWOn");
      set_enum_case(AT3_GATE_MODE, CWOff,       L"CWOff");
      set_enum_case(AT3_GATE_MODE, FireOnly,    L"FireOnly");
      set_enum_case(AT3_GATE_MODE, GateOnly,    L"GateOnly");
      set_enum_case(AT3_GATE_MODE, FireAndGate, L"FireAndGate");
      set_enum_case(AT3_GATE_MODE, DDG,         L"DDG");
      default:
        fprintf(stderr, "Unknown gate mode value: %d\n", gate);
        return false;
    }
  } else {
    fprintf(stderr, "Feature %ls is not implemented for this camera!\n", AT3_GATE_MODE);
    return false;
  }

  if(at_check_implemented(AT3_INSERTION_DELAY)) {
    switch(delay){
      set_enum_case(AT3_INSERTION_DELAY, Normal, L"Normal");
      set_enum_case(AT3_INSERTION_DELAY, Fast,   L"Fast");
      default:
        fprintf(stderr, "Unknown delay value: %d\n", gate);
        return false;
    }
  } else {
    fprintf(stderr, "Feature %ls is not implemented for this camera!\n", AT3_INSERTION_DELAY);
    return false;
  }

  if(at_check_implemented(AT3_MCP_INTELLIGATE)) {
    if (delay == Normal) {
      set_config_bool(AT3_MCP_INTELLIGATE, mcp_intelligate);
    }
  } else {
    fprintf(stderr, "Feature %ls is not implemented for this camera!\n", AT3_MCP_INTELLIGATE);
    return false;
  }

  if(at_check_implemented(AT3_MCP_GAIN)) {
    set_config_int(AT3_MCP_GAIN, mcp_gain);
  } else {
    fprintf(stderr, "Feature %ls is not implemented for this camera!\n", AT3_MCP_GAIN);
    return false;
  }

  return true;
}

bool Driver::configure(const AT_64 nframes)
{
  AT_64 img_size_bytes;
  int old_buffer_size = _buffer_size;

  // flush any queued buffers
  if (_queued) flush();

  // set metadata settings
  set_config_bool(AT3_METADATA_ENABLE, true);
  set_config_bool(AT3_METADATA_TIMESTAMP, true);

  if (nframes > 0) {
    // set camera to acquire requested number of frames
    set_config_int(AT3_FRAME_COUNT, nframes);
    if (!at_set_enum(AT3_CYCLE_MODE, L"Fixed")) {
      fprintf(stderr, "Unable to set %ls to %ls", AT3_CYCLE_MODE, L"Fixed");
      return false;
    }
  } else {
    if (!at_set_enum(AT3_CYCLE_MODE, L"Continuous")) {
      fprintf(stderr, "Unable to set %ls to %ls", AT3_CYCLE_MODE, L"Continuous");
      return false;
    }
  }

  // Figure out if the camera is going to use weird padding at the end of rows. Thanks Andor....
  if (AT_GetInt(_cam, AT3_AOI_STRIDE, &_stride) != AT_SUCCESS) {
    fprintf(stderr, "Failure reading back %ls from the camera!\n", AT3_AOI_STRIDE);
  }
  // Figure out the height and width of the camera image we will get
  if (AT_GetInt(_cam, AT3_AOI_WIDTH, &_width) != AT_SUCCESS) {
    fprintf(stderr, "Failure reading back %ls from the camera!\n", AT3_AOI_WIDTH);
  }
  if (AT_GetInt(_cam, AT3_AOI_HEIGHT, &_height) != AT_SUCCESS) {
    fprintf(stderr, "Failure reading back %ls from the camera!\n", AT3_AOI_HEIGHT);
  }
  // Figure out the size of the total data the camera will send back - frame + metadata
  if (AT_GetInt(_cam, AT3_IMAGE_SIZE_BYTES, &img_size_bytes) == AT_SUCCESS) {
    _buffer_size = static_cast<int>(img_size_bytes);
    if(_data_buffer) {
      if (_buffer_size > old_buffer_size) {
        delete[] _data_buffer;
        _data_buffer = new unsigned char[_buffer_size*_nbuffers];
      }
    } else {
      _data_buffer = new unsigned char[_buffer_size*_nbuffers];
    }

    int retcode;
    for (unsigned i=0; i<_nbuffers; i++) {
      retcode = AT_QueueBuffer(_cam, &_data_buffer[i*_buffer_size], _buffer_size);
      if(retcode != AT_SUCCESS) {
        fprintf(stderr, "Failed adding image buffer to queue: %s\n", ErrorCodes::name(retcode));
        flush();
        return false;
      }
    }
    _queued = true;

    return true;
  } else {
    fprintf(stderr, "Failed to retrieve image buffer size from camera!\n");
    return false;
  }
}

bool Driver::start()
{
  return (AT_Command(_cam, AT3_ACQUISITION_START) == AT_SUCCESS);
}

bool Driver::stop(bool flush_buffers)
{
  bool acq_stop = true;
  if (AT_Command(_cam, AT3_ACQUISITION_STOP) != AT_SUCCESS) {
    fprintf(stderr, "Stop acquistion command failed!\n");
    acq_stop = false;
  }
  if (flush_buffers) {
    return flush() && acq_stop;
  } else {
    return acq_stop;
  }
}

bool Driver::flush()
{
  if (AT_Flush(_cam) == AT_SUCCESS) _queued = false;
  return !_queued;
}

bool Driver::close()
{
  if (_open) {
    if (_queued) flush();
    _open = false;
    int retcode = AT_Close(_cam);
    _cam = AT_HANDLE_UNINITIALISED;
    return (retcode == AT_SUCCESS);
  } else {
    return true;
  }
}

bool Driver::is_present() const
{
  AT_BOOL camera_pres;
  AT_GetBool(_cam, AT3_CAMERA_PRESENT, &camera_pres);
  return (camera_pres == AT_TRUE);
}

size_t Driver::frame_size() const
{
  return (size_t) _width * _height;
}

bool Driver::get_frame(AT_64& timestamp, uint16_t* data)
{
  int retcode;
  AT_64 width;
  AT_64 height;
  AT_64 stride;
  unsigned char* buffer;

  retcode = AT_WaitBuffer(_cam, &buffer, &_buffer_size, AT_INFINITE);
  if (retcode == AT_SUCCESS) {
    bool success = true;

    if (AT_GetTimeStampFromMetadata(buffer, _buffer_size, timestamp) != AT_SUCCESS) {
      fprintf(stderr, "Failure retrieving timestamp from frame metadata\n");
      success = false;
    }
    if (at_check_implemented(AT3_METADATA_FRAME_INFO)) {
      if (AT_GetWidthFromMetadata(buffer, _buffer_size, width) != AT_SUCCESS) {
        fprintf(stderr, "Failure retrieving timestamp from frame metadata\n");
        success = false;
      }
      if (AT_GetHeightFromMetadata(buffer, _buffer_size, height) != AT_SUCCESS) {
        fprintf(stderr, "Failure retrieving timestamp from frame metadata\n");
        success = false;
      }
      if (AT_GetStrideFromMetadata(buffer, _buffer_size, stride) != AT_SUCCESS) {
        fprintf(stderr, "Failure retrieving timestamp from frame metadata\n");
        success = false;
      }
      // Check if the metadata matches with the expected frame size
      if ((width != _width) || (height != _height) || (stride != _stride)) {
        fprintf(stderr,
                "Unexpected frame size returned by camera: width (%lld vs %lld), height (%lld vs %lld), stride (%lld vs %lld)\n",
                width,
                _width,
                height,
                _height,
                stride,
                _stride);
        success = false;
      }
    
  
      // If the metadata looks good convert the buffer to a usable image an return it
      if (success) {
        retcode = AT_ConvertBufferUsingMetadata(buffer, reinterpret_cast<unsigned char*>(data), _buffer_size, AT3_PIXEL_MONO_16);
        if (retcode != AT_SUCCESS) {
          fprintf(stderr, "Failure converting data buffer to image using metadata: %s\n", ErrorCodes::name(retcode));
          success = false;
        }
      }

    } else {
      // No frame metadata for camlink cameras
      if (success) {
        retcode = AT_ConvertBuffer(buffer, reinterpret_cast<unsigned char*>(data), _width, _height, _stride, AT3_PIXEL_MONO_16, AT3_PIXEL_MONO_16);
        if (retcode != AT_SUCCESS) {
          fprintf(stderr, "Failure converting data buffer to image: %s\n", ErrorCodes::name(retcode));
          success = false;
        }
      }
    }

    // Reuse the buffer for the next frame
    AT_QueueBuffer(_cam, buffer, _buffer_size);

    return success;
  } else {
    // Acquistion failed flush the buffer before re-adding it to the queue
    if (retcode != AT_ERR_NODATA) {
      fprintf(stderr, "Failure waiting for buffer callback from camera: %s\n", ErrorCodes::name(retcode));
    }
    return false;
  }
}

AT_64 Driver::sensor_width() const
{
  return at_get_int(AT3_SENSOR_WIDTH);
}

AT_64 Driver::sensor_height() const
{
  return at_get_int(AT3_SENSOR_HEIGHT);
}

AT_64 Driver::baseline() const {
  return at_get_int(AT3_BASELINE);
}

AT_64 Driver::clock() const {
  return at_get_int(AT3_TIMESTAMP_CLOCK);
}

AT_64 Driver::clock_rate() const {
  return at_get_int(AT3_TIMESTAMP_FREQUENCY);
}

AT_64 Driver::image_stride() const {
  return at_get_int(AT3_AOI_STRIDE);
}

AT_64 Driver::image_width() const {
  return at_get_int(AT3_AOI_WIDTH);
}

AT_64 Driver::image_height() const {
  return at_get_int(AT3_AOI_HEIGHT);
}

AT_64 Driver::image_orgX() const {
  return at_get_int(AT3_AOI_LEFT);
}

AT_64 Driver::image_orgY() const {
  return at_get_int(AT3_AOI_TOP);
}

AT_64 Driver::image_binX() const {
  if(at_check_implemented(AT3_AOI_H_BIN)) {
    return at_get_int(AT3_AOI_H_BIN);
  } else {
    return image_binning();
  }
}

AT_64 Driver::image_binY() const {
  if(at_check_implemented(AT3_AOI_V_BIN)) {
    return at_get_int(AT3_AOI_V_BIN);
  } else {
    return image_binning();
  }
}

AT_64 Driver::image_binning() const {
  AT_64 binning = -1;
  AT_WC bin_buffer[128];
  if (get_binning_mode(bin_buffer, 128)) {
    if (wcscmp(AT3_BINNING_1X1, bin_buffer) == 0) {
      binning = 1;
    } else if(wcscmp(AT3_BINNING_2X2, bin_buffer) == 0) {
      binning = 2;
    } else if(wcscmp(AT3_BINNING_3X3, bin_buffer) == 0) {
      binning = 3;
    } else if(wcscmp(AT3_BINNING_4X4, bin_buffer) == 0) {
      binning = 4;
    } else if(wcscmp(AT3_BINNING_8X8, bin_buffer) == 0) {
      binning = 8;
    } else {
      fprintf(stderr, "Camera returned an unknown binning mode %ls!\n",
              bin_buffer);
    }
  }

  return binning;
}

double Driver::readout_time() const
{
  return at_get_float(AT3_READOUT_TIME);
}

double Driver::frame_rate() const
{
  return at_get_float(AT3_FRAME_RATE);
}
      
double Driver::pixel_height() const
{
  return at_get_float(AT3_PIXEL_HEIGHT);
}

double Driver::pixel_width() const
{
  return at_get_float(AT3_PIXEL_WIDTH);
}

double Driver::temperature() const
{
  return at_get_float(AT3_SENSOR_TEMPERATURE);
}

double Driver::exposure() const
{
  return at_get_float(AT3_EXPOSURE_TIME);
}

double Driver::exposure_min() const
{
  return at_get_float_min(AT3_EXPOSURE_TIME);
}

double Driver::exposure_max() const
{
  return at_get_float_max(AT3_EXPOSURE_TIME);
}

bool Driver::overlap_mode() const
{
  AT_BOOL overlap;
  AT_GetBool(_cam, AT3_OVERLAP, &overlap);
  return (overlap == AT_TRUE);
}

bool Driver::cooling_on() const
{
  AT_BOOL cooling_on;
  AT_GetBool(_cam, AT3_SENSOR_COOLING, &cooling_on);
  return (cooling_on == AT_TRUE);
}

bool Driver::check_cooling(bool is_stable) const
{
  int retcode;
  int temp_idx;
  AT_WC temp_buf[128];
  
  retcode = AT_GetEnumIndex(_cam, AT3_TEMPERATURE_STATUS, &temp_idx);
  if (retcode != AT_SUCCESS) {
    fprintf(stderr, "Failed to retrieve %ls from camera!\n", AT3_TEMPERATURE_STATUS);
    return false;
  }

  retcode = AT_GetEnumStringByIndex(_cam, AT3_TEMPERATURE_STATUS, temp_idx, temp_buf, 128);
  if (retcode != AT_SUCCESS) {
    fprintf(stderr, "Failed to retrieve %ls from camera!\n", AT3_TEMPERATURE_STATUS);
    return false;
  }

  if (wcscmp(AT3_STABILISED, temp_buf) == 0) {
    return true;
  } else if(!is_stable && (wcscmp(AT3_NOT_STABILISED, temp_buf) == 0)) {
    return true;
  } else {
    return false;
  }
}

bool Driver::wait_cooling(unsigned timeout, bool is_stable) const
{
  int retcode;
  bool ready = false;
  int temp_idx;
  double temp;
  int count = 0;
  time_t start = time(0);
  AT_WC temp_buf[128];

  while((time(0) - start < timeout) || (timeout == 0)) {
    retcode = AT_GetEnumIndex(_cam, AT3_TEMPERATURE_STATUS, &temp_idx);
    if (retcode != AT_SUCCESS) {
      fprintf(stderr, "Failed to retrieve %ls from camera!\n", AT3_TEMPERATURE_STATUS);
      break;
    }
    retcode = AT_GetEnumStringByIndex(_cam, AT3_TEMPERATURE_STATUS, temp_idx, temp_buf, 128);
    if (retcode != AT_SUCCESS) {
      fprintf(stderr, "Failed to retrieve %ls from camera!\n", AT3_TEMPERATURE_STATUS);
      break;
    }
    retcode = AT_GetFloat(_cam, AT3_SENSOR_TEMPERATURE, &temp);
    if (retcode != AT_SUCCESS) {
      fprintf(stderr, "Failed to retrieve %ls from camera!\n", AT3_SENSOR_TEMPERATURE);
      break;
    }
    if (count % 5 == 0) 
      printf("Temperature status: %ls (%G C)\n", temp_buf, temp);
    if(wcscmp(AT3_STABILISED, temp_buf) == 0 || wcscmp(AT3_COOLER_OFF, temp_buf) == 0) {
      ready = true;
      break;
    }
    if(!is_stable && wcscmp(AT3_NOT_STABILISED, temp_buf) == 0) {
      ready = true;
      break;
    }
    count++;
    sleep(1);
  }

  return ready;
}

bool Driver::get_cooling_status(AT_WC* buffer, int buffer_size) const
{
  return at_get_enum(AT3_TEMPERATURE_STATUS, buffer, buffer_size);
}

bool Driver::get_shutter_mode(AT_WC* buffer, int buffer_size) const
{
  return at_get_enum(AT3_SHUTTERING_MODE, buffer, buffer_size);
}

bool Driver::get_trigger_mode(AT_WC* buffer, int buffer_size) const
{
  return at_get_enum(AT3_TRIGGER_MODE, buffer, buffer_size);
}

bool Driver::get_gain_mode(AT_WC* buffer, int buffer_size) const
{
  return at_get_enum(AT3_PREAMP_GAIN_MODE, buffer, buffer_size);
}

bool Driver::get_readout_rate(AT_WC* buffer, int buffer_size) const
{
  return at_get_enum(AT3_PIXEL_READOUT_RATE, buffer, buffer_size);
}

bool Driver::get_binning_mode(AT_WC* buffer, int buffer_size) const
{
  return at_get_enum(AT3_AOI_BINNING, buffer, buffer_size);
}

bool Driver::get_name(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_CAMERA_NAME, buffer, buffer_size);
}

bool Driver::get_model(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_CAMERA_MODEL, buffer, buffer_size);
}

bool Driver::get_family(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_CAMERA_FAMILY, buffer, buffer_size);
}

bool Driver::get_serial(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_SERIAL_NUMBER, buffer, buffer_size);
}

bool Driver::get_firmware(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_FIRMWARE_VERSION, buffer, buffer_size);
}

bool Driver::get_interface_type(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_INTERFACE_TYPE, buffer, buffer_size);
}

bool Driver::get_sdk_version(AT_WC* buffer, int buffer_size) const
{
  return at_get_string(AT3_SOFTWARE_VERSION, buffer, buffer_size);
}

bool Driver::at_check_implemented(const AT_WC* feature) const
{
  AT_BOOL implemented;
  if (AT_IsImplemented(_cam, feature, &implemented) != AT_SUCCESS) { 
    fprintf(stderr, "Unable to get the implementation status of %ls from the camera\n", feature);
    return false;
  }
  return implemented;
}

bool Driver::at_check_write(const AT_WC* feature) const
{
  AT_BOOL writeable;
  if (AT_IsWritable(_cam, feature, &writeable) != AT_SUCCESS) {
    fprintf(stderr, "Unable to get the write status of %ls from the camera\n", feature);
    return false;
  }
  return writeable;
}

bool Driver::at_get_string(const AT_WC* feature, AT_WC* buffer, int buffer_size) const
{
  return (AT_GetString(_cam, feature, buffer, buffer_size) == AT_SUCCESS);
}

AT_64 Driver::at_get_int(const AT_WC* feature) const
{
  AT_64 value = 0;
  if (AT_GetInt(_cam, feature, &value) != AT_SUCCESS)
    fprintf(stderr, "Failed to retrieve %ls from camera!\n", feature);
  return value;
}

AT_64 Driver::at_get_int_min(const AT_WC* feature) const
{
  AT_64 value = 0;
  if (AT_GetIntMin(_cam, feature, &value) != AT_SUCCESS)
    fprintf(stderr, "Failed to retrieve minimum of %ls from camera!\n", feature);
  return value;
}

AT_64 Driver::at_get_int_max(const AT_WC* feature) const
{
  AT_64 value = 0;
  if (AT_GetIntMax(_cam, feature, &value) != AT_SUCCESS)
    fprintf(stderr, "Failed to retrieve maximum of %ls from camera!\n", feature);
  return value;
}

double Driver::at_get_float(const AT_WC* feature) const
{
  double value = NAN;
  if (AT_GetFloat(_cam, feature, &value) != AT_SUCCESS)
    fprintf(stderr, "Failed to retrieve %ls from camera!\n", feature);
  return value;
}

double Driver::at_get_float_min(const AT_WC* feature) const
{
  double value = NAN;
  if (AT_GetFloatMin(_cam, feature, &value) != AT_SUCCESS)
    fprintf(stderr, "Failed to retrieve minimum of %ls from camera!\n", feature);
  return value;
}

double Driver::at_get_float_max(const AT_WC* feature) const
{
  double value = NAN;
  if (AT_GetFloatMax(_cam, feature, &value) != AT_SUCCESS)
    fprintf(stderr, "Failed to retrieve maximum of %ls from camera!\n", feature);
  return value;
}

bool Driver::at_get_enum(const AT_WC* feature, AT_WC* buffer, int buffer_size) const
{
  int retcode;
  int feature_idx;

  retcode = AT_GetEnumIndex(_cam, feature, &feature_idx);
  if (retcode == AT_SUCCESS) {
    retcode = AT_GetEnumStringByIndex(_cam, feature, feature_idx, buffer, buffer_size);
  }

  return (retcode == AT_SUCCESS);
}

bool Driver::at_set_int(const AT_WC* feature, const AT_64 value)
{
  AT_64 limit;
  if ((AT_GetIntMin(_cam, feature, &limit) != AT_SUCCESS) || (value < limit)) {
    printf("Unable to set %ls to %lld: value is below lower range of %lld\n", feature, value, limit);
    return false;
  }
  if ((AT_GetIntMax(_cam, feature, &limit) != AT_SUCCESS) || (value > limit)) {
    printf("Unable to set %ls to %lld: value is above upper range of %lld\n", feature, value, limit);
    return false;
  }

  int retcode = AT_SetInt(_cam, feature, value);
  if (retcode != AT_SUCCESS)
    fprintf(stderr, "Failed to set integer feature %ls: %s\n", feature, ErrorCodes::name(retcode));

  return (retcode == AT_SUCCESS);
}

bool Driver::at_set_float(const AT_WC* feature, const double value)
{
  double limit;
  if ((AT_GetFloatMin(_cam, feature, &limit) != AT_SUCCESS) || (value < limit)) {
    printf("Unable to set %ls to %G: value is below lower range of %G\n", feature, value, limit);
    return false;
  }
  if ((AT_GetFloatMax(_cam, feature, &limit) != AT_SUCCESS) || (value > limit)) {
    printf("Unable to set %ls to %G: value is above upper range of %G\n", feature, value, limit);
    return false;
  }

  int retcode = AT_SetFloat(_cam, feature, value);
  if (retcode != AT_SUCCESS)
    fprintf(stderr, "Failed to set float feature %ls: %s\n", feature, ErrorCodes::name(retcode));

  return (retcode == AT_SUCCESS);
}

bool Driver::at_set_enum(const AT_WC* feature, const AT_WC* value)
{
  int retcode = AT_SetEnumString(_cam, feature, value);

  if (retcode != AT_SUCCESS)
    fprintf(stderr, "Failed to set enum feature %ls: %s\n", feature, ErrorCodes::name(retcode));

  return (retcode == AT_SUCCESS);
}

bool Driver::at_set_bool(const AT_WC* feature, bool value)
{
  int retcode;
  if (value) retcode = AT_SetBool(_cam, feature, AT_TRUE);
  else retcode = AT_SetBool(_cam, feature, AT_FALSE);

  if (retcode != AT_SUCCESS)
    fprintf(stderr, "Failed to set boolean feature %ls: %s\n", feature, ErrorCodes::name(retcode));

  return (retcode == AT_SUCCESS);
}

#undef LENGTH_FIELD_SIZE
#undef CID_FIELD_SIZE
#undef set_config_bool
#undef set_config_int
#undef set_config_float
#undef set_enum_case
