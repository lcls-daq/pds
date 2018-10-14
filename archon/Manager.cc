#include "pds/archon/Manager.hh"
#include "pds/archon/Driver.hh"
#include "pds/archon/Server.hh"

#include "pds/config/ArchonConfigType.hh"
#include "pds/config/ArchonDataType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/Task.hh"


namespace Pds {
  namespace Archon {
    class FrameReader : Routine {
    public:
      FrameReader(Driver& driver, Server& server, Semaphore& sem, Task* task) :
        _task(task),
        _sem(sem),
        _driver(driver),
        _server(server),
        _disable(true),
        _first(false),
        _frame_sz(0),
        _buffer_sz(0),
        _buffer(0),
        _header(0)
      {}
      virtual ~FrameReader() {
        if (_buffer) delete[] _buffer;
        if (_header) delete _header;
      }
      Task& task() { return *_task; }
      void enable () { _disable=false; _first=true; _task->call(this); }
      void disable() { _disable=true ; _driver.timeout_waits(); }
      void set_frame(uint32_t width, uint32_t height, uint32_t depth, uint32_t offset) {
        // Redo the frame header
        if (_header) delete _header;
          _header = new ArchonDataType(width, height, depth, offset);

        _frame_sz = _header->_sizeof() - sizeof(ArchonDataType);
        _server.set_frame_sz(_frame_sz);
        if (_buffer) {
          if (_frame_sz > _buffer_sz) {
            delete[] _buffer;
            _buffer = new char[_frame_sz];
            _buffer_sz = _frame_sz;
          }
        } else {
          _buffer = new char[_frame_sz];
          _buffer_sz = _frame_sz;
        }
      }
      void routine() {
        if (_disable) {
          _driver.timeout_waits(false);
          _sem.give();
        } else {
          if (_first) {
            _first = false;
            _sem.take();
          }
          if (_buffer) {
            if (_driver.wait_frame(_buffer, &_frame_meta)) {
              _server.post(_frame_meta.number, _header, _buffer);
            } else if(!_disable) {
              fprintf(stderr, "Error: FrameReader failed to retrieve frame from the Archon\n");
            }
          } else {
            fprintf(stderr, "Error: FrameReader has not been allocated a data buffer!\n");
          }
          _task->call(this);
        }
      }
    private:
      Task*           _task;
      Semaphore&      _sem;
      Driver&         _driver;
      Server&         _server;
      bool            _disable;
      bool            _first;
      unsigned        _frame_sz;
      unsigned        _buffer_sz;
      char*           _buffer;
      ArchonDataType* _header;
      FrameMetaData   _frame_meta;
    };

    class AllocAction : public Action {
    public:
      AllocAction(CfgClientNfs& cfg) : _cfg(cfg) {}
      Transition* fire(Transition* tr) {
        const Allocate& alloc = reinterpret_cast<const Allocate&>(*tr);
        _cfg.initialize(alloc.allocation());
        return tr;
      }
    private:
      CfgClientNfs& _cfg;
    };

    class ConfigAction : public Action {
    public:
      ConfigAction(Manager& mgr, Driver& driver, Server& server, FrameReader& reader, CfgClientNfs& cfg) :
        _mgr(mgr),
        _driver(driver),
        _server(server),
        _reader(reader),
        _cfg(cfg),
        _cfgtc(_archonConfigType,cfg.src()),
        _config_buf(0),
        _config_version(0),
        _config_max_size(0),
        _occPool(sizeof(UserMessage),1),
        _error(false) {
        ArchonConfigType ac_max(ArchonConfigMaxFileLength);
        _config_max_size = ac_max._sizeof();
        _config_buf = new char[_config_max_size];
      }
      ~ConfigAction() {
        if (_config_buf) delete[] _config_buf;
      }
      InDatagram* fire(InDatagram* dg) {
        // insert assumes we have enough space in the input datagram
        dg->insert(_cfgtc,    _config_buf);
        if (_error) {
          printf("*** Found configuration errors\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = false;

        int len = _cfg.fetch( *tr, _archonConfigType, _config_buf, _config_max_size );

        if (len <= 0) {
          _error = true;

          printf("ConfigAction: failed to retrieve configuration: (%d).\n", len);

          UserMessage* msg = new (&_occPool) UserMessage("Archon Config Error: failed to retrieve configuration.\n");
          _mgr.appliance().post(msg);
        } else {
          ArchonConfigType* config = reinterpret_cast<ArchonConfigType*>(_config_buf);
          _cfgtc.extent = sizeof(Xtc) + config->_sizeof();
          if(_config_version != config->configVersion()) {
            printf("Sending updated ACF file to the controller\n");
            if(!_driver.configure((void*) config->config(), config->configSize())) {
              _error = true;

              UserMessage* msg = new (&_occPool) UserMessage;
              msg->append("Archon Config Error: failed to apply configuration.\n");
              _mgr.appliance().post(msg);
            } else {
              _config_version = config->configVersion();
            }
          }

          // Power on/off the CCD
          if (config->power() == ArchonConfigType::On) {
            _driver.power_on();
          } else {
            _driver.power_off();
          }

          if (!_error) {
            const Pds::Archon::System& system = _driver.system();
            const Pds::Archon::Status& status = _driver.status();
            const Pds::Archon::BufferInfo& info = _driver.buffer_info();
            const Pds::Archon::Config& config_rbv = _driver.config();
            if(_driver.fetch_system()) {
              printf("Number of modules: %d\n", system.num_modules());
              printf("Backplane info:\n");
              printf(" Type: %u\n", system.type());
              printf(" Rev:  %u\n", system.rev());
              printf(" Ver:  %s\n", system.version().c_str());
              printf(" ID:   %#06x\n", system.id());
              printf(" Module present mask: %#06x\n", system.present());
              printf("Module Info:\n");
              for (unsigned i=0; i<16; i++) {
                if (system.module_present(i)) {
                  printf(" Module %u:\n", i+1);
                  printf("  Type: %u\n", system.module_type(i));
                  printf("  Rev:  %u\n", system.module_rev(i));
                  printf("  Ver:  %s\n", system.module_version(i).c_str());
                  printf("  ID:   %#06x\n", system.module_id(i));
                }
              }
            }

            if (_driver.fetch_status()) {
              if(status.is_valid()) {
                printf("Valid status returned by controller\n");
                printf("Number of module entries: %d\n", status.num_module_entries());
                printf("System status update count: %u\n", status.count());
                printf("Power status: %s\n", status.power_str());
                printf("Power is good: %s\n", status.is_power_good() ? "yes" : "no");
                printf("Overheated: %s\n", status.is_overheated() ? "yes" : "no");
                printf("Backplane temp: %.3f\n", status.backplane_temp());
              } else {
                printf("Invalid status returned by controller!\n");
                status.dump();
                _error = true;
              }
            }

            if (_driver.fetch_buffer_info()) {
              _server.setFrame(info.frame_num());
            } else {
              printf("Invalid buffer info returned by controller!\n");
              _error = true;
            }

            if (_error) {
              UserMessage* msg = new (&_occPool) UserMessage("Archon Config Error: unable to retrieve controller status!\n");
              _mgr.appliance().post(msg);
            } else {
              if (!_driver.set_bias(config->bias(), config->biasChan(), config->biasVoltage())) {
                printf("ConfigAction: failed to set sensor bias parameters!\n");
                _error = true;
              } else if (!_driver.set_vertical_binning(config->verticalBinning())) {
                printf("ConfigAction: failed to set vertical_binning parameter!\n");
                _error = true;
              } else if (!_driver.set_horizontal_binning(config->horizontalBinning())) {
                printf("ConfigAction: failed to set horizontal_binning parameter!\n");
                _error = true;
              } else if (!_driver.set_number_of_lines(config->lines())) {
                printf("ConfigAction: failed to set lines parameter!\n");
                _error = true;
              } else if (!_driver.set_number_of_pixels(config->pixels())) {
                printf("ConfigAction: failed to set pixels parameter!\n");
                _error = true;
              } else if (!_driver.set_preframe_clear(config->preFrameSweepCount())) {
                printf("ConfigAction: failed to set preframe_clear parameter!\n");
                _error = true;
              } else if (!_driver.set_integration_time(config->integrationTime())) {
                printf("ConfigAction: failed to set integration_time parameter!\n");
                _error = true;
              } else if (!_driver.set_non_integration_time(config->nonIntegrationTime())) {
                printf("ConfigAction: failed to set non_integration_time parameter!\n");
                _error = true;
              } else if (!_driver.set_idle_clear(config->idleSweepCount())) {
                printf("ConfigAction: failed to set idle_clear parameter!\n");
                _error = true;
              } else if (!_driver.set_external_trigger(config->readoutMode() == ArchonConfigType::Triggered)) {
                printf("ConfigAction: failed to set external_trigger parameter!\n");
                _error = true;
              } else if (!_driver.set_clock_at(config->at())) {
                printf("ConfigAction: failed to set clock_at parameter!\n");
                _error = true;
              } else if (!_driver.set_clock_st(config->st())) {
                printf("ConfigAction: failed to set clock_st parameter!\n");
                _error = true;
              } else if (!_driver.set_clock_stm1(config->stm1())) {
                printf("ConfigAction: failed to set clock_stm1 parameter!\n");
                _error = true;
              }

              if (_error) {
                UserMessage* msg = new (&_occPool) UserMessage("Archon Config Error: failed to set timing/bias parameters!\n");
                _mgr.appliance().post(msg);
              } else {
                _reader.set_frame(config_rbv.pixels_per_line(), config_rbv.linecount(), config_rbv.bytes_per_pixel()*8, 0);
                // Waiting for ccd power to come on - timeout after 2000 ms
                if(config->power() == ArchonConfigType::On) {
                  printf("Waiting for the CCD to finish powering on\n");
                  if(!_driver.wait_power_mode(Pds::Archon::On, 2000)) {
                    printf("ConfigAction: CCD power not on but was requested!\n");
                    _error = true;

                    UserMessage* msg = new (&_occPool) UserMessage("Archon Config Error: CCD power not on!\n");
                    _mgr.appliance().post(msg);
                  }
                }
              }
            }
          }

          _server.resetCount();

          if (!_error) {
            printf("Controller configuration completed\n");
          }
        }
        return tr;
      }
    private:
      Manager&          _mgr;
      Driver&           _driver;
      Server&           _server;
      FrameReader&      _reader;
      CfgClientNfs&     _cfg;
      Xtc               _cfgtc;
      char*             _config_buf;
      unsigned          _config_version;
      unsigned          _config_max_size;
      GenericPool       _occPool;
      bool              _error;
    };

    class EnableAction : public Action {
    public:
      EnableAction(Driver& driver, FrameReader& reader, Semaphore& sem):
        _driver(driver),
        _reader(reader),
        _sem(sem),
        _error(false) { }
      ~EnableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("EnableAction: failed to enable Uxi.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _error = !_driver.start_acquisition();
        _sem.give();
        _reader.enable();
        return tr;
      }
    private:
      Driver&       _driver;
      FrameReader&  _reader;
      Semaphore&    _sem;
      bool          _error;
    };

    class DisableAction : public Action {
    public:
      DisableAction(Driver& driver, FrameReader& reader, Semaphore& sem):
        _driver(driver),
        _reader(reader),
        _sem(sem),
        _error(false) { }
      ~DisableAction() { }
      InDatagram* fire(InDatagram* dg) {
        if (_error) {
          printf("DisableAction: failed to disable Uxi.\n");
          dg->datagram().xtc.damage.increase(Pds::Damage::UserDefined);
        }
        return dg;
      }
      Transition* fire(Transition* tr) {
        _reader.disable();
        _sem.take();
        _error = !_driver.stop_acquisition();
        return tr;
      }
    private:
      Driver&       _driver;
      FrameReader&  _reader;
      Semaphore&    _sem;
      bool          _error;
    };
  }
}

using namespace Pds::Archon;

Manager::Manager(Driver& driver, Server& server, CfgClientNfs& cfg) : _fsm(*new Pds::Fsm())
{
  Task* task = new Task(TaskObject("ArchonReadout",35));
  Semaphore& sem = *new Semaphore(Semaphore::EMPTY);
  FrameReader& reader = *new FrameReader(driver, server, sem, task);

  _fsm.callback(Pds::TransitionId::Map, new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure, new ConfigAction(*this, driver, server, reader, cfg));
  _fsm.callback(Pds::TransitionId::Enable   , new EnableAction(driver, reader, sem));
  _fsm.callback(Pds::TransitionId::Disable  , new DisableAction(driver, reader, sem));
//  _fsm.callback(Pds::TransitionId::L1Accept ,&driver);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

