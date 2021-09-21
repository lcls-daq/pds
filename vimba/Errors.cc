#include "Errors.hh"

#define VIMBA_BOOL(x)  case x: return #x

const char *Pds::Vimba::Bool::name(VmbBool_t value) {
  switch (value) {
    VIMBA_BOOL(VmbBoolFalse);
    VIMBA_BOOL(VmbBoolTrue);
    default: return "Unknown bool value";
  }
}

#undef VIMBA_BOOL

const char *Pds::Vimba::Bool::desc(VmbBool_t value) {
  switch (value) {
    case VmbBoolFalse:  return "False";
    case VmbBoolTrue:   return "True";
    default:            return "Unknown";
  }
}

#define VIMBA_ERROR(x)  case x: return #x

const char *Pds::Vimba::ErrorCodes::name(VmbError_t err) {
  switch (err) {
    VIMBA_ERROR(VmbErrorSuccess);
    VIMBA_ERROR(VmbErrorInternalFault);
    VIMBA_ERROR(VmbErrorApiNotStarted);
    VIMBA_ERROR(VmbErrorNotFound);
    VIMBA_ERROR(VmbErrorBadHandle);
    VIMBA_ERROR(VmbErrorDeviceNotOpen);
    VIMBA_ERROR(VmbErrorInvalidAccess);
    VIMBA_ERROR(VmbErrorBadParameter);
    VIMBA_ERROR(VmbErrorStructSize);
    VIMBA_ERROR(VmbErrorMoreData);
    VIMBA_ERROR(VmbErrorWrongType);
    VIMBA_ERROR(VmbErrorInvalidValue);
    VIMBA_ERROR(VmbErrorTimeout);
    VIMBA_ERROR(VmbErrorOther);
    VIMBA_ERROR(VmbErrorResources);
    VIMBA_ERROR(VmbErrorInvalidCall);
    VIMBA_ERROR(VmbErrorNoTL);
    VIMBA_ERROR(VmbErrorNotImplemented);
    VIMBA_ERROR(VmbErrorNotSupported);
    default: return "Unknown error";
  }
}

#undef VIMBA_ERROR

const char *Pds::Vimba::ErrorCodes::desc(VmbError_t err) {
  switch (err) {
    case VmbErrorSuccess:           return "Success";
    case VmbErrorInternalFault:     return "Unexpected fault in VmbApi or driver";
    case VmbErrorApiNotStarted:     return "API not started";
    case VmbErrorNotFound:          return "Not found";
    case VmbErrorBadHandle:         return "Invalid handle";
    case VmbErrorDeviceNotOpen:     return "Device not open";
    case VmbErrorInvalidAccess:     return "Invalid access";
    case VmbErrorBadParameter:      return "Bad parameter";
    case VmbErrorStructSize:        return "Wrong library version";
    case VmbErrorMoreData:          return "More data returned than memory provided";
    case VmbErrorWrongType:         return "Wrong type";
    case VmbErrorInvalidValue:      return "Invalid value";
    case VmbErrorTimeout:           return "Timeout";
    case VmbErrorOther:             return "TL error";
    case VmbErrorResources:         return "Resource not available";
    case VmbErrorInvalidCall:       return "Invalid call";
    case VmbErrorNoTL:              return "TL not loaded";
    case VmbErrorNotImplemented:    return "Not implemented";
    case VmbErrorNotSupported:      return "Not supported";
    default:                        return "Unknown";
  }
}

#define VIMBA_FRAME_STATUS(x) case x: return #x

const char *Pds::Vimba::FrameStatusCodes::name(VmbFrameStatus_t status) {
  switch (status) {
    VIMBA_FRAME_STATUS(VmbFrameStatusComplete);
    VIMBA_FRAME_STATUS(VmbFrameStatusIncomplete);
    VIMBA_FRAME_STATUS(VmbFrameStatusTooSmall);
    VIMBA_FRAME_STATUS(VmbFrameStatusInvalid);
    default: return "Unknown status";
  }
}

#undef VIMBA_FRAME_STATUS

const char *Pds::Vimba::FrameStatusCodes::desc(VmbFrameStatus_t status) {
  switch (status) {
    case VmbFrameStatusComplete:    return "Frame complete";
    case VmbFrameStatusIncomplete:  return "Frame incomplete";
    case VmbFrameStatusTooSmall:    return "Frame too small";
    case VmbFrameStatusInvalid:     return "Frame invalid";
    default:                        return "Unknown";
  }
}

#define VIMBA_PIXEL_FORMAT(x) case x: return #x

const char *Pds::Vimba::PixelFormatTypes::name(VmbPixelFormat_t format) {
  switch (static_cast<VmbPixelFormatType>(format)) {
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono10p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono12Packed);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono12p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono14);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatMono16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR12Packed);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG12Packed);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB12Packed);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG12Packed);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR10p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG10p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB10p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG10p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR12p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG12p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB12p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG12p);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGR16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerRG16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerGB16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBayerBG16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgb8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgr8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgb10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgr10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgb12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgr12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgb14);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgr14);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgb16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgr16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatArgb8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgra8);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgba10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgra10);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgba12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgra12);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgba14);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgra14);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatRgba16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatBgra16);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatYuv411);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatYuv422);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatYuv444);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatYCbCr411_8_CbYYCrYY);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatYCbCr422_8_CbYCrY);
    VIMBA_PIXEL_FORMAT(VmbPixelFormatYCbCr8_CbYCr);
    default: return "Unknown format";
  }
}

#undef VIMBA_PIXEL_FORMAT

#define VIMBA_PIXEL_FORMAT_DESC(name) case VmbPixelFormat##name: return #name
const char *Pds::Vimba::PixelFormatTypes::desc(VmbPixelFormat_t format) {
  switch (format) {
    VIMBA_PIXEL_FORMAT_DESC(Mono8);
    VIMBA_PIXEL_FORMAT_DESC(Mono10);
    VIMBA_PIXEL_FORMAT_DESC(Mono10p);
    VIMBA_PIXEL_FORMAT_DESC(Mono12);
    VIMBA_PIXEL_FORMAT_DESC(Mono12Packed);
    VIMBA_PIXEL_FORMAT_DESC(Mono12p);
    VIMBA_PIXEL_FORMAT_DESC(Mono14);
    VIMBA_PIXEL_FORMAT_DESC(Mono16);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR8);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG8);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB8);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG8);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR10);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG10);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB10);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG10);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR12);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG12);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB12);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG12);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR12Packed);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG12Packed);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB12Packed);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG12Packed);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR10p);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG10p);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB10p);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG10p);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR12p);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG12p);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB12p);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG12p);
    VIMBA_PIXEL_FORMAT_DESC(BayerGR16);
    VIMBA_PIXEL_FORMAT_DESC(BayerRG16);
    VIMBA_PIXEL_FORMAT_DESC(BayerGB16);
    VIMBA_PIXEL_FORMAT_DESC(BayerBG16);
    VIMBA_PIXEL_FORMAT_DESC(Rgb8);
    VIMBA_PIXEL_FORMAT_DESC(Bgr8);
    VIMBA_PIXEL_FORMAT_DESC(Rgb10);
    VIMBA_PIXEL_FORMAT_DESC(Bgr10);
    VIMBA_PIXEL_FORMAT_DESC(Rgb12);
    VIMBA_PIXEL_FORMAT_DESC(Bgr12);
    VIMBA_PIXEL_FORMAT_DESC(Rgb14);
    VIMBA_PIXEL_FORMAT_DESC(Bgr14);
    VIMBA_PIXEL_FORMAT_DESC(Rgb16);
    VIMBA_PIXEL_FORMAT_DESC(Bgr16);
    VIMBA_PIXEL_FORMAT_DESC(Argb8);
    VIMBA_PIXEL_FORMAT_DESC(Bgra8);
    VIMBA_PIXEL_FORMAT_DESC(Rgba10);
    VIMBA_PIXEL_FORMAT_DESC(Bgra10);
    VIMBA_PIXEL_FORMAT_DESC(Rgba12);
    VIMBA_PIXEL_FORMAT_DESC(Bgra12);
    VIMBA_PIXEL_FORMAT_DESC(Rgba14);
    VIMBA_PIXEL_FORMAT_DESC(Bgra14);
    VIMBA_PIXEL_FORMAT_DESC(Rgba16);
    VIMBA_PIXEL_FORMAT_DESC(Bgra16);
    VIMBA_PIXEL_FORMAT_DESC(Yuv411);
    VIMBA_PIXEL_FORMAT_DESC(Yuv422);
    VIMBA_PIXEL_FORMAT_DESC(Yuv444);
    VIMBA_PIXEL_FORMAT_DESC(YCbCr411_8_CbYYCrYY);
    VIMBA_PIXEL_FORMAT_DESC(YCbCr422_8_CbYCrY);
    VIMBA_PIXEL_FORMAT_DESC(YCbCr8_CbYCr);
    default: return "Unknown";
  }
}

#undef VIMBA_PIXEL_FORMAT_DESC
