#ifndef PDS_IPIMBOARD
#define PDS_IPIMBOARD

#include <arpa/inet.h>

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <time.h>

#include "pdsdata/ipimb/ConfigV1.hh"

#define BAUDRATE B115200
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

namespace Pds {
  class IpimBoardData;
  //  class Ipimb::ConfigV1;

  class IpimBoard {
  public:
    IpimBoard(char* serialDevice);
    ~IpimBoard();// {printf("IPBM::dtor, this = %p\n", this);}
    
    enum { // was IpimBoardRegisters
      timestamp0 = 0x00,
      timestamp1 = 0x01,
      serid0 = 0x02,
      serid1 = 0x03,
      adc0 = 0x04,
      adc1 = 0x05,
      rg_config = 0x06,
      cal_rg_config = 0x07,
      reset = 0x08,
      bias_data = 0x09,
      cal_data = 0x0a,
      biasdac_data_config = 0x0b,
      status = 0x0c,
      errors = 0x0d,
      cal_strobe = 0x0e,
      trig_delay = 0x0f
    };

    void SetTriggerCounter(unsigned start0, unsigned start1);
    unsigned GetTriggerCounter1();
    void SetCalibrationMode(bool*);
    void SetCalibrationDivider(unsigned*);
    void SetCalibrationPolarity(bool*);
    void SetChargeAmplifierRef(float refVoltage);
    void SetCalibrationVoltage(float calibrationVoltage);
    void SetChargeAmplifierMultiplier(unsigned*);
    void SetInputBias(float biasVoltage);
    void SetChannelAcquisitionWindow(uint32_t acqLength, uint16_t acqDelay);
    void SetTriggerDelay(uint32_t triggerDelay);
    void CalibrationStart(unsigned calStrobeLength);
    unsigned ReadRegister(unsigned regAddr);
    void WriteRegister(unsigned regAddr, unsigned regValue);
    IpimBoardData WaitData();
    //  unsigned* read(bool command, int nBytes);
    void WriteCommand(unsigned*);
    int inWaiting(bool command);
    
    bool configure(Ipimb::ConfigV1& config);
    //    int configure(Ipimb::ConfigV1 config);
    int get_fd();
    void flush();
    bool dataDamaged();
    bool commandResponseDamaged();
    uint64_t GetSerialID();
    uint16_t GetStatus();
    uint16_t GetErrors();
    
    //  void signal_handler_IO (int status);   /* definition of signal handler */

  private:
    unsigned _commandList[4];
    unsigned _dataList[12];
    int _commandIndex;
    int _dataIndex;
    int _fd;//, _res, _maxfd;
    //    fd_set _readfs;
    bool _dataDamage, _commandResponseDamage;
    bool _startWaitingForData;
    int _w0, _w1, _w2;
  };
  
  
  class IpimBoardCommand {
  public:
    IpimBoardCommand(bool write, unsigned address, unsigned data);
    ~IpimBoardCommand();// {printf("IPBMC::dtor, this = %p\n", this);}
    
    //  list<unsigned> getData();
    unsigned* getAll();
    
  private:
    //  list<unsigned> _lst;
    unsigned _commandList[4];
  };


  class IpimBoardResponse {
  public:
    IpimBoardResponse(unsigned* packet);
    ~IpimBoardResponse();// {printf("IPBMR::dtor, this = %p\n", this);}
    
    bool CheckCRC();
    void setAll(int, int, unsigned*);
    unsigned* getAll(int, int);
    unsigned getData();
    
  private:
    unsigned _addr;
    unsigned _data;
    unsigned _checksum;
    unsigned _respList[4];
  };
  
  class IpimBoardData {
  public:
    IpimBoardData(unsigned* packet);
    IpimBoardData();
    ~IpimBoardData() {};//printf("IPBMD::dtor, this = %p\n", this);}
    
    bool CheckCRC();
    void setAll(int, int, unsigned*);
    void setList(int, int, unsigned*);
    uint64_t GetTriggerCounter();
    unsigned GetTriggerDelay_ns();
    float GetCh0_V();
    float GetCh1_V();
    float GetCh2_V();
    float GetCh3_V();
    unsigned GetConfig0();
    unsigned GetConfig1();
    unsigned GetConfig2();
    void dumpRaw();

  private:
    uint64_t _triggerCounter;
    uint16_t _config0;
    uint16_t _config1;
    uint16_t _config2;
    uint16_t _ch0;
    uint16_t _ch1;
    uint16_t _ch2;
    uint16_t _ch3;
    uint16_t _checksum;
  };
  
  unsigned CRC(unsigned*, int);
  timespec diff(timespec start, timespec end);
}
#endif
