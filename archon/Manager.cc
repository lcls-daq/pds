#include "pds/archon/Manager.hh"
#include "pds/archon/Driver.hh"
#include "pds/archon/Server.hh"

#include "pds/config/ArchonConfigType.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/client/Action.hh"
#include "pds/client/Fsm.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/GenericPool.hh"


namespace Pds {
  namespace Archon {

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
      ConfigAction(Manager& mgr, Driver& driver, CfgClientNfs& cfg) :
        _mgr(mgr),
        _driver(driver),
        _cfg(cfg),
        _cfgtc(_archonConfigType,cfg.src()),
        _config_buf(0),
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
          if(!_driver.configure((void*) config->config(), config->configSize())) {
            _error = true;

            UserMessage* msg = new (&_occPool) UserMessage;
            msg->append("Archon: failed to apply configuration.\n");
            _mgr.appliance().post(msg);
          } else {
            const Pds::Archon::System& system = _driver.system();
            const Pds::Archon::Status& status = _driver.status();
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

            if (_error) {
              UserMessage* msg = new (&_occPool) UserMessage("Archon Config Error: unable to controller status!\n");
              _mgr.appliance().post(msg);
            } else {

            }
          }
        }
        return tr;
      }
    private:
      Manager&          _mgr;
      Driver&           _driver;
      CfgClientNfs&     _cfg;
      Xtc               _cfgtc;
      char*             _config_buf;
      unsigned          _config_max_size;
      GenericPool       _occPool;
      bool              _error;
    };
  }
}

using namespace Pds::Archon;

Manager::Manager(Driver& driver, Server& server, CfgClientNfs& cfg) : _fsm(*new Pds::Fsm())
{
  _fsm.callback(Pds::TransitionId::Map, new AllocAction(cfg));
  _fsm.callback(Pds::TransitionId::Configure, new ConfigAction(*this, driver, cfg));
//  _fsm.callback(Pds::TransitionId::Enable   ,&driver);
//  _fsm.callback(Pds::TransitionId::Disable  ,&driver);
//  _fsm.callback(Pds::TransitionId::L1Accept ,&driver);
}

Manager::~Manager() {}

Pds::Appliance& Manager::appliance() {return _fsm;}

