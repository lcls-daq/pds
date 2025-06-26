#include "pds/jungfrau/ConfigCache.hh"
#include "pds/jungfrau/Driver.hh"
#include "pds/jungfrau/Manager.hh"
#include "pds/jungfrau/DetectorId.hh"

#include "pds/config/JungfrauConfigType.hh"
#include "pds/utility/Appliance.hh"

using namespace Pds::Jungfrau;

enum {StaticALlocationNumberOfConfigurationsForScanning=100};

ConfigCache::ConfigCache(const Src& dbsrc,
                         const Src& xtcsrc,
                         Detector& detector,
                         DetIdLookup& lookup,
                         Manager& mgr) :
  CfgCache(dbsrc, xtcsrc, _jungfrauConfigType, StaticALlocationNumberOfConfigurationsForScanning * sizeof(JungfrauConfigType)),
  _occPool(sizeof(UserMessage),1),
  _detector(detector),
  _lookup(lookup),
  _mgr(mgr)
{}

ConfigCache::~ConfigCache()
{}

bool ConfigCache::configure(bool apply)
{
  bool error = false;
  unsigned nrows = 0;
  unsigned ncols = 0;
  bool mod_size_set = false;
  JungfrauConfigType& config = *reinterpret_cast<JungfrauConfigType*>(_cur_config);
  JungfrauModConfigType module_config[JungfrauConfigType::MaxModulesPerDetector];

  for (unsigned i=0; i<_detector.get_num_modules(); i++) {
    if (!mod_size_set) {
      nrows = _detector.get_num_rows(i);
      ncols = _detector.get_num_columns(i);
      mod_size_set = true;
    } else if ((_detector.get_num_rows(i) != nrows) || (_detector.get_num_columns(i) != ncols)) {
      printf("ConfigCache: detector modules with different shapes are not supported! (%u x %u) vs (%u x %u)\n",
             nrows, ncols, _detector.get_num_rows(i), _detector.get_num_columns(i));
      UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Config Error: modules with different shapes are not supported!\n");
      _mgr.appliance().post(msg);
      error = true;
    }
  }

  if (!mod_size_set) {
    printf("ConfigCache: detector seems to have no modules to configure!\n");
    UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Config Error: detector seems to have no modules to configure!\n");
    _mgr.appliance().post(msg);
    error = true;
  }

  if (!error) {
    // Retrieve the module specific read-only configuration info to updated config object
    if (!_detector.get_module_config(module_config)) {
      printf("ConfigCache: failed to retrieve module version information!\n");
      UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Config Error: failed to retrieve module version information!\n");
      _mgr.appliance().post(msg);
      error = true;
    }

    for (unsigned i=0; i<_detector.get_num_modules(); i++) {
      const char* hostname = _detector.get_hostname(i);
      if (hostname) {
        if(_lookup.has(hostname)) {
          DetId old_id(module_config[i].serialNumber());
          DetId new_id(_lookup[hostname], old_id.module());
          JungfrauModConfig::setSerialNumber(module_config[i], new_id.full());
          printf("ConfigCache: module %u serial number updated from %#lx to %#lx\n", i, old_id.full(), new_id.full());
        }
      } else {
        printf("ConfigCache: module %u appears to have no hostname to use for serial number lookup!\n", i);
        UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Config Error: failed to lookup serial number!\n");
        _mgr.appliance().post(msg);
        error = true;
      }
      printf("Module %u version info:\n"
             "  serial number:  0x%lx\n"
             "  version number: 0x%lx\n"
             "  firmware:       0x%lx\n",
             i,
             module_config[i].serialNumber(),
             module_config[i].moduleVersion(),
             module_config[i].firmwareVersion());
    }

    JungfrauConfig::setSize(config, _detector.get_num_modules(), nrows, ncols, module_config);
    DacsConfig dacs_config(config.vb_ds(), config.vb_comp(), config.vb_pixbuf(), config.vref_ds(),
                           config.vref_comp(), config.vref_prech(), config.vin_com(), config.vdd_prot());

    if (apply && !_detector.configure(0, config.gainMode(), config.speedMode(),
                                      config.triggerDelay(), config.exposureTime(),
                                      config.exposurePeriod(), config.biasVoltage(),
                                      dacs_config)) {
      error = true;
    }

    if (!_detector.check_size(config.numberOfModules(),
                              config.numberOfRowsPerModule(),
                              config.numberOfColumnsPerModule())) {
      error = true;
    }

    if (error) {
      printf("ConfigCache: failed to apply configuration.\n");

      const char** errors = _detector.errors();
      for (unsigned i=0; i<_detector.get_num_modules(); i++) {
        if (strlen(errors[i])) {
          UserMessage* msg = new (&_occPool) UserMessage("Jungfrau Config ");
          if (strlen(errors[i])) {
            msg->append(errors[i]);
          }
          _mgr.appliance().post(msg);
        }
      }
      _detector.clear_errors();
    }
  }

  if (error) {
    _configtc.damage.increase(Damage::UserDefined);
  }

  return !error;
}

int ConfigCache::_size(void* tc) const
{
  return sizeof(JungfrauConfigType);
}
