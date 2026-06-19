#ifndef Pds_NsCam_Device_hh
#define Pds_NsCam_Device_hh

#include "pds/nscam/Types.hh"

#include <memory>
#include <string>
#include <map>

namespace Pds {
  namespace NsCam {
    class Comm;

    struct SubRegister {
      std::string regname;
      uint32_t start_bit;
      uint32_t width;
      bool writable;
      double minV;
      double maxV;
      SubRegister(std::string regname,
                  uint32_t start_bit,
                  uint32_t width,
                  bool writable);
      SubRegister(std::string regname,
                  uint32_t start_bit,
                  uint32_t width,
                  bool writable,
                  double minV,
                  double maxV);

      uint32_t minValue() const;
      uint32_t maxValue() const;
      double resolution() const;
    };

    class Device {
    public:
      Device(std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames,
             const std::map<std::string, uint32_t>& defaults);
      Device(std::shared_ptr<Comm> comm,
             const std::map<std::string, uint16_t>& regnames,
             const std::map<std::string, SubRegister>& subregnames,
             const std::map<std::string, uint32_t>& defaults,
             const std::map<std::string, std::string>& aliases,
             const std::map<std::string, std::string>& monitors);
      virtual ~Device() = default;

      virtual uint32_t getRegister(const std::string& regname) const;
      virtual uint32_t getRegister(uint16_t address) const;
      virtual void setRegister(const std::string& regname, uint32_t value);
      virtual void setRegister(uint16_t address, uint32_t value);
      virtual uint32_t getSubRegister(const std::string& subregname) const;
      virtual uint32_t getSubRegister(const SubRegister& subreg) const;
      virtual void setSubRegister(const std::string& subregname, uint32_t value);
      virtual void setSubRegister(const SubRegister& subreg, uint32_t value);

      virtual double getPot(const std::string& potname) const;
      virtual double getPotV(const std::string& potname) const;
      virtual void setPot(const std::string& potname, double fraction=1.0);
      virtual void setPotV(const std::string& potname, double voltage, bool tune=false, double accuracy=0.01, int iterations=20, double approach=0.75, bool tuneErr=false);
      virtual double getMonV(const std::string& potname) const;

      virtual CommType commType() const;
      virtual std::string commName() const;

      static uint32_t getBit(uint32_t regval, uint32_t bit);
      static uint64_t combineRegisters(uint32_t lowregval, uint32_t highregval);

    protected:
      virtual void addRegister(const std::string& regname, uint16_t address);
      virtual void addSubRegister(const std::string& regname, const SubRegister& subreg);
      virtual void addDefault(const std::string& regname, uint32_t value);
      virtual double convertMonV(double fraction) const;
      virtual void initDefaults();

    private:
      std::string resolveRegisterName(const std::string& regname) const;
      uint16_t lookupRegister(const std::string& regname) const;
      const SubRegister& lookupSubRegister(const std::string& subregname) const;
      std::string lookupMonitor(const std::string& potname) const;
      bool hasMonitor(const std::string& potname) const;

      std::map<std::string, uint16_t> regnames_;
      std::map<std::string, SubRegister> subregnames_;
      std::map<std::string, uint32_t> defaults_;
      std::map<std::string, std::string> aliases_;
      std::map<std::string, std::string> monitors_;
      std::shared_ptr<Comm> comm_;
    };
  }
}

#endif
