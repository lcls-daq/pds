#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "pds/config/PixisDataType.hh"
#include "pds/xtc/Datagram.hh"
#include "pds/xtc/CDatagram.hh"
#include "pds/service/Task.hh"
#include "pds/service/Routine.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/Table.hh"

#include "pds/picam/PixisServer.hh"
#include "pds/picam/PixisConfigWrapper.hh"
#include "pds/picam/PiUtils.hh"

using std::string;
using namespace PiUtils;

namespace Pds
{

PixisServer::PixisServer(int iCamera, bool bDelayMode, bool bInitTest, const Src& src, string sConfigDb, int iSleepInt, int iDebugLevel) :
 _iCamera(iCamera), _bDelayMode(bDelayMode), _bInitTest(bInitTest), _src(src),
 _sConfigDb(sConfigDb), _iSleepInt(iSleepInt), _iDebugLevel(iDebugLevel),
 _hCam(NULL), _bCameraInited(false), _bCaptureInited(false),
 _iDetectorWidth(-1), _iDetectorHeight(-1), _iImageWidth(-1), _iImageHeight(-1),
 _fPrevReadoutTime(0), _bSequenceError(false), _clockPrevDatagram(0,0), _iNumExposure(0),
 _config(),
 _config_wrapper(0),
 _fReadoutTime(0),
 _poolFrameData(_iMaxFrameDataSize, _iPoolDataCount), _pDgOut(NULL),
 _CaptureState(CAPTURE_STATE_IDLE), _pTaskCapture(NULL), _routineCapture(*this)
{
  if ( initDevice() != 0 )
    throw PicamServerException( "PixisServer::PixisServer(): initPixis() failed" );

  /*
   * Known issue:
   *
   * If initCaptureTask() is put here, occasionally we will miss some FRAME_COMPLETE event when we do the polling
   * Possible reason is that this function (constructor) is called by a different thread other than the polling thread.
   */
}

PixisServer::~PixisServer()
{
  /*
   * Note: This function may be called by the signal handler, and need to be finished quickly
   */
  if (_hCam == NULL)
    return;

  int iError = Picam_StopAcquisition( _hCam );
  if (!piIsFuncOk(iError))
    printf("PixisServer::~PixisServer(): Picam_StopAcquisition() failed. %s\n", piErrorDesc(iError));

  deinit();

  if ( _pTaskCapture != NULL )
    _pTaskCapture->destroy(); // task object will destroy the thread and release the object memory by itself
}

#define CHECK_PICAM_ERROR(errorCode, strScope) \
if (!piIsFuncOk(errorCode)) \
{ \
  if (errorCode == PicamError_ParameterDoesNotExist) \
    printf("%s: %s\n", strScope, piErrorDesc(iError)); \
  else \
  { \
    printf("%s failed: %s\n", strScope, piErrorDesc(iError)); \
    return ERROR_SDK_FUNC_FAIL; \
  } \
}

int PixisServer::initDevice()
{
  if ( _bCameraInited )
    return 0;

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  int  iError;
  const int iNumMaxTry = 3;

  const PicamCameraID *listCamID;
  int                  iNumCamera;

  int iTry;
  for (iTry = 0; iTry < iNumMaxTry; ++iTry)
  {
    iError = Picam_InitializeLibrary();
    if (iError != 0)
    {
      if (iTry != 0) // Don't print for the first failure, since it usually fails at the first call
        printf("PixisServer::initDevice(): Picam_InitializeLibrary() failed at %d-th try. Error code = %d\n", iTry+1, iError);
      continue;
    }

    printf("Picam_InitializeLibrary() succeeded at %d-th try.\n", iTry+1);
    iError = Picam_GetAvailableCameraIDs( &listCamID, &iNumCamera );
    CHECK_PICAM_ERROR(iError, "PixisServer::initDevice(): Picam_GetAvailableCameraIDs()");

    printf("Found %d Pixis Cameras at %d-th try\n", (int) iNumCamera, iTry+1);
    if (iNumCamera > 0 || _iCamera < 0)
      break;

    iError = Picam_UninitializeLibrary();
    CHECK_PICAM_ERROR(iError, "PixisServer::deinit(): Picam_UninitializeLibrary()");
    sleep(2);
  }

  if (iTry == iNumMaxTry)
    return ERROR_SDK_FUNC_FAIL;

  for (int iCamera=0; iCamera<iNumCamera; ++iCamera)
  {
    const PicamFirmwareDetail*  firmware_array;
    piint                       firmware_count;
    iError = Picam_GetFirmwareDetails( &listCamID[iCamera], &firmware_array, &firmware_count );
    CHECK_PICAM_ERROR(iError, "PixisServer::initDevice(): Picam_GetFirmwareDetails()");

    printf("  camera %d : %s\n", _iCamera, piCameraDesc(listCamID[iCamera]));
    for (int iFirmware = 0; iFirmware < firmware_count; ++iFirmware)
      printf("    firmware [%d] %s : %s\n", iFirmware, firmware_array[iFirmware].name, firmware_array[iFirmware].detail);
    Picam_DestroyFirmwareDetails(firmware_array);
  }

  if (_iCamera >= iNumCamera)
  {
    printf("PixisServer::initDevice(): Invalid Camera selection: %d (max %d)\n", _iCamera, (int) iNumCamera-1);
    Picam_DestroyCameraIDs( listCamID );
    return ERROR_INVALID_CONFIG;
  }
  if (_iCamera < 0) {
    printf("PixisServer::initDevice(): Requested Camera %d. Using a simulated camera\n", _iCamera);
    Picam_ConnectDemoCamera(PicamModel_PixisXF2048B,"0008675309",&_piCameraId);
  } else {
    _piCameraId = listCamID[_iCamera];
  }
  iError = Picam_DestroyCameraIDs( listCamID );
  CHECK_PICAM_ERROR(iError, "PixisServer::initDevice(): Picam_DestroyCameraIDs()");

  printf("Opening camera %d...\n", _iCamera);

  iError = Picam_OpenCamera( &_piCameraId, &_hCam );
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::initDevice(): Picam_OpenCamera() failed: %s\n", piErrorDesc(iError));
    _hCam = NULL;
    return ERROR_SDK_FUNC_FAIL;
  }

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  printInfo();

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );

  double fOpenTime    = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  double fReportTime  = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  printf("Camera Open Time = %6.1lf ms Report Time = %6.1lf ms\n", fOpenTime, fReportTime);

  return 0;
}

int PixisServer::initSetup()
{
  if ( _bCameraInited )
    return 0;

  //Get Detector dimensions
  const PicamRoisConstraint *pRoiConstraint;  // Constraints
  int iError = Picam_GetParameterRoisConstraint( _hCam, PicamParameter_Rois, PicamConstraintCategory_Required, &pRoiConstraint);
  CHECK_PICAM_ERROR(iError, "PixisServer::initSetup(): Picam_GetParameterRoisConstraint()");

  _iDetectorWidth  = pRoiConstraint->width_constraint.maximum;
  _iDetectorHeight = pRoiConstraint->height_constraint.maximum;

   Picam_DestroyRoisConstraints(pRoiConstraint);

  piflt fPixelWidth = -1, fPixelHeight = -1;
  iError = Picam_GetParameterFloatingPointValue( _hCam, PicamParameter_PixelWidth , &fPixelWidth );
  CHECK_PICAM_ERROR(iError, "PixisServer::initSetup(): Picam_GetParameterFloatingPointValue(PicamParameter_PixelWidth)");

  iError = Picam_GetParameterFloatingPointValue( _hCam, PicamParameter_PixelHeight, &fPixelHeight );
  CHECK_PICAM_ERROR(iError, "PixisServer::initSetup(): Picam_GetParameterFloatingPointValue(PicamParameter_PixelHeight)");

  float fTemperature = 999;
  string sTemperatureStatus;
  iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);

  printf( "Detector Width %d Height %d Pixel Width (um) %.2f Height %.2f Temperature %g C  Status %s\n",
    _iDetectorWidth, _iDetectorHeight, fPixelWidth, fPixelHeight, fTemperature, sTemperatureStatus.c_str());

  // setup general parameters
  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ReadoutControlMode, PicamReadoutControlMode_FullFrame);
  CHECK_PICAM_ERROR(iError, "PixisServer::initSetup(): Picam_SetParameterIntegerValue(PicamParameter_ReadoutControlMode)");

  iError = Picam_SetParameterLargeIntegerValue( _hCam, PicamParameter_ReadoutCount, 1);
  CHECK_PICAM_ERROR(iError, "PixisServer::initSetup(): Picam_SetParameterLargeIntegerValue(PicamParameter_ReadoutCount)");

  if (_bInitTest)
  {
    if (initTest() != 0)
      return ERROR_FUNCTION_FAILURE;
  }

  iError = piCommitParameters(_hCam);
  if (iError != 0) return ERROR_SDK_FUNC_FAIL;

  piPrintParameter(_hCam, PicamParameter_ReadoutCount, true);
  piPrintParameter(_hCam, PicamParameter_ReadoutControlMode, true);
  piPrintParameter(_hCam, PicamParameter_TriggerDetermination, true);

  int iFail = initCameraBeforeConfig();
  if (iFail != 0)
  {
    printf("PixisServer::init(): initCameraBeforeConfig() failed!\n");
    return ERROR_FUNCTION_FAILURE;
  }

  fTemperature = 999;
  iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);

  printf( "Detector Width %d Height %d Temperature %g C Status %s\n",
    _iDetectorWidth, _iDetectorHeight, fTemperature, sTemperatureStatus.c_str());

  const int iDevId = ((DetInfo&)_src).devId();
  printf( "Pixis Camera [%d] (device %d) has been initialized\n", iDevId, _iCamera );
  _bCameraInited = true;

  return 0;
}

int PixisServer::printInfo()
{
  piPrintAllParameters(_hCam);
  return 0;
}

int PixisServer::deinit()
{
  // If the camera has been init-ed before, and not deinit-ed yet
  if ( _bCaptureInited )
    deinitCapture(); // deinit the camera explicitly

  if (_hCam != NULL)
  {
    float fTemperature = 999;
    string sTemperatureStatus;
    int iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);

    printf("Current Temperature %g C Status %s\n", fTemperature, sTemperatureStatus.c_str());

    if ( fTemperature < 0 )
      printf("Warning: Temperature is still low (%g C). May results in fast warming.\n", fTemperature);

    iError = Picam_CloseCamera( _hCam );
    CHECK_PICAM_ERROR(iError, "PixisServer::deinit(): Picam_CloseCamera()");

    printf("Picam_CloseCamera() okay\n");

    iError = Picam_UninitializeLibrary();
    CHECK_PICAM_ERROR(iError, "PixisServer::deinit(): Picam_UninitializeLibrary()");

    printf("Picam_UninitializeLibrary() okay\n");
  }

  _bCameraInited = false;

  const int iDevId = ((DetInfo&)_src).devId();
  printf( "Pixis Camera [%d] (device %d) has been deinitialized\n", iDevId, _iCamera );
  return 0;
}

int PixisServer::map()
{
  /*
   * Thread Issue:
   *
   * initCaptureTask() need to be put here. See the comment in the constructor function.
   */
  if ( initCaptureTask() != 0 )
    return ERROR_SERVER_INIT_FAIL;

  return 0;
}

int PixisServer::config(PicamConfig* config, std::string& sConfigWarning)
{
  _config_wrapper = dynamic_cast<PixisConfigWrapper*>(config);

  if ( _config_wrapper )
  {
    return configure(_config_wrapper->config(), sConfigWarning);
  } 
  else
  {
    printf("PixisServer::config(): Invalid configuration type!\n");
    return ERROR_INVALID_CONFIG;
  }
}

int PixisServer::configure(PixisConfigType& config, std::string& sConfigWarning)
{
  if ( configCamera(config, sConfigWarning) != 0 )
    return ERROR_SERVER_INIT_FAIL;

  const int iDevId = ((DetInfo&)_src).devId();

  if ( (int) config.width() > _iDetectorWidth || (int) config.height() > _iDetectorHeight)
  {
    char sMessage[128];
    sprintf( sMessage, "!!! Pixis %d ConfigSize (%d,%d) > Det Size(%d,%d)\n", iDevId, config.width(), config.height(), _iDetectorWidth, _iDetectorHeight);
    printf(sMessage);
    sConfigWarning += sMessage;
    Pds::PixisConfig::setSize(config,_iDetectorWidth,_iDetectorHeight);
  }

  if ( (int) (config.orgX() + config.width()) > _iDetectorWidth ||
       (int) (config.orgY() + config.height()) > _iDetectorHeight)
  {
    char sMessage[128];
    sprintf( sMessage, "!!! Pixis %d ROI Boundary (%d,%d) > Det Size(%d,%d)\n", iDevId,
             config.orgX() + config.width(), config.orgY() + config.height(), _iDetectorWidth, _iDetectorHeight);
    printf(sMessage);
    sConfigWarning += sMessage;
    Pds::PixisConfig::setSize (config,
                               _iDetectorWidth - config.orgX(),
                               _iDetectorHeight - config.orgY());
  }

  _config = config;

  //Note: We don't send error for cooling incomplete
  setupCooling( (double) _config.coolingTemp() );
  //if ( setupCooling() != 0 )
    //return ERROR_SERVER_INIT_FAIL;

  float fTemperature = 999;
  string sTemperatureStatus;
  piReadTemperature(_hCam, fTemperature, sTemperatureStatus);

  printf( "Detector Width %d Height %d Speed %g MHz Gain %d ADC Mode %d Temperature %g C Status %s\n",
    _iDetectorWidth, _iDetectorHeight, _config.readoutSpeed(), _config.gainMode(), _config.adcMode(), fTemperature, sTemperatureStatus.c_str());

  return 0;
}

int PixisServer::unconfig()
{
  return 0;
}

int PixisServer::beginRun()
{
  /*
   * Check data pool status
   */
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "PixisServer::enable(): Memory usage issue. Data Pool is not empty (%d/%d allocated).\n",
      _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );
  else if ( _iDebugLevel >= 3 )
    printf( "BeginRun Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  /*
   * Reset the per-run data
   */
  _fPrevReadoutTime = 0;
  _bSequenceError   = false;
  _iNumExposure     = 0;

  return 0;
}

int PixisServer::endRun()
{
  /*
   * Check data pool status
   */
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "PixisServer::enable(): Memory usage issue. Data Pool is not empty (%d/%d allocated).\n",
      _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );
  else if ( _iDebugLevel >= 3 )
    printf( "EndRun Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  return 0;
}

int PixisServer::beginCalibCycle()
{
  int iFail = initCapture();
  if ( iFail != 0 )
    return ERROR_FUNCTION_FAILURE;

  return 0;
}

int PixisServer::endCalibCycle()
{
  int iFail  = deinitCapture();
  if ( iFail != 0 )
    return ERROR_FUNCTION_FAILURE;

  return 0;
}

int PixisServer::enable()
{
  /*
   * Check data pool status
   */
  if ( _poolFrameData.numberOfAllocatedObjects() > 0 )
    printf( "PixisServer::enable(): Memory usage issue. Data Pool is not empty (%d/%d allocated).\n",
      _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );
  else if ( _iDebugLevel >= 3 )
    printf( "Enable Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  return 0;
}

int PixisServer::disable()
{
  // Note: Here we don't worry if the data pool is not free, because the
  //       traffic shaping thead may still holding the L1 Data
  if ( _iDebugLevel >= 3 )
    printf( "Disable Pool status: %d/%d allocated.\n", _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects() );

  int iFail  = stopCapture();
  if ( iFail != 0 )
    return ERROR_FUNCTION_FAILURE;

  return 0;
}

int PixisServer::initCapture()
{
  if ( _bCaptureInited )
    deinitCapture();

  LockCameraData lockInitCapture(const_cast<char*>("PixisServer::initCapture()"));

  printf("\nInit capture...\n");
  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  int iError = setupROI();
  if (iError != 0)
    return ERROR_FUNCTION_FAILURE;

  if ( (int) _config.frameSize() - (int) sizeof(PixisDataType) + _iFrameHeaderSize > _iMaxFrameDataSize )
  {
    printf( "PixisServer::initCapture(): Frame size (%i) + Frame header size (%d)"
     "is larger than internal data frame buffer size (%d)\n",
     _config.frameSize() - (int) sizeof(PixisDataType), _iFrameHeaderSize, _iMaxFrameDataSize );
    return ERROR_INVALID_CONFIG;
  }

  piflt fTimeReadout = 999;
  iError = Picam_GetParameterFloatingPointValue( _hCam, PicamParameter_ReadoutTimeCalculation , &fTimeReadout );
  CHECK_PICAM_ERROR(iError, "PixisServer::initCapture(): Picam_GetParameterFloatingPointValue(PicamParameter_ReadoutTimeCalculation)");

  piint iImageSize = -1;
  iError = Picam_GetParameterIntegerValue( _hCam, PicamParameter_ReadoutStride, &iImageSize );
  CHECK_PICAM_ERROR(iError, "PixisServer::initTest(): Picam_GetParameterIntegerValue(PicamParameter_ReadoutStride)");

  printf("Estimated Readout time: %f s  Image size = %d B\n", fTimeReadout * 1e-3f, iImageSize);

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );
  double fTimePreAcq    = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;

  _bCaptureInited = true;
  printf( "Capture initialized. Time = %6.1lf ms\n",  fTimePreAcq);

  if ( _iDebugLevel >= 2 )
    printf( "Frame size for image capture = %i. Exposure time = %f s\n",
     _config.frameSize(), _config.exposureTime());

  return 0;
}

int PixisServer::stopCapture()
{
  LockCameraData lockDeinitCapture(const_cast<char*>("PixisServer::stopCapture()"));

  resetFrameData(true);

  int iError = Picam_StopAcquisition( _hCam );
  CHECK_PICAM_ERROR(iError, "PixisServer::stopCapture(): Picam_StopAcquisition()");

  printf( "Capture stopped\n" );
  return 0;
}

int PixisServer::deinitCapture()
{
  if ( !_bCaptureInited )
    return 0;

  stopCapture();

  LockCameraData lockDeinitCapture(const_cast<char*>("PixisServer::deinitCapture()"));

  _bCaptureInited = false;
  return 0;
}

int PixisServer::startCapture()
{
  if ( _pDgOut == NULL )
  {
    printf( "PixisServer::startCapture(): Datagram has not been allocated. No buffer to store the image data\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  int iError = Picam_StartAcquisition( _hCam );
  CHECK_PICAM_ERROR(iError, "PixisServer::startCapture(): Picam_StartAcquisition()");

  return 0;
}

int PixisServer::configCamera(PixisConfigType& config, std::string& sConfigWarning)
{
  int   iError;

  printf("\nConfiguring...\n");

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_AdcAnalogGain, config.gainMode());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_AdcAnalogGain, %d) failed: %s\n",
           config.gainMode(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_AdcQuality, config.adcMode());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_AdcQuality, %d) failed: %s\n",
           config.adcMode(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = piSetParameterToCollection(_hCam, PicamParameter_AdcSpeed, (piflt) config.readoutSpeed());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): piSetParameterToCollection(PicamParameter_AdcSpeed, %g) failed: %s\n",
           config.readoutSpeed(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = piSetParameterToIncrement(_hCam, PicamParameter_ExposureTime, (piflt) config.exposureTime());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): piSetParameterToIncrement(PicamParameter_ExposureTime, %g) failed: %s\n",
           config.exposureTime(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = piSetParameterToCollection(_hCam, PicamParameter_VerticalShiftRate, (piflt) config.vsSpeed());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): piSetParameterToCollection(PicamParameter_VerticalShiftRate, %g) failed: %s\n",
           config.vsSpeed(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  if ((config.triggerMode() == PixisConfigType::External) || (config.triggerMode() == PixisConfigType::ExternalWithCleaning))
  {
    iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_TriggerResponse, PicamTriggerResponse_ReadoutPerTrigger);
    if (!piIsFuncOk(iError))
    {
      printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_TriggerResponse, %d) failed: %s\n",
            PicamTriggerResponse_ReadoutPerTrigger, piErrorDesc(iError));
      return ERROR_INVALID_CONFIG;
    }

    iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_TriggerDetermination, PicamTriggerDetermination_PositivePolarity);
    if (!piIsFuncOk(iError))
    {
      printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_TriggerDetermination, %d) failed: %s\n",
            PicamTriggerDetermination_PositivePolarity, piErrorDesc(iError));
      return ERROR_INVALID_CONFIG;
    }

    iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_CleanUntilTrigger, config.triggerMode() == PixisConfigType::ExternalWithCleaning);
    if (!piIsFuncOk(iError))
    {
      printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_CleanUntilTrigger, %d) failed: %s\n",
            config.triggerMode() == PixisConfigType::ExternalWithCleaning, piErrorDesc(iError));
      return ERROR_INVALID_CONFIG;
    }
  }
  else
  {
    iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_TriggerResponse, PicamTriggerResponse_NoResponse);
    if (!piIsFuncOk(iError))
    {
      printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_TriggerResponse, %d) failed: %s\n",
            PicamTriggerResponse_NoResponse, piErrorDesc(iError));
      return ERROR_INVALID_CONFIG;
    }
  }


  // Active area control
  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ActiveWidth, config.activeWidth());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_ActiveWidth, %d) failed: %s\n",
           config.activeWidth(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ActiveHeight, config.activeHeight());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_ActiveHeight, %d) failed: %s\n",
           config.activeHeight(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ActiveTopMargin, config.activeTopMargin());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_ActiveTopMargin, %d) failed: %s\n",
           config.activeTopMargin(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ActiveBottomMargin, config.activeBottomMargin());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_ActiveBottomMargin, %d) failed: %s\n",
           config.activeBottomMargin(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ActiveLeftMargin, config.activeLeftMargin());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_ActiveLeftMargin, %d) failed: %s\n",
           config.activeLeftMargin(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ActiveRightMargin, config.activeRightMargin());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_ActiveRightMargin, %d) failed: %s\n",
           config.activeRightMargin(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }
  
  /*
   * Disable shutter control
   */
  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ShutterTimingMode, PicamShutterTimingMode_AlwaysClosed);
  CHECK_PICAM_ERROR(iError, "PixisServer::initCapture(): Picam_SetParameterIntegerValue(PicamParameter_ShutterTimingMode)");

  iError = Picam_SetParameterFloatingPointValue(_hCam, PicamParameter_ShutterOpeningDelay, 0.0f );
  CHECK_PICAM_ERROR(iError, "PixisServer::initCapture(): Picam_SetParameterFloatingPointValue(PicamParameter_ShutterOpeningDelay)");

  iError = Picam_SetParameterFloatingPointValue(_hCam, PicamParameter_ShutterClosingDelay, 0.0f );
  CHECK_PICAM_ERROR(iError, "PixisServer::initCapture(): Picam_SetParameterFloatingPointValue(PicamParameter_ShutterClosingDelay)");

  // Cleaning control
  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_CleanSerialRegister, 1);
  CHECK_PICAM_ERROR(iError, "PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_CleanSerialRegister)");

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_CleanCycleCount, config.cleanCycleCount());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_CleanCycleCount, %d) failed: %s\n",
           config.cleanCycleCount(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_CleanCycleHeight, config.cleanCycleHeight());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_CleanCycleHeight, %d) failed: %s\n",
           config.cleanCycleHeight(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_CleanSectionFinalHeight, config.cleanFinalHeight());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_CleanSectionFinalHeight, %d) failed: %s\n",
           config.cleanFinalHeight(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_CleanSectionFinalHeightCount, config.cleanFinalHeightCount());
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::configCamera(): Picam_SetParameterIntegerValue(PicamParameter_CleanSectionFinalHeightCount, %d) failed: %s\n",
           config.cleanFinalHeightCount(), piErrorDesc(iError));
    return ERROR_INVALID_CONFIG;
  }

  iError = piCommitParameters(_hCam);
  if (iError != 0) return ERROR_SDK_FUNC_FAIL;

  piPrintParameter(_hCam, PicamParameter_ExposureTime, true);
  piPrintParameter(_hCam, PicamParameter_AdcAnalogGain, true);
  piPrintParameter(_hCam, PicamParameter_AdcSpeed, true);
  piPrintParameter(_hCam, PicamParameter_AdcQuality, true);
  piPrintParameter(_hCam, PicamParameter_VerticalShiftRate, true);
  piPrintParameter(_hCam, PicamParameter_TriggerResponse, true);
  if ( _iDebugLevel >= 3 ) {
    piPrintParameter(_hCam, PicamParameter_ActiveWidth, true);
    piPrintParameter(_hCam, PicamParameter_ActiveHeight, true);
    piPrintParameter(_hCam, PicamParameter_ActiveTopMargin, true);
    piPrintParameter(_hCam, PicamParameter_ActiveBottomMargin, true);
    piPrintParameter(_hCam, PicamParameter_ActiveLeftMargin, true);
    piPrintParameter(_hCam, PicamParameter_ActiveRightMargin, true);
    piPrintParameter(_hCam, PicamParameter_CleanUntilTrigger, true);
    piPrintParameter(_hCam, PicamParameter_CleanCycleCount, true);
    piPrintParameter(_hCam, PicamParameter_CleanCycleHeight, true);
    piPrintParameter(_hCam, PicamParameter_CleanSectionFinalHeight, true);
    piPrintParameter(_hCam, PicamParameter_CleanSectionFinalHeightCount, true);
  }

  return 0;
}

int PixisServer::initCameraBeforeConfig()
{
  if (_sConfigDb.empty())
    return 0;

  size_t iIndexComma = _sConfigDb.find_first_of(',');

  string sConfigPath, sConfigType;
  if (iIndexComma == string::npos)
  {
    sConfigPath = _sConfigDb;
    sConfigType = "BEAM";
  }
  else
  {
    sConfigPath = _sConfigDb.substr(0, iIndexComma);
    sConfigType = _sConfigDb.substr(iIndexComma+1, string::npos);
  }

  printf("PixisServer::initCameraBeforeConfig(): Getting runkey from path %s\n", sConfigPath.c_str());
  // Setup ConfigDB and Run Key
  int runKey;
  try
  { Pds_ConfigDb::Experiment expt(sConfigPath.c_str(),
                                  Pds_ConfigDb::Experiment::NoLock);
    expt.read();
    const Pds_ConfigDb::TableEntry* entry = expt.table().get_top_entry(sConfigType);
    if (entry == NULL)
      {
        printf("PixisServer::initCameraBeforeConfig(): Invalid config db path [%s] type [%s]\n",sConfigPath.c_str(), sConfigType.c_str());
        return ERROR_FUNCTION_FAILURE;
      }
    runKey = strtoul(entry->key().c_str(),NULL,16);
  }
  catch (const std::exception& except)
  {
    printf("PixisServer::initCameraBeforeConfig(): Experiment read failed: %s\n", except.what());
    return 0;
  }

  printf("PixisServer::initCameraBeforeConfig(): runkey == %d\n", runKey);

  const TypeId typePixisConfig = TypeId(TypeId::Id_PixisConfig, PixisConfigType::Version);

  CfgClientNfs client(_src);
  client.initialize(Allocation("none",sConfigPath.c_str(),0));
  Transition tr(TransitionId::BeginRun, Env(runKey));

  PixisConfigType config;
  int iSizeRead = client.fetch(tr, typePixisConfig, &config, sizeof(config));
  if (iSizeRead != sizeof(config))
  {
    printf("PixisServer::initCameraBeforeConfig(): Read config data of incorrect size. Read size = %d (should be %d) bytes\n",
      iSizeRead, (int) sizeof(config));
    return ERROR_FUNCTION_FAILURE;
  }

  printf("Setting cooling temperature: %g C\n", config.coolingTemp());
  setupCooling( (double) config.coolingTemp() );

  return 0;
}

int PixisServer::initTest()
{
  printf( "Running init test...\n" );

  timespec timeVal0;
  clock_gettime( CLOCK_REALTIME, &timeVal0 );

  int iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_TriggerResponse, PicamTriggerResponse_NoResponse);
  CHECK_PICAM_ERROR(iError, "PixisServer::initTest(): Picam_SetParameterIntegerValue(PicamParameter_TriggerResponse)");

  PicamRoi roi =
  {
    0,                // x
    _iDetectorWidth,  // width
    1,                // x binning
    0,                // y
    _iDetectorHeight, // height
    1,                // y binning
  };
  PicamRois rois = { &roi, 1 };

  iError = Picam_SetParameterRoisValue( _hCam, PicamParameter_Rois, &rois);
  CHECK_PICAM_ERROR(iError, "PixisServer::initTest(): Picam_SetParameterRoisValue()");

  iError = piCommitParameters(_hCam);
  if (iError != 0) return ERROR_SDK_FUNC_FAIL;

  piflt fTimeReadout = 999;
  iError = Picam_GetParameterFloatingPointValue( _hCam, PicamParameter_ReadoutTimeCalculation , &fTimeReadout );
  CHECK_PICAM_ERROR(iError, "PixisServer::initTest(): Picam_GetParameterFloatingPointValue(PicamParameter_ReadoutTimeCalculation)");

  piint iImageSize = -1;
  iError = Picam_GetParameterIntegerValue( _hCam, PicamParameter_ReadoutStride, &iImageSize );
  CHECK_PICAM_ERROR(iError, "PixisServer::initTest(): Picam_GetParameterIntegerValue(PicamParameter_ReadoutStride)");

  printf("Estimated Readout time: %f s  Image size = %d B\n", fTimeReadout * 1e-3f, iImageSize);

  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  for (int iTry = 0; iTry < 3; ++iTry)
  {
    timespec timeVal2;
    clock_gettime( CLOCK_REALTIME, &timeVal2 );

    iError = Picam_StartAcquisition( _hCam );
    CHECK_PICAM_ERROR(iError, "PixisServer::initTest(): Picam_StartAcquisition()");

    timespec timeVal3;
    clock_gettime( CLOCK_REALTIME, &timeVal3 );

    printf("getting frame [%d]... ", iTry);
    unsigned char* pData;
    iError = piWaitAcquisitionUpdate(_hCam, _iMaxReadoutTime, false, true, &pData);
    printf(" done. data = %p.", pData);

    timespec timeVal4;
    clock_gettime( CLOCK_REALTIME, &timeVal4 );

    // another wait to clean the acquisition status (for PI-CAM library)
    iError = piWaitAcquisitionUpdate(_hCam, _iMaxReadoutTime, true, true, &pData);

    timespec timeVal5;
    clock_gettime( CLOCK_REALTIME, &timeVal5 );

    double fSetupTime   = (timeVal3.tv_nsec - timeVal2.tv_nsec) * 1.e-6 + ( timeVal3.tv_sec - timeVal2.tv_sec ) * 1.e3;
    double fReadoutTime = (timeVal4.tv_nsec - timeVal3.tv_nsec) * 1.e-6 + ( timeVal4.tv_sec - timeVal3.tv_sec ) * 1.e3;
    double fCleanAcqTime= (timeVal5.tv_nsec - timeVal4.tv_nsec) * 1.e-6 + ( timeVal5.tv_sec - timeVal4.tv_sec ) * 1.e3;
    printf(" Capture Setup Time = %6.1lfms Exposure+Readout Time = %6.1lfms CleanAcq Time = %6.1lfms\n",
           fSetupTime, fReadoutTime, fCleanAcqTime );
  }

  timespec timeVal6;
  clock_gettime( CLOCK_REALTIME, &timeVal6 );

  double fInitTime  = (timeVal1.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal1.tv_sec - timeVal0.tv_sec ) * 1.e3;
  double fTotalTime = (timeVal6.tv_nsec - timeVal0.tv_nsec) * 1.e-6 + ( timeVal6.tv_sec - timeVal0.tv_sec ) * 1.e3;
  printf("Capture Init Time = %6.1lfms Total Time = %6.1lfms\n",
    fInitTime, fTotalTime );

  return 0;
}

int PixisServer::setupCooling(double fCoolingTemperature)
{
  int iError;

  float fTemperature = 999;
  string sTemperatureStatus;
  iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);
  printf("Temperature Before cooling: %g C  Status %s\n", fTemperature, sTemperatureStatus.c_str());


  iError = piSetParameterToIncrement(_hCam, PicamParameter_SensorTemperatureSetPoint, fCoolingTemperature);
  if (!piIsFuncOk(iError))
  {
    printf("PixisServer::setupCooling(): piSetParameterToIncrement(PicamParameter_SensorTemperatureSetPoint, %g) failed: %s\n",
           fCoolingTemperature, piErrorDesc(iError));
    return ERROR_SDK_FUNC_FAIL;
  }

  iError = piCommitParameters(_hCam);
  if (iError != 0) return ERROR_SDK_FUNC_FAIL;

  piPrintParameter(_hCam, PicamParameter_SensorTemperatureSetPoint, true);

  printf("Set Temperature to %g C\n", fCoolingTemperature);

  const static timeval timeSleepMicroOrg = {0, 5000}; // 5 millisecond
  timespec timeVal1;
  clock_gettime( CLOCK_REALTIME, &timeVal1 );

  int iNumLoop       = 0;
  int iNumRepateRead = 5;
  int iRead          = 0;

  while (1)
  {
    fTemperature = 999;
    iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);

    if ( fTemperature <= fCoolingTemperature )
    {
      if ( ++iRead >= iNumRepateRead )
        break;
    }
    else
      iRead = 0;

    if ( (iNumLoop+1) % 200 == 0 )
      printf("Temperature *Updating*: %g C\n", fTemperature );

    timespec timeValCur;
    clock_gettime( CLOCK_REALTIME, &timeValCur );
    int iWaitTime = (timeValCur.tv_nsec - timeVal1.tv_nsec) / 1000000 +
     ( timeValCur.tv_sec - timeVal1.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime > _iMaxCoolingTime ) break;

    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg;
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);
    iNumLoop++;
  }

  timespec timeVal2;
  clock_gettime( CLOCK_REALTIME, &timeVal2 );
  double fCoolingTime = (timeVal2.tv_nsec - timeVal1.tv_nsec) * 1.e-6 + ( timeVal2.tv_sec - timeVal1.tv_sec ) * 1.e3;
  printf("Cooling Time = %6.1lf ms\n", fCoolingTime);

  fTemperature = 999;
  iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);
  printf("Temperature After cooling: %g C  Status %s\n", fTemperature, sTemperatureStatus.c_str());

  if ( fTemperature > fCoolingTemperature )
  {
    printf("PixisServer::setupCooling(): Cooling temperature not reached yet; final temperature = %g C",
     fTemperature );
    return ERROR_TEMPERATURE;
  }

  return 0;
}

int PixisServer::initCaptureTask()
{
  if ( _pTaskCapture != NULL )
    return 0;

  if ( ! _bDelayMode ) // Prompt mode doesn't need to initialize the capture task, because the task will be performed in the event handler
    return 0;

  _pTaskCapture = new Task(TaskObject("PixisServer"));

  return 0;
}

int PixisServer::runCaptureTask()
{
  if ( _CaptureState != CAPTURE_STATE_RUN_TASK )
  {
    printf( "PixisServer::runCaptureTask(): _CaptureState = %d. Capture task is not initialized correctly\n",
     _CaptureState );
    return ERROR_INCORRECT_USAGE;
  }

  LockCameraData lockCaptureProcess(const_cast<char*>("PixisServer::runCaptureTask(): Start data polling and processing" ));

  /*
   * Check if current run is being reset or program is exiting
   */
  if ( !_bCaptureInited )
  {
    resetFrameData(true);
    return 0;
  }

  /*
   * Check if the datagram and frame data have been properly set up
   *
   * Note: This condition should NOT happen normally. Here is a logical check
   * and will output warnings.
   */
  if ( _pDgOut == NULL )
  {
    printf( "PixisServer::runCaptureTask(): Datagram or frame data have not been properly set up before the capture task\n" );
    resetFrameData(true);
    return ERROR_INCORRECT_USAGE;
  }

  //!!!debug
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */
  char      sTimeText[40];
  timespec  timeCurrent;
  time_t    timeSeconds ;

  int iFail = 0;
  do
  {
    ////!!!debug
    //clock_gettime( CLOCK_REALTIME, &timeCurrent );
    //timeSeconds = timeCurrent.tv_sec;
    //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    //printf("PixisServer::runCaptureTask(): Before waitForNewFrameAvailable(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

    iFail  = waitForNewFrameAvailable();

    ////!!!debug
    //clock_gettime( CLOCK_REALTIME, &timeCurrent );
    //timeSeconds = timeCurrent.tv_sec;
    //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    //printf("PixisServer::runCaptureTask(): After waitForNewFrameAvailable(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

    // Even if waitForNewFrameAvailable() failed, we still fill in the frame data with ShotId information
    iFail |= processFrame();

    //!!!debug
    clock_gettime( CLOCK_REALTIME, &timeCurrent );
    timeSeconds = timeCurrent.tv_sec;
    strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    printf("PixisServer::runCaptureTask(): After processFrame(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);
  }
  while (false);

  if ( iFail != 0 )
  {
    // set damage bit, and still keep the image data
    _pDgOut->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

    // // Old ways: delete image data and set the capture state to IDLE
    //resetFrameData(true);
    //return ERROR_FUNCTION_FAILURE;
  }

  /*
   * Set the damage bit if
   *   1. temperature status is not good
   *   2. sequence error happened in the current run
   */
  // Note: Dont send damage when temperature is high
  updateTemperatureData();
  //if ( updateTemperatureData() != 0 )
  //  _pDgOut->datagram().xtc.damage.increase(Pds::Damage::UserDefined);

  _CaptureState = CAPTURE_STATE_DATA_READY;

  //!!!debug
  clock_gettime( CLOCK_REALTIME, &timeCurrent );
  timeSeconds = timeCurrent.tv_sec;
  strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
  printf("After capture: Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

  return 0;
}

int PixisServer::startExposure()
{
  ++_iNumExposure; // update event counter

  /*
   * Chkec if we are allowed to add a new catpure task
   */
  if ( _CaptureState != CAPTURE_STATE_IDLE  )
  {

    /*
     * Chkec if the data is ready, but has not been transfered out
     *
     * Note: This should NOT happen normally, unless the L1Accept handler didn't get
     * the data out when the previous data is ready. Here is a logical check
     * and will output warnings.
     */
    if ( _CaptureState == CAPTURE_STATE_DATA_READY )
    {
      printf( "PixisServer::startExposure(): Previous image data has not been sent out\n" );
      resetFrameData(true);
      return ERROR_INCORRECT_USAGE;
    }

    /*
     * Remaning case:  _CaptureState == CAPTURE_STATE_RUN_TASK
     * capture thread is still busy polling or processing the previous image data
     *
     * Note: Originally, this is a possible case for software adaptive mode, where the
     * L1Accept event are raised no matter whether the polling thread is busy or not.
     *
     * However, since current EVR implementation doesn't support software adaptive mode,
     * this case is NOT a normal case.
     */

    printf( "PixisServer::startExposure(): Capture task is running. It is impossible to start a new capture.\n" );

    /*
     * Here we don't reset the frame data, because the capture task is running and will use the data later
     */
    return ERROR_INCORRECT_USAGE; // No error for adaptive mode
  }

  //!!!debug
  static const char sTimeFormat[40] = "%02d_%02H:%02M:%02S"; /* Time format string */
  char      sTimeText[40];
  timespec  timeCurrent;
  time_t    timeSeconds ;

  ////!!!debug
  //clock_gettime( CLOCK_REALTIME, &timeCurrent );
  //timeSeconds = timeCurrent.tv_sec;
  //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
  //printf("PixisServer::startExposure(): Before setupFrame(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

  int iFail = 0;
  do
  {
    iFail = setupFrame();
    if ( iFail != 0 ) break;

    ////!!!debug
    //clock_gettime( CLOCK_REALTIME, &timeCurrent );
    //timeSeconds = timeCurrent.tv_sec;
    //strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    //printf("PixisServer::startExposure(): After setupFrame(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);

    iFail = startCapture();

    //!!!debug
    clock_gettime( CLOCK_REALTIME, &timeCurrent );
    timeSeconds = timeCurrent.tv_sec;
    strftime(sTimeText, sizeof(sTimeText), sTimeFormat, localtime(&timeSeconds));
    printf("PixisServer::startExposure(): After startCapture(): Local Time: %s.%09ld\n", sTimeText, timeCurrent.tv_nsec);
  }
  while (false);

  if ( iFail != 0 )
  {
    resetFrameData(true);
    return ERROR_FUNCTION_FAILURE;
  }

  _CaptureState = CAPTURE_STATE_RUN_TASK;

  if (_bDelayMode)
    _pTaskCapture->call( &_routineCapture );
  else
    runCaptureTask();

  return 0;
}

int PixisServer::getData(InDatagram* in, InDatagram*& out)
{
  out = in; // Default: return empty stream

  if ( _CaptureState != CAPTURE_STATE_DATA_READY )
    return 0;

  /*
   * Check if the datagram and frame data have been properly set up
   *
   * Note: This condition should NOT happen normally. Here is a logical check
   * and will output warnings.
   */
  if ( _pDgOut == NULL )
  {
    printf( "PixisServer::getData(): Datagram is not properly set up\n" );
    resetFrameData(true);
    return ERROR_LOGICAL_FAILURE;
  }


  Datagram& dgIn  = in->datagram();
  Datagram& dgOut = _pDgOut->datagram();

  /*
   * Backup the orignal Xtc data
   *
   * Note that it is not correct to use  Xtc xtc1 = xtc2, which means using the Xtc constructor,
   * instead of the copy operator to do the copy. In this case, the xtc data size will be set to 0.
   */
  Xtc xtcOutBkp;
  xtcOutBkp = dgOut.xtc;

  /*
   * Compose the datagram
   *
   *   1. Use the header from dgIn
   *   2. Use the xtc (data header) from dgOut
   *   3. Use the data from dgOut (the data was located right after the xtc)
   */
  dgOut     = dgIn;

  //dgOut.xtc = xtcOutBkp; // not okay for command
  dgOut.xtc.damage = xtcOutBkp.damage;
  dgOut.xtc.extent = xtcOutBkp.extent;

  unsigned char*  pFrameHeader  = (unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc);
  PixisDataType* pData = (PixisDataType*) pFrameHeader;
  new (pFrameHeader) PixisDataType(in->datagram().seq.stamp().fiducials(), _fReadoutTime, pData->temperature());

  out       = _pDgOut;

  /*
   * Reset the frame data, without releasing the output data
   *
   * Note:
   *   1. _pDgOut will be set to NULL, so that the same data will never be sent out twice
   *   2. _pDgOut will not be released, because the data need to be sent out for use.
   */
  resetFrameData(false);

  // Delayed data sending for multiple pimax cameras, to avoid creating a burst of traffic
  timeval timeSleepMicro = {0, 1000 * _iSleepInt}; // (_iSleepInt) milliseconds
  select( 0, NULL, NULL, NULL, &timeSleepMicro);

  return 0;
}

int PixisServer::waitData(InDatagram* in, InDatagram*& out)
{
  out = in; // Default: return empty stream

  if ( _CaptureState == CAPTURE_STATE_IDLE )
    return 0;

  static timespec tsWaitStart;
  clock_gettime( CLOCK_REALTIME, &tsWaitStart );

  const static timeval timeSleepMicroOrg = {0, 1000}; // 1 milliseconds

  while (_CaptureState != CAPTURE_STATE_DATA_READY)
  {
    // This data will be modified by select(), so need to be reset
    timeval timeSleepMicro = timeSleepMicroOrg;
    // Use select() to simulate nanosleep(), because experimentally select() controls the sleeping time more precisely
    select( 0, NULL, NULL, NULL, &timeSleepMicro);


    timespec tsCurrent;
    clock_gettime( CLOCK_REALTIME, &tsCurrent );

    int iWaitTime = (tsCurrent.tv_nsec - tsWaitStart.tv_nsec) / 1000000 +
     ( tsCurrent.tv_sec - tsWaitStart.tv_sec ) * 1000; // in milliseconds
    if ( iWaitTime >= _iMaxLastEventTime )
    {
      printf( "PixisServer::waitData(): Waiting time is too long. Skip the final data\n" );
      return ERROR_FUNCTION_FAILURE;
    }
  } // while (1)

  return getData(in, out);
}

bool PixisServer::IsCapturingData()
{
  return ( _CaptureState != CAPTURE_STATE_IDLE );
}

int PixisServer::waitForNewFrameAvailable()
{
  static timespec tsWaitStart;
  clock_gettime( CLOCK_REALTIME, &tsWaitStart );

  unsigned char* pData;
  int iError = piWaitAcquisitionUpdate(_hCam, _iMaxReadoutTime, false, true, &pData);
  if (iError != 0)
  {
    timespec tsAcqComplete;
    clock_gettime( CLOCK_REALTIME, &tsAcqComplete );
    _fReadoutTime = (tsAcqComplete.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsAcqComplete.tv_sec - tsWaitStart.tv_sec ); // in seconds

    return ERROR_SDK_FUNC_FAIL;
  }

  // Data returned by Picam_WaitForAcquisitionUpdate is only valid until the next call, so memcpy the data first!
  uint8_t* pImage = (uint8_t*) _pDgOut + _iFrameHeaderSize;
  memcpy(pImage, pData, _iImageWidth*_iImageHeight * 2);

  // another wait to clean the acquisition status (for PI-CAM library)
  unsigned char* pDummyData;
  iError = piWaitAcquisitionUpdate(_hCam, _iMaxReadoutTime, true, true, &pDummyData);
  if (iError != 0)
  {
    timespec tsAcqComplete;
    clock_gettime( CLOCK_REALTIME, &tsAcqComplete );
    _fReadoutTime = (tsAcqComplete.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsAcqComplete.tv_sec - tsWaitStart.tv_sec ); // in seconds

    return ERROR_SDK_FUNC_FAIL;
  }

  timespec tsWaitEnd;
  clock_gettime( CLOCK_REALTIME, &tsWaitEnd );
  _fReadoutTime = (tsWaitEnd.tv_nsec - tsWaitStart.tv_nsec) / 1.0e9 + ( tsWaitEnd.tv_sec - tsWaitStart.tv_sec ); // in seconds

  // Report the readout time for the first few L1 events
  if ( _iNumExposure <= _iMaxEventReport )
    printf( "Readout time report [%d]: %.2f s  Non-exposure time %.2f s\n", _iNumExposure, _fReadoutTime,
      _fReadoutTime - _config.exposureTime());

  return 0;
}

PicamConfig* PixisServer::config()
{
  return _config_wrapper;
}

int PixisServer::processFrame()
{
  if ( _pDgOut == NULL )
  {
    printf( "PixisServer::startCapture(): Datagram has not been allocated. No buffer to store the image data\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  if ( _iNumExposure <= _iMaxEventReport ||  _iDebugLevel >= 5 )
  {
    unsigned char*  pFrameHeader    = (unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc);
    PixisDataType* pFrame           = (PixisDataType*) pFrameHeader;
    const uint16_t*     pPixel      = pFrame->data(_config).data();
    //int                 iWidth   = (int) ( (_config.width()  + _config.binX() - 1 ) / _config.binX() );
    //int                 iHeight  = (int) ( (_config.height() + _config.binY() - 1 ) / _config.binY() );
    const uint16_t*     pEnd        = (const uint16_t*) ( (unsigned char*) pFrame->data(_config).data() + _config.frameSize() );
    const uint64_t      uNumPixels  = (uint64_t) (_config.frameSize() / sizeof(uint16_t) );

    uint64_t            uSum    = 0;
    uint64_t            uSumSq  = 0;
    for ( ; pPixel < pEnd; pPixel++ )
    {
      uSum   += *pPixel;
      uSumSq += ((uint32_t)*pPixel) * ((uint32_t)*pPixel);
    }

    printf( "Frame Avg Value = %.2lf  Std = %.2lf\n",
      (double) uSum / (double) uNumPixels,
      sqrt( (uNumPixels * uSumSq - uSum * uSum) / (double)(uNumPixels*uNumPixels)) );
  }

  return 0;
}

int PixisServer::setupFrame()
{
  if ( _poolFrameData.numberOfFreeObjects() <= 0 )
  {
    printf( "PixisServer::setupFrame(): Pool is full, and cannot provide buffer for new datagram\n" );
    return ERROR_LOGICAL_FAILURE;
  }

  const int iFrameSize =_config.frameSize();

  /*
   * Set the output datagram pointer
   *
   * Note: This pointer will be used in processFrame(). In the case of delay mode,
   * processFrame() will be exectued in another thread.
   */
  InDatagram*& out = _pDgOut;
  //
  //  Fake a datagram header.  The real header will come with the L1Accept.
  //
  out =
    new ( &_poolFrameData ) CDatagram( TypeId(TypeId::Any,0), DetInfo(0,DetInfo::NoDetector,0,DetInfo::NoDevice,0) );

  out->datagram().xtc.alloc( sizeof(Xtc) + iFrameSize );

  if ( _iDebugLevel >= 3 )
  {
    printf( "PixisServer::setupFrame(): pool status: %d/%d allocated, datagram: %p\n",
     _poolFrameData.numberOfAllocatedObjects(), _poolFrameData.numberofObjects(), _pDgOut  );
  }

  /*
   * Set frame object
   */
  unsigned char* pcXtcFrame = (unsigned char*) _pDgOut + sizeof(CDatagram);

  Xtc* pXtcFrame =
   new ((char*)pcXtcFrame) Xtc(_pixisDataType, _src);
  pXtcFrame->alloc( iFrameSize );

  return 0;
}

int PixisServer::resetFrameData(bool bDelOutDatagram)
{
  /*
   * Update Capture Thread Control and I/O variables
   */
  _CaptureState     = CAPTURE_STATE_IDLE;

  /*
   * Reset buffer data
   */
  if (bDelOutDatagram)  delete _pDgOut;
  _pDgOut = NULL;

  return 0;
}

int PixisServer::setupROI()
{
  printf("ROI (%d,%d) W %d/%d H %d/%d ", _config.orgX(), _config.orgY(),
    _config.width(), _config.binX(), _config.height(), _config.binY());

  _iImageWidth  = _config.width()  / _config.binX();
  _iImageHeight = _config.height() / _config.binY();
  printf("Image size: W %d H %d\n", _iImageWidth, _iImageHeight);

  //setup readout control. Only need when we need to switch between full frame and kinetics mode
  //iError = Picam_SetParameterIntegerValue(_hCam, PicamParameter_ReadoutControlMode, PicamReadoutControlMode_FullFrame);
  //CHECK_PICAM_ERROR(iError, "PixisServer::initSetup(): Picam_SetParameterIntegerValue(PicamParameter_ReadoutControlMode)");

  PicamRoi roi =
  {
    (piint) _config.orgX(),   // x
    (piint) _config.width(),  // width
    (piint) _config.binX(),   // x binning
    (piint) _config.orgY(),   // y
    (piint) _config.height(), // height
    (piint) _config.binY(),   // y binning
  };
  PicamRois rois = { &roi, 1 };

  int iError = Picam_SetParameterRoisValue( _hCam, PicamParameter_Rois, &rois);
  CHECK_PICAM_ERROR(iError, "PixisServer::setupROI(): Picam_SetParameterRoisValue()");

  iError = piCommitParameters(_hCam);
  if (iError != 0) return ERROR_SDK_FUNC_FAIL;

  piPrintParameter(_hCam, PicamParameter_Rois, true);

  return 0;
}

int PixisServer::updateTemperatureData()
{
  int iError;
  float fTemperature = 999;
  string sTemperatureStatus;

  if (_config.infoReportInterval() <= 0 || ((_iNumExposure-1) % _config.infoReportInterval()) != 0)
    fTemperature = 999;
  else
  {
    iError = piReadTemperature(_hCam, fTemperature, sTemperatureStatus);
    if (iError != 0)
      printf("PixisServer::updateTemperatureData(): piReadTemperature() failed. %s\n", piErrorDesc(iError));
    printf( "Detector Temperature report [%d]: %g C Status %s\n", _iNumExposure, fTemperature, sTemperatureStatus.c_str() );
  }


  if ( _pDgOut == NULL )
  {
    printf( "PixisServer::updateTemperatureData(): Datagram has not been allocated. No buffer to store the info data\n" );
  }
  else
  {
    PixisDataType*  pPixisData       = (PixisDataType*) ((unsigned char*) _pDgOut + sizeof(CDatagram) + sizeof(Xtc));
    Pds::PixisData::setTemperature( *pPixisData, fTemperature );
  }

  if (  fTemperature >= _config.coolingTemp() + _fTemperatureHiTol ||
        fTemperature <= _config.coolingTemp() - _fTemperatureLoTol )
  {
    printf( "** PixisServer::updateTemperatureData(): Detector temperature (%g C) is not fixed to the configuration (%g C)\n",
      fTemperature, _config.coolingTemp() );
    return ERROR_TEMPERATURE;
  }

  return 0;
}

/*
 * Definition of private static consts
 */
const int       PixisServer::_iMaxCoolingTime;
const int       PixisServer::_fTemperatureHiTol;
const int       PixisServer::_fTemperatureLoTol;
const int       PixisServer::_iFrameHeaderSize      = sizeof(CDatagram) + sizeof(Xtc) + sizeof(PixisDataType);
const int       PixisServer::_iMaxFrameDataSize     = _iFrameHeaderSize + 2048*2048*2;
const int       PixisServer::_iPoolDataCount;
const int       PixisServer::_iMaxReadoutTime;
const int       PixisServer::_iMaxThreadEndTime;
const int       PixisServer::_iMaxLastEventTime;
const int       PixisServer::_iMaxEventReport;
const float     PixisServer::_fEventDeltaTimeFactor = 1.01f;

/*
 * Definition of private static data
 */
pthread_mutex_t PixisServer::_mutexPlFuncs = PTHREAD_MUTEX_INITIALIZER;


PixisServer::CaptureRoutine::CaptureRoutine(PixisServer& server) : _server(server)
{
}

void PixisServer::CaptureRoutine::routine(void)
{
  _server.runCaptureTask();
}

} //namespace Pds
