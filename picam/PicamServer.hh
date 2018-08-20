#ifndef PICAM_SERVER_HH
#define PICAM_SERVER_HH

#include <string>

namespace Pds
{

class PicamConfig;

class PicamServer
{
public:
  virtual ~PicamServer() {}

  virtual int   initSetup()=0;
  virtual int   map()=0;
  virtual int   config(PicamConfig* config, std::string& sConfigWarning)=0;
  virtual int   unconfig()=0;
  virtual int   beginRun()=0;
  virtual int   endRun()=0;
  virtual int   beginCalibCycle()=0;
  virtual int   endCalibCycle()=0;
  virtual int   enable()=0;
  virtual int   disable()=0;
  virtual int   startExposure()=0;
  virtual int   getData (InDatagram* in, InDatagram*& out)=0;
  virtual int   waitData(InDatagram* in, InDatagram*& out)=0;
  virtual bool  IsCapturingData()=0;
  virtual PicamConfig* config()=0;

  enum  ErrorCodeEnum
  {
    ERROR_INVALID_ARGUMENTS = 1,
    ERROR_FUNCTION_FAILURE  = 2,
    ERROR_INCORRECT_USAGE   = 3,
    ERROR_LOGICAL_FAILURE   = 4,
    ERROR_SDK_FUNC_FAIL     = 5,
    ERROR_SERVER_INIT_FAIL  = 6,
    ERROR_INVALID_CONFIG    = 7,
    ERROR_TEMPERATURE       = 8,
    ERROR_SEQUENCE_ERROR    = 9,
  };

};

class PicamServerException : public std::runtime_error
{
public:
  explicit PicamServerException( const std::string& sDescription ) :
    std::runtime_error( sDescription )
  {}
};

} //namespace Pds

#endif
