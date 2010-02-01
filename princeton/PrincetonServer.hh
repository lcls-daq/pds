#ifndef PRINCETON_SERVER_HH
#define PRINCETON_SERVER_HH

#include <time.h>
#include <pthread.h>
#include <string>
#include <stdexcept>

#include "pvcam/include/master.h"
#include "pvcam/include/pvcam.h"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/princeton/ConfigV1.hh"
#include "pdsdata/princeton/FrameV1.hh"
#include "pds/xtc/InDatagram.hh"
#include "pds/xtc/Datagram.hh"

namespace Pds 
{
  
class PrincetonServer
{
public:
  PrincetonServer(bool bUseCaptureThread, bool bStreamMode, std::string sFnOutput, const Src& src, int iDebugLevel);
  ~PrincetonServer();
  
  int   configCamera(Princeton::ConfigV1& config);
  int   unconfigCamera();
  int   onEventShotIdStart(int iShotIdStart);
  int   onEventShotIdEnd(int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   onEventShotIdUpdate(int iShotIdStart, int iShotIdEnd, InDatagram* in, InDatagram*& out);
  int   getMakeUpData(InDatagram* in, InDatagram*& out);
  
private:

  int   initCamera();
  int   deinitCamera();
  void  abortAndResetCamera();
  int   initCapture();
  int   startCapture(int iBufferIndex);

  int   initControlThreads();
  int   runMonitorThread();
  int   runCaptureThread();

  int   checkInitSettings();    
  int   initCameraSettings(Princeton::ConfigV1& config);
  int   setupCooling();    
  int   isExposureInProgress();
  int   waitForNewFrameAvailable();
  int   processFrame(InDatagram* in, InDatagram*& out);
  int   writeFrameToFile(const Datagram& dgOut);  
  void  setupROI(rgn_type& region);
  void  checkTemperature();  
  
  /*
   * Initial settings
   */
  const bool        _bUseCaptureThread;
  const bool        _bStreamMode;
  const std::string _sFnOutput;
  const Src         _src;
  const int         _iDebugLevel;
  
  /*
   * Internal data
   */
  short               _hCam;
  int                 _iCurShotIdStart;
  int                 _iCurShotIdEnd;
  float               _fReadoutTime;          // in seconds
  Princeton::ConfigV1 _configCamera; 
  uns32               _uFrameSize;
  unsigned char*      _pCircBufferWithHeader; // special value: NULL -> Camera has not been configured  
  int                 _iCurBufferIndex;       // Used to locate the current buffer inside the circular buffer
  int                 _iCameraAbortAndReset;  // 0 -> normal, 1 -> resetting in progress, 2 -> reset complete
  int                 _iEventCaptureEnd;      // 0 -> normal, 1 -> capture end event triggered
  bool                _bForceCameraReset;     
  int                 _iTemperatureStatus;    // 0 -> normal, 1 -> too high
  Datagram            _dgEvent;
  InDatagram*         _pDgOut;  
  
  /*
   * private static consts
   */  
  static const int      _iMaxCoolingTime        = 10000;  // in miliseconds
  static const int      _iTemperatureTolerance  = 100;    // 1 degree Fahrenheit
  static const int      _iFrameHeaderSize;                  // Buffer header used to store the Xtc and FrameV1 object
  static const int      _iCircBufFrameCount     = 5;
  static const int      _iMaxExposureTime       = 10000;  // Limit exposure time to prevent CCD from burning
  static const int      _iMaxReadoutTime        = 3000;   // Max readout time          
  static const timespec _tmLockTimeout;                   // timeout for acquiring the lock
    
  /*
   * private static functions
   */
  static void* threadEntryMonitor(void * pServer);
  static void* threadEntryCapture(void * pServer);

  static void lockPlFunc();
  static void releaseLockPlFunc();
  
  //inline static void lockPlFunc()
  //{
  //  if ( !pthread_mutex_timedlock(&_mutexPlFuncs, &_tmLockTimeout) )
  //    printf( "PrincetonServer::lockPlFunc(): pthread_mutex_timedlock() failed\n" );
  //}
  //inline static void releaseLockPlFunc()
  //{
  //  pthread_mutex_unlock(&_mutexPlFuncs);
  //}
  
  /*
   * private static data
   */
  static pthread_mutex_t _mutexPlFuncs;
};


class PrincetonServerException : public std::runtime_error
{
public:
  explicit PrincetonServerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}  
};

} //namespace Pds 

#endif
