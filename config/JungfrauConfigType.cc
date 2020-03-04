#include "pds/config/JungfrauConfigType.hh"

void Pds::JungfrauModConfig::setSerialNumber(JungfrauModConfigType& c,
                                             uint64_t serialNumber)
{
  new(&c) JungfrauModConfigType(serialNumber,
                                c.moduleVersion(),
                                c.firmwareVersion());
}

void Pds::JungfrauConfig::setSize(JungfrauConfigType& c,
                                  unsigned modules,
                                  unsigned rows,
                                  unsigned columns,
                                  JungfrauModConfigType* modcfg)
{
  JungfrauModConfigType _modcfg[JungfrauConfigType::MaxModulesPerDetector];
  if (modcfg) {
    for (unsigned i=0; i<modules; i++) {
      _modcfg[i] = modcfg[i];
    }
  } else {
    for (unsigned i=0; i<c.numberOfModules(); i++) {
      _modcfg[i] = c.moduleConfig(i);
    }
  }

  new(&c) JungfrauConfigType(modules,
                             rows,
                             columns,
                             c.biasVoltage(),
                             c.gainMode(),
                             c.speedMode(),
                             c.triggerDelay(),
                             c.exposureTime(),
                             c.exposurePeriod(),
                             c.vb_ds(),
                             c.vb_comp(),
                             c.vb_pixbuf(),
                             c.vref_ds(),
                             c.vref_comp(),
                             c.vref_prech(),
                             c.vin_com(),
                             c.vdd_prot(),
                             _modcfg);
}
