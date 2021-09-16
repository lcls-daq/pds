#ifndef Pds_Vimba_Features_hh
#define Pds_Vimba_Features_hh

namespace Pds {
  namespace Vimba {
    static const char* VMB_MODE_ON                = "On";
    static const char* VMB_MODE_OFF               = "Off";
    static const char* VMB_PIXEL_MONO8            = "Mono8";
    static const char* VMB_PIXEL_MONO10           = "Mono10";
    static const char* VMB_PIXEL_MONO10P          = "Mono10p";
    static const char* VMB_PIXEL_MONO12           = "Mono12";
    static const char* VMB_PIXEL_MONO12P          = "Mono12p";
    static const char* VMB_PIXEL_MONO14           = "Mono14";
    static const char* VMB_PIXEL_MONO16           = "Mono16";
    static const char* VMB_ACQ_FRAME_COUNT        = "AcquisitionFrameCount";
    static const char* VMB_ACQ_FRAME_RATE         = "AcquisitionFrameRate";
    static const char* VMB_ACQ_FRAME_RATE_EN      = "AcquisitionFrameRateEnable";
    static const char* VMB_ACQ_FRAME_RATE_MODE    = "AcquisitionFrameRateMode";
    static const char* VMB_ACQ_START              = "AcquisitionStart";
    static const char* VMB_ACQ_STOP               = "AcquisitionStop";
    static const char* VMB_ACQ_STATUS             = "AcquisitionStatus";
    static const char* VMB_ACQ_MODE               = "AcquisitionMode";
    static const char* VMB_BLACK_LEVEL            = "BlackLevel";
    static const char* VMB_BLACK_LEVEL_SEL        = "BlackLevelSelector";
    static const char* VMB_CONT_BRIGHT_LIMIT      = "ContrastBrightLimit";
    static const char* VMB_CONT_CFG_MODE          = "ContrastConfigurationMode";
    static const char* VMB_CONT_DARK_LIMIT        = "ContrastDarkLimit";
    static const char* VMB_CONT_ENABLE            = "ContrastEnable";
    static const char* VMB_CONT_SHAPE             = "ContrastShape";
    static const char* VMB_DEV_FAMILY_NAME        = "DeviceFamilyName";
    static const char* VMB_DEV_FIRMWARE_ID        = "DeviceFirmwareID";
    static const char* VMB_DEV_FIRMWARE_VER       = "DeviceFirmwareVersion";
    static const char* VMB_DEV_GENCP_MAJ_VER      = "DeviceGenCPVersionMajor";
    static const char* VMB_DEV_GENCP_MIN_VER      = "DeviceGenCPVersionMinor";
    static const char* VMB_DEV_INDICATOR_LUM      = "DeviceIndicatorLuminance";
    static const char* VMB_DEV_INDICATOR_MODE     = "DeviceIndicatorMode";
    static const char* VMB_DEV_LINK_CMD_TMO       = "DeviceLinkCommandTimeout";
    static const char* VMB_DEV_LINK_SPEED         = "DeviceLinkSpeed";
    static const char* VMB_DEV_LINK_TP_LIMIT      = "DeviceLinkThroughputLimit";
    static const char* VMB_DEV_LINK_TP_MODE       = "DeviceLinkThroughputLimitMode";
    static const char* VMB_DEV_MANUFACTURER       = "DeviceManufacturerInfo";
    static const char* VMB_DEV_MODEL              = "DeviceModelName";
    static const char* VMB_DEV_RESET              = "DeviceReset";
    static const char* VMB_DEV_SFNC_MAJ_VER       = "DeviceSFNCVersionMajor";
    static const char* VMB_DEV_SFNC_MIN_VER       = "DeviceSFNCVersionMinor";
    static const char* VMB_DEV_SFNC_PATCH_VER     = "DeviceSFNCVersionSubMinor";
    static const char* VMB_DEV_SCAN_TYPE          = "DeviceScanType";
    static const char* VMB_DEV_SERIAL_NUM         = "DeviceSerialNumber";
    static const char* VMB_DEV_TEMPERATURE        = "DeviceTemperature";
    static const char* VMB_DEV_TEMP_SELECTOR      = "DeviceTemperatureSelector";
    static const char* VMB_DEV_TL_MAJ_VER         = "DeviceTLVersionMajor";
    static const char* VMB_DEV_TL_MIN_VER         = "DeviceTLVersionMinor";
    static const char* VMB_DEV_USER_ID            = "DeviceUserID";
    static const char* VMB_DEV_VENDOR             = "DeviceVendorName";
    static const char* VMB_DEV_VERSION            = "DeviceVersion";
    static const char* VMB_GAIN                   = "Gain";
    static const char* VMB_GAIN_SELECTOR          = "GainSelector";
    static const char* VMB_GAMMA                  = "Gamma";
    static const char* VMB_HEIGHT                 = "Height";
    static const char* VMB_HEIGHT_MAX             = "HeightMax";
    static const char* VMB_OFFSET_X               = "OffsetX";
    static const char* VMB_OFFSET_Y               = "OffsetY";
    static const char* VMB_PIXEL_FORMAT           = "PixelFormat";
    static const char* VMB_PIXEL_SIZE             = "PixelSize";
    static const char* VMB_REVERSE_X              = "ReverseX";
    static const char* VMB_REVERSE_Y              = "ReverseY";
    static const char* VMB_SENSOR_HEIGHT          = "SensorHeight";
    static const char* VMB_SENSOR_WIDTH           = "SensorWidth";
    static const char* VMB_SHUTTER_MODE           = "SensorShutterMode";
    static const char* VMB_WIDTH                  = "Width";
    static const char* VMB_WIDTH_MAX              = "WidthMax";
    static const char* VMB_EXPOSURE_MODE          = "ExposureMode";
    static const char* VMB_EXPOSURE_TIME          = "ExposureTime";
    static const char* VMB_LINE_SELECTOR          = "LineSelector";
    static const char* VMB_LINE_MODE              = "LineMode";
    static const char* VMB_PAYLOAD_SIZE           = "PayloadSize";
    static const char* VMB_TRIGGER_ACTIVATION     = "TriggerActivation";
    static const char* VMB_TRIGGER_SELECTOR       = "TriggerSelector";
    static const char* VMB_TRIGGER_SOURCE         = "TriggerSource";
    static const char* VMB_TRIGGER_MODE           = "TriggerMode";
    static const char* VMB_TRIGGER_SOFTWARE       = "TriggerSoftware";
    static const char* VMB_TIMESTAMP_RESET        = "TimestampReset";
    static const char* VMB_TIMESTAMP_LATCH        = "TimestampLatch";
    static const char* VMB_TIMESTAMP_VALUE        = "TimestampLatchValue";
    static const char* VMB_MAX_DRV_BUFFER_COUNT   = "MaxDriverBuffersCount";
    static const char* VMB_STREAM_BUFFER_MIN      = "StreamAnnounceBufferMinimum";
    static const char* VMB_STREAM_BUFFER_COUNT    = "StreamAnnouncedBufferCount";
    static const char* VMB_STREAM_BUFFER_MODE     = "StreamBufferHandlingMode";
    static const char* VMB_CORRECTION_MODE        = "CorrectionMode";
    static const char* VMB_CORRECTION_SELECTOR    = "CorrectionSelector";
    static const char* VMB_CORRECTION_SET         = "CorrectionSet";
    static const char* VMB_CORRECTION_SET_DEF     = "CorrectionSetDefault";
    static const char* VMB_CORRECTION_DATA_SIZE   = "CorrectionDataSize";
    static const char* VMB_CORRECTION_ENTRY_TYPE  = "CorrectionEntryType";
  }
}

#endif
